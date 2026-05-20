#pragma once

#include <visage/ui.h>
#include <visage/graphics.h>
#include "BinaryData.h"

#include <array>
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>

// ============================================================
//  SID Trance Machine — Visage UI Widgets
//  Style: future-retro trance, dark navy + cyan neon
// ============================================================

namespace SIDColors {
    // Backgrounds
    static constexpr uint32_t BG_VOID         = 0xFF060E1E;
    static constexpr uint32_t BG_PANEL        = 0xFF0A1628;
    static constexpr uint32_t BG_PANEL_DARK   = 0xFF071120;
    static constexpr uint32_t BG_SECTION      = 0xFF0F1E35;

    // Borders
    static constexpr uint32_t BORDER_PANEL    = 0xFF1A3A6B;
    static constexpr uint32_t BORDER_INNER    = 0xFF0F2A50;
    static constexpr uint32_t SEPARATOR       = 0xFF152840;

    // Accent cyan (most controls)
    static constexpr uint32_t ACCENT_CYAN_BRIGHT = 0xFF00D4FF;
    static constexpr uint32_t ACCENT_CYAN        = 0xFF0099CC;
    static constexpr uint32_t ACCENT_CYAN_DIM    = 0xFF005577;
    static constexpr uint32_t ACCENT_CYAN_GLOW   = 0x4400D4FF;

    // Accent green (resonance only)
    static constexpr uint32_t ACCENT_GREEN    = 0xFF00FF7F;
    static constexpr uint32_t ACCENT_GREEN_DIM= 0xFF006633;

    // Knob / fader
    static constexpr uint32_t KNOB_BODY       = 0xFF1A2840;
    static constexpr uint32_t KNOB_BODY_LARGE = 0xFF101E32;
    static constexpr uint32_t KNOB_INDICATOR  = 0xFFE8F4FD;
    static constexpr uint32_t FADER_TRACK     = 0xFF0C1A30;
    static constexpr uint32_t FADER_THUMB     = 0xFF1E3A5A;
    static constexpr uint32_t FADER_THUMB_ACT = 0xFF2A5080;

    // Step sequencer
    static constexpr uint32_t STEP_ACTIVE     = 0xFF1E6FBF;
    static constexpr uint32_t STEP_INACTIVE   = 0xFF0A1E38;
    static constexpr uint32_t STEP_PLAYING    = 0xFF00D4FF;
    static constexpr uint32_t STEP_BORDER     = 0xFF1A4080;

    // Text
    static constexpr uint32_t TEXT_PRIMARY    = 0xFFE8F4FD;
    static constexpr uint32_t TEXT_LABEL      = 0xFF8BAFD4;
    static constexpr uint32_t TEXT_DIM        = 0xFF4A6A8A;
    static constexpr uint32_t TEXT_ACCENT     = 0xFF00D4FF;

    // Buttons
    static constexpr uint32_t BTN_OFF_BG      = 0xFF0A1628;
    static constexpr uint32_t BTN_OFF_BORDER  = 0xFF1A3A6B;
    static constexpr uint32_t BTN_ON_BG       = 0xFF003366;
    static constexpr uint32_t BTN_ON_BORDER   = 0xFF0066CC;
    static constexpr uint32_t BTN_ON_TEXT     = 0xFF00D4FF;
    static constexpr uint32_t LED_ON          = 0xFFFFAA00;
    static constexpr uint32_t LED_OFF         = 0xFF332200;

    // Scope
    static constexpr uint32_t SCOPE_LINE      = 0xFF00D4FF;
    static constexpr uint32_t SCOPE_FILL      = 0x2200D4FF;
    static constexpr uint32_t SCOPE_BG        = 0xFF060E1E;
}

// ============================================================
//  Font cache — shared across all widgets
// ============================================================
struct SIDFonts {
    visage::Font title;
    visage::Font section;
    visage::Font label;
    visage::Font value;
    visage::Font button;

    void init(float dpi, const unsigned char* fontData, int fontDataSize) {
        title   = visage::Font(28.0f, fontData, fontDataSize, dpi);
        section = visage::Font(10.0f, fontData, fontDataSize, dpi);
        label   = visage::Font(9.0f,  fontData, fontDataSize, dpi);
        value   = visage::Font(11.0f, fontData, fontDataSize, dpi);
        button  = visage::Font(9.0f,  fontData, fontDataSize, dpi);
    }
};

