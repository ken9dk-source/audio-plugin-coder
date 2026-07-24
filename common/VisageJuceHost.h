/*
  ==============================================================================
    VisageJuceHost.h
    Bridge for JUCE 8 + Visage (Fixed Rendering Pipeline)
  ==============================================================================
*/
#pragma once
#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_events/juce_events.h>
#include "visage/app.h"
#include "visage/ui.h"
#include "visage/graphics.h"

// Crash Handler
static void npsCrashHandler(void*) {
    auto logFile = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
                   .getChildFile("APC_CRASH_REPORT.txt");
    juce::String report = "TIME: " + juce::Time::getCurrentTime().toString(true, true) + "\n";
    report += juce::SystemStats::getStackBacktrace();
    logFile.replaceWithText(report);
}

/**
 * VisagePluginEditor - A JUCE AudioProcessorEditor that hosts Visage UI
 * 
 * This class properly integrates Visage's rendering pipeline with JUCE's OpenGL context.
 * 
 * Key concepts:
 * 1. Visage uses a Frame hierarchy where each Frame has a Region
 * 2. The Canvas manages rendering and needs regions added to it
 * 3. Frames must be initialized and have their event handlers set up
 * 4. The redraw() mechanism triggers actual drawing via drawToRegion()
 */
class VisagePluginEditor : public juce::AudioProcessorEditor,
                           private juce::Timer
{
public:
    VisagePluginEditor(juce::AudioProcessor& p) : AudioProcessorEditor(&p) {
        static bool crashHandlerSet = false;
        if (!crashHandlerSet) {
            juce::SystemStats::setApplicationCrashHandler(npsCrashHandler);
            crashHandlerSet = true;
        }
        
        setOpaque(true);
        startTimerHz(60);
    }

    ~VisagePluginEditor() override {
        stopTimer();
        teardownVisage();
    }

    void paint(juce::Graphics& g) override { 
        g.fillAll(juce::Colours::black);

        if (windowless_ && backbuffer_.isValid()) {
            g.drawImageAt(backbuffer_, 0, 0);
        }
    }

    void mouseDown(const juce::MouseEvent& e) override {
        dispatchMouse(e, MouseDispatch::Down);
    }

    void mouseDrag(const juce::MouseEvent& e) override {
        dispatchMouse(e, MouseDispatch::Drag);
    }

    void mouseUp(const juce::MouseEvent& e) override {
        dispatchMouse(e, MouseDispatch::Up);
    }

    void mouseMove(const juce::MouseEvent& e) override {
        dispatchMouse(e, MouseDispatch::Move);
    }

    void mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) override {
        if (!event_root_)
            return;
        const visage::Point window_pos = { e.position.x, e.position.y };
        visage::Frame* target = event_root_->frameAtPoint(window_pos);
        if (!target)
            return;
        visage::MouseEvent me;
        me.window_position = window_pos;
        me.position = window_pos - target->positionInWindow();
        me.event_frame = target;
        me.wheel_delta_x = wheel.deltaX;
        me.wheel_delta_y = wheel.deltaY;
        me.precise_wheel_delta_x = wheel.deltaX;
        me.precise_wheel_delta_y = wheel.deltaY;
        me.wheel_momentum = wheel.isInertial;
        me.button_state = visage::kMouseButtonNone;
        me.button_id = last_button_id_;
        // Bubble up through parents until someone handles it
        while (target) {
            if (!target->ignoresMouseEvents()) {
                target->processMouseWheel(me);
                break;
            }
            target = target->parent();
        }
    }

    void resized() override { 
        onResize(getWidth(), getHeight()); 
        if (canvas_) {
            if (windowless_) {
                canvas_->setWindowless(getWidth(), getHeight());
            } else {
                canvas_->setDimensions(getWidth(), getHeight());
            }
        }
    }

    void timerCallback() override {
        if (!rendererInitialized_) {
            tryInitialize();
            return;
        }

        if (!canvas_)
            return;

        onRender();

        // In windowless (software) mode the canvas framebuffer is cleared every frame,
        // so we must mark every frame dirty before drawing or anything not in the
        // stale-list will vanish from the screenshot.  (Non-windowless HW mode uses
        // persistent per-region textures and does NOT need this.)
        if (windowless_ && event_root_)
            event_root_->redrawAll();

        drawStaleFrames();
        canvas_->submit();

        if (windowless_) {
            updateBackbufferFromScreenshot(canvas_->takeScreenshot());
            repaint();
        }
    }

    // Override these in your subclass
    virtual void onInit() {}
    virtual void onRender() {}
    virtual void onDestroy() {}
    virtual void onResize(int w, int h) {}