// ============================================================
//  SIDPanelBase — shared panel background/header drawing
// ============================================================
class SIDPanelBase : public visage::Frame {
public:
    explicit SIDPanelBase(const std::string& headerText = "")
        : headerText_(headerText) {}

    void setHeaderText(const std::string& t) { headerText_ = t; redraw(); }
    void setFonts(SIDFonts* f)               { fonts_ = f; }

protected:
    void drawPanelBase(visage::Canvas& canvas) {
        // Background
        canvas.setColor(SIDColors::BG_PANEL);
        canvas.roundedRectangle(0, 0, width(), height(), 4.0f);
        canvas.fill(0, 0, width(), height());

        // Border
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangle(0, 0, width(), height(), 4.0f);

        // Separator under header
        canvas.setColor(SIDColors::SEPARATOR);
        canvas.fill(4, 18, width() - 8, 1);

        // Header text
        if (!headerText_.empty() && fonts_) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(headerText_, fonts_->section, visage::Font::kCenter,
                        4, 4, width() - 8, 14);
        }
    }

    std::string headerText_;
    SIDFonts*   fonts_ = nullptr;
};

// ============================================================
//  SIDKnob — rotary knob with arc ring
// ============================================================
class SIDKnob : public visage::Frame {
public:
    std::function<void(float)> onValueChanged;

    SIDKnob() = default;

    void setValue(float v, bool notify = false) {
        value_ = std::clamp(v, 0.0f, 1.0f);
        redraw();
        if (notify && onValueChanged) onValueChanged(value_);
    }
    float getValue() const { return value_; }

    void setRingColor(uint32_t c) { ringColor_ = c; }
    void setLarge(bool b)         { large_ = b; }
    void setLabel(const std::string& l) { label_ = l; }
    void setFonts(SIDFonts* f)    { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        const float cx = width()  * 0.5f;
        const float cy = height() * 0.5f - (label_.empty() ? 0.0f : 7.0f);
        const float r  = std::min(cx, cy) - 4.0f;
        const float trackWidth = large_ ? 5.0f : 3.0f;

        static constexpr float kStartAngle = 135.0f * (3.14159265f / 180.0f);
        static constexpr float kSweep      = 270.0f * (3.14159265f / 180.0f);

        // Knob body
        canvas.setColor(large_ ? SIDColors::KNOB_BODY_LARGE : SIDColors::KNOB_BODY);
        canvas.circle(cx - r, cy - r, r * 2.0f);

        // Track (full arc, dim)
        drawArcApprox(canvas, cx, cy, r, kStartAngle, kSweep, SIDColors::ACCENT_CYAN_DIM, trackWidth);

        // Value arc
        const float valueSweep = kSweep * value_;
        drawArcApprox(canvas, cx, cy, r, kStartAngle, valueSweep, ringColor_, trackWidth);

        // Glow on hover
        if (hovered_) {
            canvas.setColor(SIDColors::ACCENT_CYAN_GLOW);
            canvas.circle(cx - r - 2, cy - r - 2, (r + 2) * 2.0f);
        }

        // Indicator line
        const float angle = kStartAngle + kSweep * value_;
        const float ix = cx + (r - 6.0f) * std::cos(angle);
        const float iy = cy + (r - 6.0f) * std::sin(angle);
        canvas.setColor(SIDColors::KNOB_INDICATOR);
        canvas.segment(cx, cy, ix, iy, 1.5f, false);

        // Label
        if (!label_.empty() && fonts_) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(label_, fonts_->label, visage::Font::kCenter,
                        0, static_cast<int>(height()) - 14, width(), 12);
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        dragStartY_ = e.position.y;
        dragStartValue_ = value_;
    }

    void mouseDrag(const visage::MouseEvent& e) override {
        const float delta = (dragStartY_ - e.position.y) / 200.0f;
        setValue(std::clamp(dragStartValue_ + delta, 0.0f, 1.0f), true);
    }

    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit(const visage::MouseEvent&)  override { hovered_ = false; redraw(); }

private:
    void drawArcApprox(visage::Canvas& canvas, float cx, float cy, float r,
                        float startAngle, float sweep, uint32_t color, float lineWidth) {
        // Approximate arc as N line segments
        const int N = 40;
        canvas.setColor(color);
        for (int i = 0; i < N; ++i) {
            const float a0 = startAngle + sweep * (float(i)   / N);
            const float a1 = startAngle + sweep * (float(i+1) / N);
            const float x0 = cx + r * std::cos(a0);
            const float y0 = cy + r * std::sin(a0);
            const float x1 = cx + r * std::cos(a1);
            const float y1 = cy + r * std::sin(a1);
            canvas.segment(x0, y0, x1, y1, lineWidth, false);
        }
    }

    float       value_          = 0.5f;
    uint32_t    ringColor_      = SIDColors::ACCENT_CYAN;
    bool        large_          = false;
    bool        hovered_        = false;
    std::string label_;
    SIDFonts*   fonts_          = nullptr;
    float       dragStartY_     = 0.0f;
    float       dragStartValue_ = 0.5f;
};

// ============================================================
//  SIDFader — vertical fader for ADSR
// ============================================================
class SIDFader : public visage::Frame {
public:
    std::function<void(float)> onValueChanged;

    void setValue(float v, bool notify = false) {
        value_ = std::clamp(v, 0.0f, 1.0f);
        redraw();
        if (notify && onValueChanged) onValueChanged(value_);
    }
    float getValue() const { return value_; }
    void setLabel(const std::string& l) { label_ = l; }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        const float trackX = width() * 0.5f - 3.0f;
        const float trackTop = 4.0f;
        const float trackBot = height() - (label_.empty() ? 4.0f : 16.0f);
        const float trackH   = trackBot - trackTop;
        const float thumbY   = trackTop + trackH * (1.0f - value_);

        // Track
        canvas.setColor(SIDColors::FADER_TRACK);
        canvas.roundedRectangle(trackX, trackTop, 6.0f, trackH, 3.0f);
        canvas.fill(0, 0, width(), height());

        // Fill from bottom
        canvas.setColor(SIDColors::ACCENT_CYAN_DIM);
        canvas.fill(trackX + 1.0f, thumbY, 4.0f, trackBot - thumbY);

        // Thumb
        canvas.setColor(hovered_ ? SIDColors::FADER_THUMB_ACT : SIDColors::FADER_THUMB);
        canvas.roundedRectangle(trackX - 9.0f, thumbY - 5.0f, 24.0f, 10.0f, 2.0f);
        canvas.fill(0, 0, width(), height());

        // Label
        if (!label_.empty() && fonts_) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(label_, fonts_->label, visage::Font::kCenter,
                        0, static_cast<int>(height()) - 14, width(), 12);
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        dragStartY_ = e.position.y;
        dragStartValue_ = value_;
    }

    void mouseDrag(const visage::MouseEvent& e) override {
        const float trackH = height() - 20.0f;
        const float delta  = (dragStartY_ - e.position.y) / trackH;
        setValue(std::clamp(dragStartValue_ + delta, 0.0f, 1.0f), true);
    }

    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit(const visage::MouseEvent&)  override { hovered_ = false; redraw(); }

private:
    float       value_          = 0.5f;
    bool        hovered_        = false;
    std::string label_;
    SIDFonts*   fonts_          = nullptr;
    float       dragStartY_     = 0.0f;
    float       dragStartValue_ = 0.5f;
};

// ============================================================
//  SIDToggleButton — lit/unlit button (SYNC, RETRIG, KEY TRACK…)
// ============================================================
class SIDToggleButton : public visage::Frame {
public:
    std::function<void(bool)> onToggled;

    void setState(bool on) { on_ = on; redraw(); }
    bool getState() const  { return on_; }
    void setLabel(const std::string& l) { label_ = l; }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        canvas.setColor(on_ ? SIDColors::BTN_ON_BG : SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);
        canvas.fill(0, 0, width(), height());

        canvas.setColor(on_ ? SIDColors::BTN_ON_BORDER : SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);

        if (fonts_) {
            canvas.setColor(on_ ? SIDColors::BTN_ON_TEXT : SIDColors::TEXT_DIM);
            canvas.text(label_, fonts_->button, visage::Font::kCenter,
                        2, 2, width() - 4, height() - 4);
        }
    }

    void mouseDown(const visage::MouseEvent&) override {
        on_ = !on_;
        redraw();
        if (onToggled) onToggled(on_);
    }

private:
    bool        on_    = false;
    std::string label_;
    SIDFonts*   fonts_ = nullptr;
};