protected:
    visage::Canvas& getCanvas() { return *canvas_; }
    visage::FrameEventHandler& getEventHandler() { return eventHandler_; }
    void setEventRoot(visage::Frame* root) { event_root_ = root; }
    
    /**
     * Add a frame to the canvas for rendering.
     * This sets up the frame's region and event handler.
     */
    void addFrameToCanvas(visage::Frame* frame) {
        if (!canvas_ || !frame)
            return;
            
        // Add the frame's region to the canvas
        canvas_->addRegion(frame->region());
        
        // Set up the event handler so redraw() works
        frame->setEventHandler(&eventHandler_);
        
        // Set DPI scale
        frame->setDpiScale((float)getDesktopScaleFactor());
        
        // Initialize the frame
        frame->init();
        
        // Trigger initial redraw
        frame->redrawAll();
    }
    
    /**
     * Remove a frame from the canvas.
     */
    void removeFrameFromCanvas(visage::Frame* frame) {
        if (!frame)
            return;
            
        // Clear event handler
        frame->setEventHandler(nullptr);
        
        // Remove from stale list
        auto pos = std::find(staleFrames_.begin(), staleFrames_.end(), frame);
        if (pos != staleFrames_.end())
            staleFrames_.erase(pos);
    }
    
    /**
     * Draw all frames that need redrawing.
     * This is called automatically in renderOpenGL().
     */
    void drawStaleFrames() {
        if (!canvas_)
            return;
            
        // Swap stale list to avoid issues if redraw() is called during draw
        std::vector<visage::Frame*> drawing;
        std::swap(staleFrames_, drawing);
        
        for (visage::Frame* frame : drawing) {
            if (frame && frame->isDrawing())
                frame->drawToRegion(*canvas_);
        }
        
        // Handle any frames that were added during drawing
        for (auto it = staleFrames_.begin(); it != staleFrames_.end();) {
            visage::Frame* frame = *it;
            if (std::find(drawing.begin(), drawing.end(), frame) == drawing.end()) {
                if (frame && frame->isDrawing())
                    frame->drawToRegion(*canvas_);
                it = staleFrames_.erase(it);
            } else {
                ++it;
            }
        }
    }