// ============================================================
//  SIDStepButton — 16-step sequencer pad
// ============================================================
class SIDStepButton : public visage::Frame {
public:
    std::function<void(bool)> onToggled;
    void setState(bool on) { on_ = on; redraw(); }
    bool getState() const  { return on_; }
    void setPlaying(bool p){ playing_ = p; redraw(); }
    void setNumber(int n)  { number_ = n; }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        uint32_t bg = playing_ ? SIDColors::STEP_PLAYING
                     : on_     ? SIDColors::STEP_ACTIVE
                               : SIDColors::STEP_INACTIVE;
        canvas.setColor(bg);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);
        canvas.fill(0, 0, width(), height());

        canvas.setColor(SIDColors::STEP_BORDER);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);

        if (fonts_) {
            canvas.setColor(on_ ? SIDColors::TEXT_PRIMARY : SIDColors::TEXT_DIM);
            canvas.text(std::to_string(number_), fonts_->label, visage::Font::kCenter,
                        0, 0, width(), height());
        }
    }

    void mouseDown(const visage::MouseEvent&) override {
        on_ = !on_;
        redraw();
        if (onToggled) onToggled(on_);
    }

private:
    bool      on_      = false;
    bool      playing_ = false;
    int       number_  = 1;
    SIDFonts* fonts_   = nullptr;
};

// ============================================================
//  SIDOscilloscopeView — waveform scope (top panel)
// ============================================================
class SIDOscilloscopeView : public visage::Frame {
public:
    void setLabel(const std::string& l) { label_ = l; }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    // Call from audio thread via AbstractFifo — sets waveform samples
    void updateSamples(const float* data, int count) {
        const int n = std::min(count, kMaxSamples);
        for (int i = 0; i < n; ++i)
            samples_[i] = data[i];
        sampleCount_ = n;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        // Background
        canvas.setColor(SIDColors::SCOPE_BG);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);
        canvas.fill(0, 0, width(), height());

        // Border
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);

        // Center line
        const float cy = height() * 0.5f;
        canvas.setColor(SIDColors::SEPARATOR);
        canvas.segment(4, cy, width() - 4, cy, 0.5f, false);

        // Waveform
        if (sampleCount_ > 1) {
            const float xStep = (width() - 8.0f) / float(sampleCount_ - 1);
            canvas.setColor(SIDColors::SCOPE_LINE);
            for (int i = 1; i < sampleCount_; ++i) {
                const float x0 = 4.0f + (i - 1) * xStep;
                const float y0 = cy - samples_[i - 1] * (cy - 4.0f);
                const float x1 = 4.0f + i * xStep;
                const float y1 = cy - samples_[i]     * (cy - 4.0f);
                canvas.segment(x0, y0, x1, y1, 1.0f, false);
            }
        }

        // Label
        if (!label_.empty() && fonts_) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(label_, fonts_->label, visage::Font::kLeft,
                        4, 2, width() - 8, 12);
        }
    }

private:
    static constexpr int kMaxSamples = 512;
    float samples_[kMaxSamples] = {};
    int   sampleCount_          = 0;
    std::string label_;
    SIDFonts*   fonts_ = nullptr;
};

// ============================================================
//  SIDOscillatorPanel — one oscillator section (OSC 1/2/3)
// ============================================================
class SIDOscillatorPanel : public SIDPanelBase {
public:
    SIDKnob  semiKnob, fineKnob, pwKnob, volumeKnob;
    SIDFader attackFader, decayFader, sustainFader, releaseFader;
    SIDOscilloscopeView envelopeDisplay;

    // Waveform selector buttons (6 types)
    static constexpr int kNumWaveforms = 6;
    SIDToggleButton waveButtons[kNumWaveforms];

    explicit SIDOscillatorPanel(int oscNum)
        : SIDPanelBase("OSCILLATOR " + std::to_string(oscNum)) {}

    void init() override {
        updateFontsAndLayout();
    }

    void dpiChanged() override {
        updateFontsAndLayout();
    }