private:
    enum class MouseDispatch {
        Down,
        Drag,
        Up,
        Move
    };

    void dispatchMouse(const juce::MouseEvent& e, MouseDispatch type) {
        if (!event_root_)
            return;

        // Component-local coords == Visage logical window coords
        // (mainView fills the entire editor at logical origin (0,0))
        const visage::Point window_pos = {
            static_cast<float>(e.position.x),
            static_cast<float>(e.position.y)
        };

        // Build common modifier / button state
        int mods = visage::kModifierNone;
        if (e.mods.isShiftDown())   mods |= visage::kModifierShift;
        if (e.mods.isCtrlDown())    mods |= visage::kModifierRegCtrl;
        if (e.mods.isAltDown())     mods |= visage::kModifierAlt;
        if (e.mods.isCommandDown()) mods |= visage::kModifierCmd;

        int buttons = visage::kMouseButtonNone;
        if (e.mods.isLeftButtonDown())   buttons |= visage::kMouseButtonLeft;
        if (e.mods.isMiddleButtonDown()) buttons |= visage::kMouseButtonMiddle;
        if (e.mods.isRightButtonDown())  buttons |= visage::kMouseButtonRight;

        if (type == MouseDispatch::Down) {
            last_button_id_ = e.mods.isRightButtonDown()  ? visage::kMouseButtonRight :
                              e.mods.isMiddleButtonDown() ? visage::kMouseButtonMiddle :
                                                            visage::kMouseButtonLeft;
        }

        auto makeEvent = [&](visage::Frame* target) {
            visage::MouseEvent me;
            me.window_position = window_pos;
            me.position        = window_pos - target->positionInWindow();
            me.relative_position = me.position;   // Visage's relativePosition() reads this field
            me.event_frame     = target;
            me.modifiers       = mods;
            me.button_state    = buttons;
            me.button_id       = last_button_id_;
            me.is_down         = (type != MouseDispatch::Up);
            return me;
        };

        if (type == MouseDispatch::Down) {
            // Hit-test: find the deepest child under the cursor
            mouse_down_frame_ = event_root_->frameAtPoint(window_pos);
            if (mouse_down_frame_) {
                auto me = makeEvent(mouse_down_frame_);
                mouse_down_frame_->processMouseDown(me);
            }
        }
        else if (type == MouseDispatch::Drag) {
            // Always deliver drag to the frame that got the Down event
            if (mouse_down_frame_) {
                auto me = makeEvent(mouse_down_frame_);
                mouse_down_frame_->processMouseDrag(me);
            }
        }
        else if (type == MouseDispatch::Up) {
            visage::Frame* up_target = mouse_down_frame_;
            mouse_down_frame_ = nullptr;
            if (up_target) {
                auto me = makeEvent(up_target);
                up_target->processMouseUp(me);
            }
        }
        else if (type == MouseDispatch::Move) {
            if (mouse_down_frame_) {
                // Button held but JUCE sent Move instead of Drag — treat as drag
                auto me = makeEvent(mouse_down_frame_);
                mouse_down_frame_->processMouseDrag(me);
            } else {
                // Update hover state
                visage::Frame* hovered = event_root_->frameAtPoint(window_pos);
                if (hovered != mouse_hovered_frame_) {
                    if (mouse_hovered_frame_) {
                        auto me = makeEvent(mouse_hovered_frame_);
                        mouse_hovered_frame_->processMouseExit(me);
                    }
                    if (hovered) {
                        auto me = makeEvent(hovered);
                        hovered->processMouseEnter(me);
                    }
                    mouse_hovered_frame_ = hovered;
                } else if (mouse_hovered_frame_) {
                    auto me = makeEvent(mouse_hovered_frame_);
                    mouse_hovered_frame_->processMouseMove(me);
                }
            }
        }
    }

    void tryInitialize() {
        if (rendererInitialized_)
            return;

        auto* peer = getPeer();
        if (!peer)
            return;

        void* nativeWindow = peer->getNativeHandle();
        if (!nativeWindow)
            return;

        visage::Renderer::instance().initialize(nativeWindow, nullptr);

        canvas_ = std::make_unique<visage::Canvas>();

        // TEMP: Swap-chain path is unstable in plugin hosting. Use windowless render for preview.
        constexpr bool kForceWindowless = true;
        if (kForceWindowless || !visage::Canvas::swapChainSupported()) {
            windowless_ = true;
            canvas_->setWindowless(getWidth(), getHeight());
        } else {
            windowless_ = false;
            canvas_->pairToWindow(nativeWindow, getWidth(), getHeight());
        }

        canvas_->setDpiScale((float)getDesktopScaleFactor());

        eventHandler_.request_redraw = [this](visage::Frame* frame) {
            if (std::find(staleFrames_.begin(), staleFrames_.end(), frame) == staleFrames_.end())
                staleFrames_.push_back(frame);
        };

        eventHandler_.remove_from_hierarchy = [this](visage::Frame* frame) {
            auto pos = std::find(staleFrames_.begin(), staleFrames_.end(), frame);
            if (pos != staleFrames_.end())
                staleFrames_.erase(pos);
        };

        rendererInitialized_ = true;
        onInit();
    }

    void teardownVisage() {
        rendererInitialized_ = false;
        staleFrames_.clear();
        onDestroy();
        if (canvas_) {
            canvas_->removeFromWindow();
            canvas_.reset();
        }
        windowless_ = false;
        backbuffer_ = juce::Image();
    }

    void updateBackbufferFromScreenshot(const visage::Screenshot& shot) {
        if (shot.width() <= 0 || shot.height() <= 0)
            return;

        if (!backbuffer_.isValid() || backbuffer_.getWidth() != shot.width() || backbuffer_.getHeight() != shot.height()) {
            backbuffer_ = juce::Image(juce::Image::ARGB, shot.width(), shot.height(), true);
        }

        juce::Image::BitmapData data(backbuffer_, juce::Image::BitmapData::writeOnly);
        const uint8_t* src = shot.data();
        for (int y = 0; y < shot.height(); ++y) {
            auto* dst = reinterpret_cast<juce::PixelARGB*>(data.getLinePointer(y));
            const uint8_t* row = src + (y * shot.width() * 4);
            for (int x = 0; x < shot.width(); ++x) {
                const uint8_t r = row[x * 4 + 0];
                const uint8_t g = row[x * 4 + 1];
                const uint8_t b = row[x * 4 + 2];
                const uint8_t a = row[x * 4 + 3];
                dst[x].setARGB(a, r, g, b);
            }
        }
    }

    std::unique_ptr<visage::Canvas> canvas_;
    visage::FrameEventHandler eventHandler_;
    std::vector<visage::Frame*> staleFrames_;
    bool rendererInitialized_ = false;
    bool windowless_ = false;
    juce::Image backbuffer_;
    visage::Frame* event_root_       = nullptr;
    visage::Frame* mouse_down_frame_ = nullptr;   // Frame that received the last MouseDown
    visage::Frame* mouse_hovered_frame_ = nullptr; // Frame currently under the cursor
    visage::MouseButton last_button_id_ = visage::kMouseButtonLeft;
};