    void resized() override {
        const int w = width();
        const int h = height();

        // Waveform buttons row
        const int btnW = (w - 8) / kNumWaveforms;
        for (int i = 0; i < kNumWaveforms; ++i)
            waveButtons[i].setBounds(4 + i * btnW, 22, btnW - 2, 22);

        // Knobs row
        const int kx = 4, ky = 52, ks = 52;
        semiKnob.setBounds(kx,          ky, ks, ks);
        fineKnob.setBounds(kx + ks + 8, ky, ks, ks);
        pwKnob.setBounds(kx + 2*(ks+8), ky, ks, ks);

        // ADSR faders
        const int fx = 10, fy = 116, fw = 28, fh = 80, fgap = 42;
        attackFader.setBounds(fx,            fy, fw, fh);
        decayFader.setBounds(fx + fgap,      fy, fw, fh);
        sustainFader.setBounds(fx + 2*fgap,  fy, fw, fh);
        releaseFader.setBounds(fx + 3*fgap,  fy, fw, fh);

        // Envelope display + volume knob
        envelopeDisplay.setBounds(4, 206, w - 62, 50);
        volumeKnob.setBounds(w - 58, 206, ks, ks);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
    }

private:
    void updateFontsAndLayout() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);

        // Push fonts to children
        for (auto& b : waveButtons) b.setFonts(&fonts_);
        semiKnob.setFonts(&fonts_);    semiKnob.setLabel("SEMI");
        fineKnob.setFonts(&fonts_);    fineKnob.setLabel("FINE");
        pwKnob.setFonts(&fonts_);      pwKnob.setLabel("PW");
        volumeKnob.setFonts(&fonts_);  volumeKnob.setLabel("VOL");
        attackFader.setFonts(&fonts_);  attackFader.setLabel("A");
        decayFader.setFonts(&fonts_);   decayFader.setLabel("D");
        sustainFader.setFonts(&fonts_); sustainFader.setLabel("S");
        releaseFader.setFonts(&fonts_); releaseFader.setLabel("R");
        envelopeDisplay.setFonts(&fonts_);

        static const std::array<std::string, kNumWaveforms> waveLabels = {
            "SAW", "TRI", "PLS", "NOI", "S+T", "RNG"
        };
        for (int i = 0; i < kNumWaveforms; ++i)
            waveButtons[i].setLabel(waveLabels[i]);
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDFilterPanel — cutoff + resonance + type + slope + key/vel
// ============================================================
class SIDFilterPanel : public SIDPanelBase {
public:
    SIDKnob        cutoffKnob, resonanceKnob;
    SIDToggleButton keyTrackBtn, velocityBtn;

    SIDFilterPanel() : SIDPanelBase("FILTER") {
        cutoffKnob.setLarge(true);
        cutoffKnob.setLabel("CUTOFF");
        resonanceKnob.setLarge(true);
        resonanceKnob.setLabel("RESONANCE");
        resonanceKnob.setRingColor(SIDColors::ACCENT_GREEN);
        keyTrackBtn.setLabel("KEY TRACK");
        velocityBtn.setLabel("VELOCITY");
    }

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int ks = 76;
        cutoffKnob.setBounds(20, 28, ks, ks);
        resonanceKnob.setBounds(width() - ks - 20, 28, ks, ks);
        keyTrackBtn.setBounds(4, height() - 48, 88, 22);
        velocityBtn.setBounds(width() / 2 + 4, height() - 48, 88, 22);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
        cutoffKnob.setFonts(&fonts_);
        resonanceKnob.setFonts(&fonts_);
        keyTrackBtn.setFonts(&fonts_);
        velocityBtn.setFonts(&fonts_);
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDAmplifierPanel — global amp ADSR
// ============================================================
class SIDAmplifierPanel : public SIDPanelBase {
public:
    SIDKnob attackKnob, decayKnob, sustainKnob, releaseKnob, volumeKnob;

    SIDAmplifierPanel() : SIDPanelBase("AMPLIFIER") {}

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int ks = 40, gap = 4;
        int x = 4;
        attackKnob.setBounds(x, 24, ks, ks);   x += ks + gap;
        decayKnob.setBounds(x, 24, ks, ks);    x += ks + gap;
        sustainKnob.setBounds(x, 24, ks, ks);  x += ks + gap;
        releaseKnob.setBounds(x, 24, ks, ks);  x += ks + gap;
        volumeKnob.setBounds(x, 24, ks, ks);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
        attackKnob.setFonts(&fonts_);  attackKnob.setLabel("ATTACK");
        decayKnob.setFonts(&fonts_);   decayKnob.setLabel("DECAY");
        sustainKnob.setFonts(&fonts_); sustainKnob.setLabel("SUSTAIN");
        releaseKnob.setFonts(&fonts_); releaseKnob.setLabel("RELEASE");
        volumeKnob.setFonts(&fonts_);  volumeKnob.setLabel("VOLUME");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDLFOPanel — LFO shape + rate + sync + retrig
// ============================================================
class SIDLFOPanel : public SIDPanelBase {
public:
    SIDKnob        rateKnob;
    SIDToggleButton syncBtn, retrigBtn;

    explicit SIDLFOPanel(int lfoNum)
        : SIDPanelBase("LFO " + std::to_string(lfoNum)) {}

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        rateKnob.setBounds(80, 20, 44, 44);
        syncBtn.setBounds(130, 20, 26, 20);
        retrigBtn.setBounds(130, 44, 26, 20);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        // Shape selector placeholder (drawn as text until dropdown impl)
        if (fonts_.label.size() > 0) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("Sine", fonts_.label, visage::Font::kCenter, 4, 28, 72, 20);
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
        rateKnob.setFonts(&fonts_);  rateKnob.setLabel("RATE");
        syncBtn.setFonts(&fonts_);   syncBtn.setLabel("SYNC");
        retrigBtn.setFonts(&fonts_); retrigBtn.setLabel("RETRIG");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDArpPanel — 16-step arp / sequencer
// ============================================================
class SIDArpPanel : public SIDPanelBase {
public:
    static constexpr int kStepCount = 16;
    SIDStepButton steps[kStepCount];
    SIDToggleButton syncBtn;

    SIDArpPanel() : SIDPanelBase("ARP / SEQUENCER") {}

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int stepW = 36, stepH = 36, gap = 6;
        const int totalW = kStepCount * (stepW + gap) - gap;
        int sx = (width() - totalW) / 2;
        for (int i = 0; i < kStepCount; ++i) {
            steps[i].setBounds(sx, 58, stepW, stepH);
            sx += stepW + gap;
        }
        syncBtn.setBounds(width() - 52, 26, 44, 20);
    }

    void setPlayingStep(int step) {
        for (int i = 0; i < kStepCount; ++i)
            steps[i].setPlaying(i == step);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
        for (int i = 0; i < kStepCount; ++i) {
            steps[i].setFonts(&fonts_);
            steps[i].setNumber(i + 1);
        }
        syncBtn.setFonts(&fonts_);
        syncBtn.setLabel("SYNC");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDEffectsPanel — Chorus / Delay / Reverb
// ============================================================
class SIDEffectsPanel : public SIDPanelBase {
public:
    SIDToggleButton chorusBtn, delayBtn, reverbBtn;
    SIDKnob chorusRate, chorusDepth, chorusMix;
    SIDKnob delayFeedback, delayMix;
    SIDKnob reverbSize, reverbDamping, reverbMix;

    SIDEffectsPanel() : SIDPanelBase("EFFECTS") {}

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int ks = 34;
        // Chorus row
        chorusBtn.setBounds(4, 28, 76, 20);
        chorusRate.setBounds(84, 20, ks, ks);
        chorusDepth.setBounds(84 + ks + 4, 20, ks, ks);
        chorusMix.setBounds(84 + 2*(ks+4), 20, ks, ks);

        // Delay row
        delayBtn.setBounds(4, 88, 76, 20);
        delayFeedback.setBounds(84, 80, ks, ks);
        delayMix.setBounds(84 + ks + 4, 80, ks, ks);

        // Reverb row
        reverbBtn.setBounds(4, 148, 76, 20);
        reverbSize.setBounds(84, 140, ks, ks);
        reverbDamping.setBounds(84 + ks + 4, 140, ks, ks);
        reverbMix.setBounds(84 + 2*(ks+4), 140, ks, ks);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        // Dividers between FX sections
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(4, 70, width() - 8, 1);
        canvas.fill(4, 130, width() - 8, 1);
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
        chorusBtn.setFonts(&fonts_);  chorusBtn.setLabel("CHORUS");
        delayBtn.setFonts(&fonts_);   delayBtn.setLabel("DELAY");
        reverbBtn.setFonts(&fonts_);  reverbBtn.setLabel("REVERB");
        chorusRate.setFonts(&fonts_);    chorusRate.setLabel("RATE");
        chorusDepth.setFonts(&fonts_);   chorusDepth.setLabel("DEPTH");
        chorusMix.setFonts(&fonts_);     chorusMix.setLabel("MIX");
        delayFeedback.setFonts(&fonts_); delayFeedback.setLabel("FDBK");
        delayMix.setFonts(&fonts_);      delayMix.setLabel("MIX");
        reverbSize.setFonts(&fonts_);    reverbSize.setLabel("SIZE");
        reverbDamping.setFonts(&fonts_); reverbDamping.setLabel("DAMP");
        reverbMix.setFonts(&fonts_);     reverbMix.setLabel("MIX");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDModMatrixPanel — 4-slot modulation routing
// ============================================================
class SIDModMatrixPanel : public SIDPanelBase {
public:
    SIDModMatrixPanel() : SIDPanelBase("MOD MATRIX") {}

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {}

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        // Column headers
        if (fonts_.label.size() > 0) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("SOURCE", fonts_.label, visage::Font::kLeft, 4, 22, 80, 12);
            canvas.text("AMOUNT", fonts_.label, visage::Font::kCenter, 90, 22, 80, 12);
            canvas.text("DESTINATION", fonts_.label, visage::Font::kCenter, 174, 22, 118, 12);
        }

        // Row placeholders (dropdowns implemented in /impl)
        const std::array<std::string, 4> sources = {"LFO 1", "LFO 2", "Velocity", "Mod Wheel"};
        const std::array<std::string, 4> dests   = {"Filter Cutoff", "OSC1 Pulse Width",
                                                      "Amp Level", "Filter Resonance"};
        const std::array<float, 4>       amounts = {0.375f, 0.25f, 0.5f, 0.4f};

        for (int row = 0; row < 4; ++row) {
            const int ry = 38 + row * 30;

            // Source box
            canvas.setColor(SIDColors::BTN_OFF_BG);
            canvas.roundedRectangle(4, ry, 80, 20, 2.0f);
            canvas.fill(0, 0, width(), height());
            canvas.setColor(SIDColors::BTN_OFF_BORDER);
            canvas.roundedRectangle(4, ry, 80, 20, 2.0f);
            if (fonts_.label.size() > 0) {
                canvas.setColor(SIDColors::TEXT_PRIMARY);
                canvas.text(sources[row], fonts_.label, visage::Font::kCenter, 6, ry, 78, 20);
            }

            // Amount bar
            const float barFull = 80.0f;
            const float barVal  = amounts[row];
            canvas.setColor(SIDColors::BG_SECTION);
            canvas.fill(90, ry + 6, static_cast<int>(barFull), 8);
            canvas.setColor(SIDColors::ACCENT_CYAN);
            canvas.fill(90, ry + 6, static_cast<int>(barFull * barVal), 8);
            if (fonts_.label.size() > 0) {
                const std::string amtStr = "+" + std::to_string(int(barVal * 100)) + "%";
                canvas.setColor(SIDColors::TEXT_ACCENT);
                canvas.text(amtStr, fonts_.label, visage::Font::kCenter, 90, ry, 80, 20);
            }

            // Dest box
            canvas.setColor(SIDColors::BTN_OFF_BG);
            canvas.roundedRectangle(174, ry, 118, 20, 2.0f);
            canvas.fill(0, 0, width(), height());
            canvas.setColor(SIDColors::BTN_OFF_BORDER);
            canvas.roundedRectangle(174, ry, 118, 20, 2.0f);
            if (fonts_.label.size() > 0) {
                canvas.setColor(SIDColors::TEXT_PRIMARY);
                canvas.text(dests[row], fonts_.label, visage::Font::kCenter, 176, ry, 114, 20);
            }
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDMasterPanel — output + limiter + poly + voices
// ============================================================
class SIDMasterPanel : public SIDPanelBase {
public:
    SIDFader     outputFader;
    SIDToggleButton limiterBtn;

    SIDMasterPanel() : SIDPanelBase("MASTER") {}

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        outputFader.setBounds(60, 20, 28, 180);
        limiterBtn.setBounds(100, 208, 44, 18);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        // dB scale markings
        if (fonts_.label.size() > 0) {
            const std::array<std::string, 7> marks = {"+6","0","-6","-12","-18","-24","-inf"};
            const float y0 = 24.0f, h = 176.0f;
            for (int i = 0; i < 7; ++i) {
                const float y = y0 + h * (float(i) / 6.0f);
                canvas.setColor(SIDColors::TEXT_DIM);
                canvas.text(marks[i], fonts_.label, visage::Font::kRight,
                            4, static_cast<int>(y) - 6, 50, 12);
                // Tick mark
                canvas.setColor(SIDColors::SEPARATOR);
                canvas.segment(56, y, 60, y, 0.5f, false);
            }

            // Labels below fader
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("LIMITER", fonts_.label, visage::Font::kLeft, 4, 208, 90, 14);
            canvas.text("MONO / POLY", fonts_.label, visage::Font::kLeft, 4, 232, width() - 8, 14);
            canvas.text("POLY", fonts_.value, visage::Font::kCenter, 4, 250, width() - 8, 18);
            canvas.text("VOICE COUNT", fonts_.label, visage::Font::kLeft, 4, 276, width() - 8, 14);
            canvas.text("16", fonts_.value, visage::Font::kCenter, 4, 293, width() - 8, 18);
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_);
        outputFader.setFonts(&fonts_);
        limiterBtn.setFonts(&fonts_); limiterBtn.setLabel("ON");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDMainView — root frame, assembles all panels
// ============================================================
class SIDMainView : public visage::Frame {
public:
    // Top scopes
    SIDOscilloscopeView scope1, scope2, scope3, filterScope;

    // Oscillator panels
    SIDOscillatorPanel osc1 { 1 };
    SIDOscillatorPanel osc2 { 2 };
    SIDOscillatorPanel osc3 { 3 };

    // Filter / Amp / Master
    SIDFilterPanel    filterPanel;
    SIDAmplifierPanel ampPanel;
    SIDMasterPanel    masterPanel;

    // Bottom row
    SIDModMatrixPanel modMatrix;
    SIDEffectsPanel   effectsPanel;
    SIDLFOPanel       lfo1 { 1 };
    SIDLFOPanel       lfo2 { 2 };
    SIDArpPanel       arpPanel;

    void init() override {
        updateFonts();
        // All children are added in PluginEditor::onInit()
    }

    void dpiChanged() override {
        updateFonts();
    }

    void resized() override {
        layoutAll();
    }

    void draw(visage::Canvas& canvas) override {
        // Window background
        canvas.setColor(SIDColors::BG_VOID);
        canvas.fill(0, 0, width(), height());

        // Plugin title (top right)
        if (font_title_.size() > 0) {
            canvas.setColor(SIDColors::TEXT_PRIMARY);
            canvas.text("SID TRANCE MACHINE", font_title_, visage::Font::kRight,
                        940, 8, 252, 36);
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("C-MOS", font_label_, visage::Font::kRight,
                        940, 48, 252, 24);
            canvas.setColor(SIDColors::TEXT_DIM);
            canvas.text("MOS Chip Inside", font_label_, visage::Font::kRight,
                        940, 78, 252, 16);
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            const auto* fd = reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf);
            const int   fs = BinaryData::LatoRegular_ttfSize;
            font_title_ = visage::Font(28.0f, fd, fs, dpi);
            font_label_ = visage::Font(16.0f, fd, fs, dpi);
        }
    }

    void layoutAll() {
        const int W = width();
        const int H = height();
        const int PAD = 8, GAP = 4;

        // ── Header row ──────────────────────────────────────
        const int scopeY = PAD, scopeH = 110;
        scope1.setBounds(96,  scopeY, 182, scopeH);
        scope2.setBounds(282, scopeY, 182, scopeH);
        scope3.setBounds(468, scopeY, 182, scopeH);
        filterScope.setBounds(654, scopeY, 282, scopeH);

        // ── Middle section ──────────────────────────────────
        const int midY = scopeY + scopeH + GAP;
        const int midH = 308;
        const int oscW = 200;
        osc1.setBounds(PAD,               midY, oscW, midH);
        osc2.setBounds(PAD + oscW + GAP,  midY, oscW, midH);
        osc3.setBounds(PAD + 2*(oscW+GAP),midY, oscW, midH);

        const int filterX = PAD + 3*(oscW+GAP);
        filterPanel.setBounds(filterX, midY, 220, 160);
        ampPanel.setBounds(filterX, midY + 164, 220, 144);
        masterPanel.setBounds(filterX + 224, midY, 148, midH);

        // ── Bottom section ─────────────────────────────────
        const int botY = midY + midH + GAP;
        const int botH = 226;
        modMatrix.setBounds(PAD, botY, 296, botH);
        effectsPanel.setBounds(PAD + 300, botY, 196, botH);

        const int lfoX = PAD + 300 + 200;
        lfo1.setBounds(lfoX, botY, 160, 108);
        lfo2.setBounds(lfoX + 164, botY, 160, 108);

        const int arpY  = botY + 114;
        const int arpW  = W - (lfoX) - PAD;
        arpPanel.setBounds(lfoX, arpY, arpW, botH - 114);

        // ── Preset bar ─────────────────────────────────────
        // Drawn inline in draw() by PluginEditor for now
        (void)H; // suppress unused warning
    }

    visage::Font font_title_;
    visage::Font font_label_;
};
