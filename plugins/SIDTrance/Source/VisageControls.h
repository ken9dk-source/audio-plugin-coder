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
//  SIDyssey — Visage UI Widgets
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

    // ADSR envelope colors (reference image color scheme)
    static constexpr uint32_t ENV_ATTACK      = 0xFFFF7700;  // orange
    static constexpr uint32_t ENV_DECAY       = 0xFFFFCC00;  // yellow
    static constexpr uint32_t ENV_SUSTAIN     = 0xFF0099FF;  // blue
    static constexpr uint32_t ENV_RELEASE     = 0xFFAA00FF;  // purple
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
        const float W = float(width());
        const float H = float(height());

        // ── Panel background ───────────────────────────────────
        canvas.setColor(SIDColors::BG_PANEL);
        canvas.roundedRectangle(0, 0, W, H, 4.0f);

        // ── Outer border ───────────────────────────────────────
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangleBorder(0, 0, W, H, 4.0f, 1.0f);

        if (!headerText_.empty() && fonts_) {
            // ── Header bar fill ───────────────────────────────
            canvas.setColor(0xFF0D2244);
            canvas.fill(1, 1, W - 2, 18);

            // ── Cyan accent line along top of header ──────────
            canvas.setColor(SIDColors::ACCENT_CYAN_DIM);
            canvas.fill(1, 1, W - 2, 1);

            // ── Header text (bright white) ────────────────────
            canvas.setColor(SIDColors::TEXT_PRIMARY);
            canvas.text(headerText_, fonts_->section, visage::Font::kCenter,
                        4, 3, W - 8, 14);
        }

        // ── Separator line under header ────────────────────────
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.fill(1, 19, W - 2, 1);
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
        // arcR  = outer radius for the ring track
        // bodyR = inner knob cap, visually separated from the ring
        const float arcR  = std::min(cx, cy) - 3.0f;
        if (arcR < 4.0f) return;
        const float trackWidth = large_ ? 5.5f : 3.5f;
        const float bodyR = std::max(arcR * 0.52f, arcR - trackWidth - 2.0f);

        static constexpr float kPi         = 3.14159265f;
        static constexpr float kStartAngle = 135.0f * (kPi / 180.0f);
        static constexpr float kSweep      = 270.0f * (kPi / 180.0f);
        const float angle = kStartAngle + kSweep * value_;

        // ── Outer halo glow (behind everything) ───────────────
        if (hovered_) {
            const uint32_t haloCol = (ringColor_ & 0x00FFFFFFu) | 0x12000000u;
            canvas.setColor(haloCol);
            canvas.circle(cx - arcR - 5, cy - arcR - 5, (arcR + 5) * 2.0f);
        }

        // ── Arc track — dim groove (full 270°) ────────────────
        drawArcApprox(canvas, cx, cy, arcR, kStartAngle, kSweep,
                      0xFF091C30, trackWidth);

        // ── Arc value fill — wide bloom + sharp core ──────────
        const float valueSweep = kSweep * value_;
        if (valueSweep > 0.002f) {
            // Bloom glow (wide, semi-transparent)
            const uint32_t bloomCol = (ringColor_ & 0x00FFFFFFu) | 0x30000000u;
            drawArcApprox(canvas, cx, cy, arcR, kStartAngle, valueSweep, bloomCol, trackWidth + 6.0f);
            // Mid glow (medium, more opaque)
            const uint32_t midCol   = (ringColor_ & 0x00FFFFFFu) | 0x70000000u;
            drawArcApprox(canvas, cx, cy, arcR, kStartAngle, valueSweep, midCol,   trackWidth + 2.0f);
            // Core bright arc
            drawArcApprox(canvas, cx, cy, arcR, kStartAngle, valueSweep, ringColor_, trackWidth);
        }

        // ── Knob body shadow rim ───────────────────────────────
        canvas.setColor(0xFF020609);
        canvas.circle(cx - bodyR - 1.5f, cy - bodyR - 1.5f, (bodyR + 1.5f) * 2.0f);

        // ── Knob body ──────────────────────────────────────────
        const uint32_t bodyCol = large_ ? SIDColors::KNOB_BODY_LARGE : SIDColors::KNOB_BODY;
        canvas.setColor(bodyCol);
        canvas.circle(cx - bodyR, cy - bodyR, bodyR * 2.0f);

        // ── 3-D radial gradient faked as offset circles ────────
        // Dark lower-right shadow
        canvas.setColor(0x18000000);
        canvas.circle(cx - bodyR + bodyR * 0.18f, cy - bodyR + bodyR * 0.22f,
                      bodyR * 2.0f);
        // Light upper-left sheen
        const float shineR = bodyR * 0.50f;
        canvas.setColor(0x1AFFFFFF);
        canvas.circle(cx - bodyR * 0.30f - shineR,
                      cy - bodyR * 0.35f - shineR,
                      shineR * 2.0f);

        // ── Indicator line (center → body edge) ───────────────
        const float lineR = bodyR - 2.5f;
        const float ix = cx + lineR * std::cos(angle);
        const float iy = cy + lineR * std::sin(angle);
        // Drop shadow
        canvas.setColor(0x55000000);
        canvas.segment(cx + 0.6f, cy + 0.7f, ix + 0.6f, iy + 0.7f,
                       large_ ? 2.5f : 1.8f, false);
        // Bright white line
        canvas.setColor(SIDColors::KNOB_INDICATOR);
        canvas.segment(cx, cy, ix, iy, large_ ? 2.0f : 1.4f, false);

        // ── Tip dot — sits exactly ON the arc ring ────────────
        {
            const float tipX  = cx + arcR * std::cos(angle);
            const float tipY  = cy + arcR * std::sin(angle);
            const float tipR  = large_ ? 3.5f : 2.5f;
            // Tip glow aura
            const uint32_t tipBloom = (ringColor_ & 0x00FFFFFFu) | 0x55000000u;
            canvas.setColor(tipBloom);
            canvas.circle(tipX - tipR - 2.0f, tipY - tipR - 2.0f, (tipR + 2.0f) * 2.0f);
            // Solid colored dot
            canvas.setColor(ringColor_);
            canvas.circle(tipX - tipR, tipY - tipR, tipR * 2.0f);
            // Specular highlight on tip dot
            canvas.setColor(0xB0FFFFFF);
            canvas.circle(tipX - tipR * 0.45f, tipY - tipR * 0.55f, tipR * 0.65f);
        }

        // ── Center pivot dot ───────────────────────────────────
        const float dotR = large_ ? 3.0f : 1.8f;
        canvas.setColor(0xFF050C18);
        canvas.circle(cx - dotR, cy - dotR, dotR * 2.0f);
        canvas.setColor(0xFF1A3050);
        canvas.circle(cx - dotR * 0.6f, cy - dotR * 0.6f, dotR * 1.1f);

        // ── Label ──────────────────────────────────────────────
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

    // Scroll wheel adjusts value (0.01 per wheel tick, fine-grained control)
    bool mouseWheel(const visage::MouseEvent& e) override {
        const float delta = e.wheel_delta_y * 0.01f;
        setValue(std::clamp(value_ + delta, 0.0f, 1.0f), true);
        return true;
    }

private:
    void drawArcApprox(visage::Canvas& canvas, float cx, float cy, float r,
                        float startAngle, float sweep, uint32_t color, float lineWidth) {
        // Approximate arc as N line segments (16 is visually smooth and ~60% cheaper than 40)
        const int N = 16;
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
    void setActiveColor(uint32_t c) { activeColor_ = c; }

    void draw(visage::Canvas& canvas) override {
        const float trackX   = width() * 0.5f - 3.0f;
        const float trackTop = 6.0f;
        const float trackBot = height() - (label_.empty() ? 6.0f : 18.0f);
        const float trackH   = trackBot - trackTop;
        const float thumbY   = trackTop + trackH * (1.0f - value_);

        // ── Tick marks (right side, 5 marks) ──────────────────
        static constexpr int kTicks = 5;
        const float tickX0 = trackX + 8.0f;
        const float tickX1 = tickX0 + 5.0f;
        for (int t = 0; t <= kTicks; ++t) {
            const float ty = trackTop + trackH * (float(t) / kTicks);
            canvas.setColor(SIDColors::BORDER_INNER);
            canvas.segment(tickX0, ty, tickX1, ty, 0.8f, false);
        }

        // ── Track groove ───────────────────────────────────────
        canvas.setColor(0xFF030810);   // outer deep shadow
        canvas.roundedRectangle(trackX, trackTop, 6.0f, trackH, 3.0f);
        canvas.setColor(SIDColors::FADER_TRACK);
        canvas.roundedRectangle(trackX + 1.0f, trackTop + 1.0f, 4.0f, trackH - 2.0f, 2.0f);

        // ── Colored fill — from bottom to thumb (value zone) ──
        const float fillTop = thumbY + 5.5f;
        const float fillH   = trackBot - fillTop;
        if (fillH > 1.0f) {
            // Wide bloom glow behind fill
            const uint32_t bloomCol = (activeColor_ & 0x00FFFFFFu) | 0x28000000u;
            canvas.setColor(bloomCol);
            canvas.roundedRectangle(trackX - 1.0f, fillTop, 8.0f, fillH, 3.0f);
            // Actual colored fill (track width)
            canvas.setColor(activeColor_);
            canvas.roundedRectangle(trackX + 1.0f, fillTop, 4.0f, fillH, 2.0f);
        }

        // ── Thumb — outer dark shadow rim ─────────────────────
        canvas.setColor(0xFF03080F);
        canvas.roundedRectangle(trackX - 11.0f, thumbY - 6.5f, 28.0f, 13.0f, 4.0f);

        // ── Thumb — main metal body ────────────────────────────
        const uint32_t thumbBase = hovered_ ? SIDColors::FADER_THUMB_ACT : SIDColors::FADER_THUMB;
        canvas.setColor(thumbBase);
        canvas.roundedRectangle(trackX - 10.0f, thumbY - 5.5f, 26.0f, 11.0f, 3.0f);

        // ── Thumb — beveled edge (bottom-right darker) ────────
        canvas.setColor(0x30000000);
        canvas.roundedRectangle(trackX - 9.0f, thumbY, 25.0f, 6.0f, 3.0f);

        // ── Thumb — three horizontal grip lines ───────────────
        for (int g = -1; g <= 1; ++g) {
            const float gy = thumbY + float(g) * 2.5f;
            canvas.setColor(0xFF1A3550);
            canvas.fill(trackX - 8.0f, gy - 0.5f, 22.0f, 1.0f);
        }

        // ── Thumb — center bright groove line (main chrome stripe) ──
        canvas.setColor(0xFF3A607A);
        canvas.fill(trackX - 7.5f, thumbY - 0.5f, 21.0f, 2.0f);

        // ── Thumb — top specular sheen ─────────────────────────
        canvas.setColor(0x28FFFFFF);
        canvas.roundedRectangle(trackX - 9.0f, thumbY - 5.0f, 24.0f, 5.0f, 3.0f);

        // ── Active color tint on thumb edge ────────────────────
        if (hovered_) {
            const uint32_t tintCol = (activeColor_ & 0x00FFFFFFu) | 0x25000000u;
            canvas.setColor(tintCol);
            canvas.roundedRectangleBorder(trackX - 10.0f, thumbY - 5.5f, 26.0f, 11.0f, 3.0f, 1.0f);
        }

        // ── Label ──────────────────────────────────────────────
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

    // Scroll wheel for precise adjustment (0.02 per tick — faders have shorter range)
    bool mouseWheel(const visage::MouseEvent& e) override {
        const float delta = e.wheel_delta_y * 0.02f;
        setValue(std::clamp(value_ + delta, 0.0f, 1.0f), true);
        return true;
    }

private:
    float       value_          = 0.5f;
    uint32_t    activeColor_    = SIDColors::ACCENT_CYAN_DIM;
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
    void setLabel(const std::string& l) { label_ = l; labelOff_ = l; }
    // Set different text for each state (e.g. "24dB" on, "12dB" off)
    void setLabels(const std::string& onLabel, const std::string& offLabel) {
        label_ = onLabel; labelOff_ = offLabel;
    }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());

        // ── LED mode — small square indicator (no text) ────────
        if (W <= 18.0f && H <= 18.0f) {
            // Off: dark square; On: bright colored square
            canvas.setColor(on_ ? SIDColors::ACCENT_CYAN : SIDColors::BTN_OFF_BG);
            canvas.roundedRectangle(0, 0, W, H, 2.5f);
            // Border
            canvas.setColor(on_ ? SIDColors::BTN_ON_BORDER : SIDColors::BTN_OFF_BORDER);
            canvas.roundedRectangleBorder(0, 0, W, H, 2.5f, 1.0f);
            // Inner highlight when on
            if (on_) {
                canvas.setColor(0x60FFFFFF);
                canvas.circle(W * 0.25f - 1.5f, H * 0.25f - 1.5f, 3.0f);
            }
            return;
        }

        // ── Normal button mode ─────────────────────────────────
        canvas.setColor(on_ ? SIDColors::BTN_ON_BG : SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(0, 0, W, H, 3.0f);

        if (on_) {
            canvas.setColor(0x1400AADD);
            canvas.roundedRectangle(1, 1, W - 2, H - 2, 2.5f);
            canvas.setColor(0x1AFFFFFF);
            canvas.roundedRectangle(1, 1, W - 2, H * 0.45f, 2.5f);
        }

        canvas.setColor(on_ ? SIDColors::BTN_ON_BORDER : SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(0, 0, W, H, 3.0f, 1.0f);

        if (fonts_) {
            canvas.setColor(on_ ? SIDColors::BTN_ON_TEXT : SIDColors::TEXT_DIM);
            const std::string& lbl = on_ ? label_ : labelOff_;
            canvas.text(lbl, fonts_->button, visage::Font::kCenter,
                        2, 2, W - 4, H - 4);
        }
    }

    void mouseDown(const visage::MouseEvent&) override {
        on_ = !on_;
        redraw();
        if (onToggled) onToggled(on_);
    }

private:
    bool        on_       = false;
    std::string label_;
    std::string labelOff_;
    SIDFonts*   fonts_    = nullptr;
};

// ============================================================
//  SIDWaveButton — waveform selector with pixel-art wave icon
// ============================================================
class SIDWaveButton : public visage::Frame {
public:
    std::function<void(bool)> onToggled;

    void setState(bool on)     { on_ = on;       redraw(); }
    bool getState() const      { return on_; }
    void setWaveType(int w)    { waveType_ = w;  redraw(); }
    void setFonts(SIDFonts*)   {}  // accepts call, not needed

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());
        const float r = 3.0f;

        // Background
        canvas.setColor(on_ ? SIDColors::BTN_ON_BG : (hovered_ ? 0xFF0D1E38 : SIDColors::BTN_OFF_BG));
        canvas.roundedRectangle(0, 0, W, H, r);

        // Inner glow + sheen when ON
        if (on_) {
            canvas.setColor(0x1800BBDD);
            canvas.roundedRectangle(1, 1, W - 2, H - 2, r - 0.5f);
            canvas.setColor(0x18FFFFFF);
            canvas.roundedRectangle(1, 1, W - 2, H * 0.45f, r - 0.5f);
        }

        // Border
        canvas.setColor(on_ ? SIDColors::BTN_ON_BORDER : SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(0, 0, W, H, r, 1.0f);

        // Wave icon
        const float px = 4.0f, py = 4.0f;
        const float pw = W - 8.0f, ph = H - 8.0f;
        const float mid = py + ph * 0.5f;
        canvas.setColor(on_ ? SIDColors::ACCENT_CYAN_BRIGHT : SIDColors::TEXT_DIM);
        drawWaveIcon(canvas, waveType_, px, py, pw, ph, mid);
    }

    void mouseDown(const visage::MouseEvent&) override {
        on_ = !on_;
        redraw();
        if (onToggled) onToggled(on_);
    }
    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit(const visage::MouseEvent&)  override { hovered_ = false; redraw(); }

private:
    void drawWaveIcon(visage::Canvas& c, int type,
                      float x, float /*y*/, float w, float h, float mid) {
        switch (type) {
        case 0: // SAW  /|
            c.segment(x,           mid + h*0.42f, x + w,        mid - h*0.42f, 1.6f, false);
            c.segment(x + w,       mid - h*0.42f, x + w,        mid + h*0.42f, 1.6f, false);
            break;
        case 1: // TRI  /\
            c.segment(x,           mid,           x + w*0.5f,   mid - h*0.44f, 1.6f, false);
            c.segment(x + w*0.5f,  mid - h*0.44f, x + w,        mid,           1.6f, false);
            break;
        case 2: // SQR  _|‾|_
            c.segment(x,           mid + h*0.38f, x,            mid - h*0.38f, 1.6f, false);
            c.segment(x,           mid - h*0.38f, x + w*0.5f,   mid - h*0.38f, 1.6f, false);
            c.segment(x + w*0.5f,  mid - h*0.38f, x + w*0.5f,   mid + h*0.38f, 1.6f, false);
            c.segment(x + w*0.5f,  mid + h*0.38f, x + w,        mid + h*0.38f, 1.6f, false);
            break;
        case 3: { // NOI  random jagged
            static constexpr float kN[] = {0.0f,0.65f,-0.45f,0.3f,-0.72f,0.18f,0.55f,-0.38f,0.0f};
            for (int i = 1; i < 9; ++i) {
                c.segment(x + w*((i-1)/8.0f), mid - kN[i-1]*h*0.42f,
                          x + w*(i    /8.0f), mid - kN[i]  *h*0.42f, 1.4f, false);
            }
            break;
        }
        case 4: // S+T  saw-left + tri-right
            c.segment(x,           mid + h*0.38f, x + w*0.45f,  mid - h*0.38f, 1.4f, false);
            c.segment(x + w*0.45f, mid - h*0.38f, x + w*0.45f,  mid,           1.4f, false);
            c.segment(x + w*0.52f, mid,           x + w*0.76f,  mid - h*0.38f, 1.4f, false);
            c.segment(x + w*0.76f, mid - h*0.38f, x + w,        mid,           1.4f, false);
            break;
        case 5: { // RNG  ring-mod envelope
            for (int i = 1; i <= 10; ++i) {
                const float t0 = float(i-1) / 10.0f;
                const float t1 = float(i)   / 10.0f;
                const float a0 = t0 * 6.2832f * 2.2f;
                const float a1 = t1 * 6.2832f * 2.2f;
                const float am = 0.42f * (0.35f + 0.65f * std::abs(std::sin(t0 * 3.14159f)));
                c.segment(x + t0*w, mid - std::sin(a0)*h*am,
                          x + t1*w, mid - std::sin(a1)*h*am, 1.4f, false);
            }
            break;
        }
        case 6: { // HSW  Hypersaw — three overlapping saws to suggest stacked detune
            // Draw 3 slightly offset saw waves to represent the dense supersaw cluster.
            // Center saw (s=1) is drawn thicker for visual emphasis.
            static constexpr float kOffsets[3] = { -0.12f, 0.0f, +0.12f };
            for (int s = 0; s < 3; ++s) {
                const float yOff  = kOffsets[s] * h;
                const float thick = (s == 1) ? 1.8f : 1.0f;
                c.segment(x,     mid + h*0.38f + yOff, x + w, mid - h*0.38f + yOff, thick, false);
                c.segment(x + w, mid - h*0.38f + yOff, x + w, mid + h*0.38f + yOff, thick, false);
            }
            break;
        }
        default: break;
        }
    }

    bool  on_       = false;
    bool  hovered_  = false;
    int   waveType_ = 0;
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
        // Background fill (self-filling)
        canvas.setColor(bg);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);

        // Border outline only
        canvas.setColor(SIDColors::STEP_BORDER);
        canvas.roundedRectangleBorder(0, 0, width(), height(), 3.0f, 1.0f);

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
//  SIDCycleButton — click to step through labeled options
// ============================================================
class SIDCycleButton : public visage::Frame {
public:
    std::function<void(int)> onChanged;

    void setOptions(std::vector<std::string> opts) {
        options_ = std::move(opts);
        if (!options_.empty())
            index_ = std::clamp(index_, 0, (int)options_.size() - 1);
        redraw();
    }
    void setIndex(int i) {
        if (!options_.empty())
            index_ = std::clamp(i, 0, (int)options_.size() - 1);
        else
            index_ = i;
        redraw();
    }
    int  getIndex() const { return index_; }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());
        const float r = 3.0f;

        // Background
        canvas.setColor(hovered_ ? 0xFF0D1E38 : SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(0, 0, W, H, r);

        // Border
        canvas.setColor(hovered_ ? SIDColors::ACCENT_CYAN_DIM : SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(0, 0, W, H, r, 1.0f);

        // Separator before arrow zone
        const float sepX = W - 16.0f;
        canvas.setColor(SIDColors::BTN_OFF_BORDER);
        canvas.fill(sepX, 2.0f, 1.0f, H - 4.0f);

        // Option text (left-padded)
        if (fonts_ && !options_.empty()) {
            canvas.setColor(SIDColors::TEXT_PRIMARY);
            canvas.text(options_[index_], fonts_->label, visage::Font::kLeft,
                        5, 0, sepX - 3.0f, H);
        }

        // Down chevron  ▼  (two line segments forming a V)
        const float ax = W - 9.5f;
        const float ay = H * 0.5f - 2.5f;
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.segment(ax - 3.5f, ay,        ax,        ay + 4.0f, 1.3f, false);
        canvas.segment(ax,        ay + 4.0f, ax + 3.5f, ay,        1.3f, false);
    }

    // Click → cycle forward; drag up/down → select index directly
    void mouseDown(const visage::MouseEvent& e) override {
        dragStartY_   = e.position.y;
        dragStartIdx_ = index_;
        dragged_      = false;
    }
    void mouseDrag(const visage::MouseEvent& e) override {
        if (options_.empty()) return;
        const int dy    = int(dragStartY_ - e.position.y); // positive = dragged up
        const int delta = dy / 12;                          // 12 px per step
        const int newIdx = std::clamp(dragStartIdx_ + delta, 0, (int)options_.size() - 1);
        if (newIdx != index_) {
            index_   = newIdx;
            dragged_ = true;
            redraw();
            if (onChanged) onChanged(index_);
        }
    }
    void mouseUp(const visage::MouseEvent&) override {
        if (!dragged_ && !options_.empty()) {
            // Plain click with no drag: cycle forward
            index_ = (index_ + 1) % (int)options_.size();
            redraw();
            if (onChanged) onChanged(index_);
        }
        dragged_ = false;
    }
    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit(const visage::MouseEvent&)  override { hovered_ = false; redraw(); }

    // Scroll wheel: cycle options without clicking
    bool mouseWheel(const visage::MouseEvent& e) override {
        if (options_.empty()) return false;
        const int delta = (e.wheel_delta_y > 0.0f) ? 1 : -1;
        const int newIdx = std::clamp(index_ + delta, 0, (int)options_.size() - 1);
        if (newIdx != index_) {
            index_   = newIdx;
            redraw();
            if (onChanged) onChanged(index_);
        }
        return true;
    }

private:
    std::vector<std::string> options_;
    int       index_      = 0;
    bool      hovered_    = false;
    float     dragStartY_ = 0.0f;
    int       dragStartIdx_ = 0;
    bool      dragged_    = false;
    SIDFonts* fonts_      = nullptr;
};

// ============================================================
//  SIDDropdownButton — click to open a popup list of options
//  Same visual as SIDCycleButton but uses visage::PopupMenu
//  instead of cycling on click — better for long option lists.
// ============================================================
class SIDDropdownButton : public visage::Frame {
public:
    std::function<void(int)> onChanged;

    void setOptions(std::vector<std::string> opts) {
        options_ = std::move(opts);
        if (!options_.empty())
            index_ = std::clamp(index_, 0, (int)options_.size() - 1);
        redraw();
    }
    void setIndex(int i) {
        if (!options_.empty())
            index_ = std::clamp(i, 0, (int)options_.size() - 1);
        else
            index_ = i;
        redraw();
    }
    int  getIndex() const { return index_; }
    void setFonts(SIDFonts* f) { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());
        const float r = 3.0f;

        canvas.setColor(hovered_ ? 0xFF0D1E38 : SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(0, 0, W, H, r);
        canvas.setColor(hovered_ ? SIDColors::ACCENT_CYAN_DIM : SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(0, 0, W, H, r, 1.0f);

        // Separator before chevron zone
        const float sepX = W - 16.0f;
        canvas.setColor(SIDColors::BTN_OFF_BORDER);
        canvas.fill(sepX, 2.0f, 1.0f, H - 4.0f);

        // Selected option text
        if (fonts_ && !options_.empty()) {
            canvas.setColor(SIDColors::TEXT_PRIMARY);
            canvas.text(options_[index_], fonts_->label, visage::Font::kLeft,
                        5, 0, sepX - 3.0f, H);
        }

        // Down chevron ▼
        const float ax = W - 9.5f;
        const float ay = H * 0.5f - 2.5f;
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.segment(ax - 3.5f, ay,        ax,        ay + 4.0f, 1.3f, false);
        canvas.segment(ax,        ay + 4.0f, ax + 3.5f, ay,        1.3f, false);
    }

    // Click → open popup menu
    void mouseDown(const visage::MouseEvent&) override {
        if (options_.empty()) return;
        visage::PopupMenu menu;
        for (int i = 0; i < (int)options_.size(); ++i)
            menu.addOption(i, options_[i]).select(i == index_);
        menu.onSelection() += [this](int id) {
            if (id >= 0 && id < (int)options_.size() && id != index_) {
                index_ = id;
                redraw();
                if (onChanged) onChanged(index_);
            }
        };
        menu.show(this);
    }

    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit(const visage::MouseEvent&)  override { hovered_ = false; redraw(); }

    // Scroll wheel: cycle without opening popup
    bool mouseWheel(const visage::MouseEvent& e) override {
        if (options_.empty()) return false;
        const int delta  = (e.wheel_delta_y > 0.0f) ? 1 : -1;
        const int newIdx = std::clamp(index_ + delta, 0, (int)options_.size() - 1);
        if (newIdx != index_) {
            index_ = newIdx;
            redraw();
            if (onChanged) onChanged(index_);
        }
        return true;
    }

private:
    std::vector<std::string> options_;
    int       index_   = 0;
    bool      hovered_ = false;
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
        // Background (self-filling)
        canvas.setColor(SIDColors::SCOPE_BG);
        canvas.roundedRectangle(0, 0, width(), height(), 3.0f);

        // Border outline only
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangleBorder(0, 0, width(), height(), 3.0f, 1.0f);

        // Center line
        const float cy = height() * 0.5f;
        canvas.setColor(SIDColors::SEPARATOR);
        canvas.segment(4, cy, width() - 4, cy, 0.5f, false);

        // Waveform + fill
        if (sampleCount_ > 1) {
            const float xStep = (width() - 8.0f) / float(sampleCount_ - 1);

            // ── Filled area under the waveform line ────────────
            // Draw thin vertical segments from each sample point to the centerline
            canvas.setColor(0x3800C0FF);   // soft cyan fill
            for (int i = 0; i < sampleCount_; ++i) {
                const float x  = 4.0f + float(i) * xStep;
                const float yw = cy - samples_[i] * (cy - 4.0f);
                const float yTop = std::min(yw, cy);
                const float yBot = std::max(yw, cy);
                if (yBot - yTop > 0.5f)
                    canvas.segment(x, yTop, x, yBot, xStep + 0.8f, false);
            }

            // ── Bright waveform line on top ─────────────────────
            canvas.setColor(SIDColors::SCOPE_LINE);
            for (int i = 1; i < sampleCount_; ++i) {
                const float x0 = 4.0f + (i - 1) * xStep;
                const float y0 = cy - samples_[i - 1] * (cy - 4.0f);
                const float x1 = 4.0f + i * xStep;
                const float y1 = cy - samples_[i]     * (cy - 4.0f);
                canvas.segment(x0, y0, x1, y1, 1.5f, false);
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
//  SIDFilterInfoView — filter state info display (top-right scope)
// ============================================================
class SIDFilterInfoView : public visage::Frame {
public:
    // Called from PluginEditor::onRender() every frame
    void setFilterState(const std::string& type, const std::string& slope,
                        float cutoffHz, float resNorm) {
        filterType_  = type;
        filterSlope_ = slope;
        cutoffHz_    = cutoffHz;
        resNorm_     = resNorm;
        redraw();
    }

    void init()       override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());

        // Background + border
        canvas.setColor(SIDColors::SCOPE_BG);
        canvas.roundedRectangle(0, 0, W, H, 3.0f);
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangleBorder(0, 0, W, H, 3.0f, 1.0f);

        if (!fonts_) return;

        // "FILTER" header
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("FILTER", fonts_->label, visage::Font::kLeft, 5, 3, W - 10, 11);

        // ── Left half: large type + slope pill ──────────────────
        const float lW = W * 0.44f;

        canvas.setColor(SIDColors::ACCENT_CYAN_BRIGHT);
        canvas.text(filterType_, fonts_->section, visage::Font::kCenter, 4, 16, lW, 28);

        // Slope pill
        canvas.setColor(0xFF0A2A40);
        canvas.roundedRectangle(6, 47, lW - 8, 17, 3.0f);
        canvas.setColor(SIDColors::ACCENT_CYAN_DIM);
        canvas.roundedRectangleBorder(6, 47, lW - 8, 17, 3.0f, 1.0f);
        canvas.setColor(SIDColors::ACCENT_CYAN);
        canvas.text(filterSlope_, fonts_->button, visage::Font::kCenter, 6, 47, lW - 8, 17);

        // Vertical separator
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(W * 0.47f, 14, 1.0f, H - 18);

        // ── Right half: cutoff + resonance ──────────────────────
        const float rX = W * 0.50f;
        const float rW = W - rX - 5.0f;

        // Cutoff
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("CUT", fonts_->label, visage::Font::kLeft, rX, 4, 28, 11);

        char freqBuf[24];
        if (cutoffHz_ >= 1000.0f)
            std::snprintf(freqBuf, sizeof(freqBuf), "%.1f kHz", cutoffHz_ / 1000.0f);
        else
            std::snprintf(freqBuf, sizeof(freqBuf), "%.0f Hz", cutoffHz_);

        canvas.setColor(SIDColors::TEXT_PRIMARY);
        canvas.text(freqBuf, fonts_->value, visage::Font::kRight, rX, 14, rW, 16);

        // Resonance label + bar
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("RES", fonts_->label, visage::Font::kLeft, rX, 34, 28, 11);

        const float barY = 47.0f, barH = 10.0f;
        canvas.setColor(SIDColors::BG_PANEL_DARK);
        canvas.roundedRectangle(rX, barY, rW, barH, 2.0f);

        const float fillW = rW * std::clamp(resNorm_, 0.0f, 1.0f);
        if (fillW > 1.0f) {
            const uint32_t bloom = (SIDColors::ACCENT_GREEN & 0x00FFFFFFu) | 0x22000000u;
            canvas.setColor(bloom);
            canvas.roundedRectangle(rX - 1, barY - 1, fillW + 2, barH + 2, 2.5f);
            canvas.setColor(SIDColors::ACCENT_GREEN);
            canvas.roundedRectangle(rX, barY, fillW, barH, 2.0f);
        }
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.roundedRectangleBorder(rX, barY, rW, barH, 2.0f, 1.0f);

        // Subtle grid lines
        canvas.setColor(0x08FFFFFF);
        for (int i = 1; i <= 3; ++i) {
            const float gx = 4.0f + (W - 8.0f) * (float(i) / 4.0f);
            canvas.segment(gx, 14, gx, H - 4, 0.5f, false);
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        fonts_ = &fonts_storage_;
    }

    std::string filterType_  = "LP";
    std::string filterSlope_ = "24dB";
    float       cutoffHz_    = 440.0f;
    float       resNorm_     = 0.3f;
    SIDFonts*   fonts_       = nullptr;
    SIDFonts    fonts_storage_;
};

// ============================================================
//  SIDOscillatorPanel — one oscillator section (OSC 1/2/3)
// ============================================================
class SIDOscillatorPanel : public SIDPanelBase {
public:
    SIDKnob  semiKnob, fineKnob, pwKnob, volumeKnob;
    SIDFader attackFader, decayFader, sustainFader, releaseFader;
    SIDOscilloscopeView envelopeDisplay;

    // Waveform selector buttons (7 types: SAW TRI SQR NOI S+T RNG HSW)
    static constexpr int kNumWaveforms = 7;
    SIDWaveButton waveButtons[kNumWaveforms];

    // Called when user clicks a waveform button — passes index 0-5
    std::function<void(int)> onWaveformChanged;

    explicit SIDOscillatorPanel(int oscNum)
        : SIDPanelBase("OSCILLATOR " + std::to_string(oscNum)) {}

    // Set active waveform from outside (e.g. APVTS binding)
    void setWaveform(int w) {
        for (int i = 0; i < kNumWaveforms; ++i)
            waveButtons[i].setState(i == w);
    }
    int getWaveform() const {
        for (int i = 0; i < kNumWaveforms; ++i)
            if (waveButtons[i].getState()) return i;
        return 0;
    }

    void init() override {
        for (auto& b : waveButtons) addChild(&b);
        addChild(&semiKnob);
        addChild(&fineKnob);
        addChild(&pwKnob);
        addChild(&volumeKnob);
        addChild(&attackFader);
        addChild(&decayFader);
        addChild(&sustainFader);
        addChild(&releaseFader);
        addChild(&envelopeDisplay);
        updateFontsAndLayout();

        // Radio button logic: clicking one deselects all others
        for (int i = 0; i < kNumWaveforms; ++i) {
            waveButtons[i].onToggled = [this, i](bool on) {
                if (on) {
                    for (int j = 0; j < kNumWaveforms; ++j)
                        if (j != i) waveButtons[j].setState(false);
                    if (onWaveformChanged) onWaveformChanged(i);
                } else {
                    // Don't allow deselecting the active button —
                    // user must click a different one to change
                    waveButtons[i].setState(true);
                }
            };
        }
        // Default: first waveform selected
        waveButtons[0].setState(true);
    }

    void dpiChanged() override {
        updateFontsAndLayout();
    }

    void resized() override {
        const int w = width();
        const int h = height();

        // Waveform buttons row — taller for pixel-art icons
        const int btnW = (w - 8) / kNumWaveforms;
        for (int i = 0; i < kNumWaveforms; ++i)
            waveButtons[i].setBounds(4 + i * btnW, 22, btnW - 2, 30);

        // Knobs row (shifted down to accommodate taller buttons)
        const int kx = 4, ky = 58, ks = 52;
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
        attackFader.setFonts(&fonts_);   attackFader.setLabel("A");
        attackFader.setActiveColor(SIDColors::ENV_ATTACK);
        decayFader.setFonts(&fonts_);    decayFader.setLabel("D");
        decayFader.setActiveColor(SIDColors::ENV_DECAY);
        sustainFader.setFonts(&fonts_);  sustainFader.setLabel("S");
        sustainFader.setActiveColor(SIDColors::ENV_SUSTAIN);
        releaseFader.setFonts(&fonts_);  releaseFader.setLabel("R");
        releaseFader.setActiveColor(SIDColors::ENV_RELEASE);
        envelopeDisplay.setFonts(&fonts_);

        for (int i = 0; i < kNumWaveforms; ++i)
            waveButtons[i].setWaveType(i);
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDFilterPanel — cutoff + resonance + type + slope + key/vel
// ============================================================
class SIDFilterPanel : public SIDPanelBase {
public:
    SIDKnob         cutoffKnob, resonanceKnob;
    SIDKnob         envAmountKnob;
    SIDKnob         envAttackKnob, envDecayKnob, envSustainKnob, envReleaseKnob;
    SIDCycleButton  typeBtn;    // LP / HP / BP / Notch
    SIDToggleButton slopeBtn;   // 24dB / 12dB
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

    void init() override {
        addChild(&cutoffKnob);
        addChild(&resonanceKnob);
        addChild(&envAmountKnob);
        addChild(&typeBtn);
        addChild(&slopeBtn);
        addChild(&envAttackKnob);
        addChild(&envDecayKnob);
        addChild(&envSustainKnob);
        addChild(&envReleaseKnob);
        addChild(&keyTrackBtn);
        addChild(&velocityBtn);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int W = width();
        // ── Row 1: Large knobs + env amount ─────────────────
        cutoffKnob.setBounds(8, 18, 68, 68);
        resonanceKnob.setBounds(82, 18, 68, 68);
        envAmountKnob.setBounds(160, 24, 52, 52);

        // ── Row 2: Type cycle + Slope toggle ─────────────────
        typeBtn.setBounds(8, 92, 118, 20);
        slopeBtn.setBounds(132, 92, 80, 20);

        // ── Row 3: Filter ENV knobs ───────────────────────────
        const int ek = 36, gap = 4;
        int ex = 8;
        envAttackKnob.setBounds(ex, 118, ek, ek);   ex += ek + gap;
        envDecayKnob.setBounds(ex, 118, ek, ek);    ex += ek + gap;
        envSustainKnob.setBounds(ex, 118, ek, ek);  ex += ek + gap;
        envReleaseKnob.setBounds(ex, 118, ek, ek);

        // ── Row 4: Key track + Velocity ──────────────────────
        keyTrackBtn.setBounds(8,     height() - 26, 96, 20);
        velocityBtn.setBounds(W / 2, height() - 26, 96, 20);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        // "FILTER ENV" section label
        if (fonts_) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("FILTER ENV",
                        fonts_->label, visage::Font::kLeft, 8, 110, width() - 16, 10);
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        fonts_ = &fonts_storage_;
        setFonts(fonts_);
        cutoffKnob.setFonts(fonts_);
        resonanceKnob.setFonts(fonts_);
        envAmountKnob.setFonts(fonts_);   envAmountKnob.setLabel("ENV AMT");
        typeBtn.setFonts(fonts_);
        typeBtn.setOptions({"LP", "HP", "BP", "Notch"});
        slopeBtn.setFonts(fonts_);
        slopeBtn.setLabels("24dB", "12dB");  // on=24dB, off=12dB
        envAttackKnob.setFonts(fonts_);    envAttackKnob.setLabel("A");
        envAttackKnob.setRingColor(SIDColors::ENV_ATTACK);
        envDecayKnob.setFonts(fonts_);     envDecayKnob.setLabel("D");
        envDecayKnob.setRingColor(SIDColors::ENV_DECAY);
        envSustainKnob.setFonts(fonts_);   envSustainKnob.setLabel("S");
        envSustainKnob.setRingColor(SIDColors::ENV_SUSTAIN);
        envReleaseKnob.setFonts(fonts_);   envReleaseKnob.setLabel("R");
        envReleaseKnob.setRingColor(SIDColors::ENV_RELEASE);
        keyTrackBtn.setFonts(fonts_);
        velocityBtn.setFonts(fonts_);
    }

    SIDFonts* fonts_ = nullptr;
    SIDFonts  fonts_storage_;
};

// ============================================================
//  SIDAmplifierPanel — global amp ADSR
// ============================================================
class SIDAmplifierPanel : public SIDPanelBase {
public:
    SIDKnob attackKnob, decayKnob, sustainKnob, releaseKnob, volumeKnob;

    SIDAmplifierPanel() : SIDPanelBase("AMPLIFIER") {}

    void init() override {
        addChild(&attackKnob);
        addChild(&decayKnob);
        addChild(&sustainKnob);
        addChild(&releaseKnob);
        addChild(&volumeKnob);
        updateFonts();
    }
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
        attackKnob.setFonts(&fonts_);   attackKnob.setLabel("ATTACK");
        attackKnob.setRingColor(SIDColors::ENV_ATTACK);
        decayKnob.setFonts(&fonts_);    decayKnob.setLabel("DECAY");
        decayKnob.setRingColor(SIDColors::ENV_DECAY);
        sustainKnob.setFonts(&fonts_);  sustainKnob.setLabel("SUSTAIN");
        sustainKnob.setRingColor(SIDColors::ENV_SUSTAIN);
        releaseKnob.setFonts(&fonts_);  releaseKnob.setLabel("RELEASE");
        releaseKnob.setRingColor(SIDColors::ENV_RELEASE);
        volumeKnob.setFonts(&fonts_);   volumeKnob.setLabel("VOLUME");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDStepCurveEditor — 16-bar Reason Matrix-style step editor
//  Drag bars up/down to set modulation values in -1..+1.
//  Positive = orange, Negative = blue.
//  setEnabled(false) dims and blocks interaction (shown when LFO
//  shape is not STEP).
// ============================================================
class SIDStepCurveEditor : public visage::Frame {
public:
    static constexpr int kSteps = 16;

    // Fired when user adjusts a bar: (step index 0..15, new value -1..+1)
    std::function<void(int, float)> onStepChanged;

    void setValue(int step, float v) {
        if (step < 0 || step >= kSteps) return;
        values_[step] = std::clamp(v, -1.0f, 1.0f);
        redraw();
    }
    float getValue(int step) const {
        return (step >= 0 && step < kSteps) ? values_[step] : 0.0f;
    }

    // Enable/disable interaction and visual state (guard prevents redundant redraws)
    void setEnabled(bool en) {
        if (en == enabled_) return;
        enabled_ = en;
        redraw();
    }

    void draw(visage::Canvas& canvas) override {
        const float W    = float(width());
        const float H    = float(height());
        const float midY = std::round(H * 0.5f);
        const float barW = W / float(kSteps);

        // Background
        canvas.setColor(SIDColors::BG_PANEL_DARK);
        canvas.roundedRectangle(0, 0, W, H, 3.0f);

        // Bars
        for (int i = 0; i < kSteps; ++i) {
            const float v  = values_[i];
            const float bx = i * barW + 1.5f;
            const float bw = std::max(1.0f, barW - 3.0f);

            if (enabled_) {
                if (v > 0.001f) {
                    const float bh = v * midY;
                    canvas.setColor(SIDColors::ENV_ATTACK);   // amber/orange
                    canvas.fill(bx, midY - bh, bw, bh);
                } else if (v < -0.001f) {
                    const float bh = (-v) * midY;
                    canvas.setColor(SIDColors::STEP_ACTIVE);  // blue
                    canvas.fill(bx, midY, bw, bh);
                } else {
                    // Zero: thin centre marker
                    canvas.setColor(SIDColors::BORDER_PANEL);
                    canvas.fill(bx, midY - 1.0f, bw, 2.0f);
                }
            } else {
                // Disabled: flat grey line per bar
                canvas.setColor(SIDColors::TEXT_DIM);
                canvas.fill(bx, midY - 0.5f, bw, 1.0f);
            }
        }

        // Centre line (drawn on top of bars so it's always visible)
        canvas.setColor(enabled_ ? SIDColors::BORDER_PANEL : SIDColors::BORDER_INNER);
        canvas.fill(0, midY - 0.5f, W, 1.0f);

        // Outer border
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.roundedRectangleBorder(0, 0, W, H, 3.0f, 1.0f);
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (!enabled_) return;
        // Compute offset from continuously-updated e.position to local coords.
        // e.relative_position is correct at click time; e.position is always live.
        offsetX_ = e.position.x - e.relative_position.x;
        offsetY_ = e.position.y - e.relative_position.y;
        dragStep_ = getStepAt(float(e.relative_position.x));
        setValueFromY(dragStep_, float(e.relative_position.y));
    }

    void mouseDrag(const visage::MouseEvent& e) override {
        if (!enabled_ || dragStep_ < 0) return;
        // e.position is continuously updated during drag; convert to local coords.
        // Allows painting across bars and adjusting bar height by dragging.
        const float localX = e.position.x - offsetX_;
        const float localY = e.position.y - offsetY_;
        dragStep_ = getStepAt(localX);
        setValueFromY(dragStep_, localY);
    }

    void mouseUp(const visage::MouseEvent&) override { dragStep_ = -1; }

private:
    float values_[kSteps] = {};
    bool  enabled_        = false;
    int   dragStep_       = -1;
    float offsetX_        = 0.0f;  // e.position - e.relative_position at mouseDown
    float offsetY_        = 0.0f;

    int getStepAt(float x) const {
        const float barW = float(width()) / float(kSteps);
        return std::clamp(int(x / barW), 0, kSteps - 1);
    }

    void setValueFromY(int step, float y) {
        if (step < 0 || step >= kSteps) return;
        const float midY = float(height()) * 0.5f;
        const float v    = std::clamp((midY - y) / midY, -1.0f, 1.0f);
        values_[step]    = v;
        redraw();
        if (onStepChanged) onStepChanged(step, v);
    }
};

// ============================================================
//  SIDLFOPanel — LFO shape + rate + sync + retrig + step curve
// ============================================================
class SIDLFOPanel : public SIDPanelBase {
public:
    SIDKnob           rateKnob;
    SIDToggleButton   onBtn;              // LFO enable
    SIDToggleButton   syncBtn, retrigBtn;
    SIDCycleButton    shapeBtn;           // Sine/Tri/Saw/RevSaw/Sqr/S&H/STEP
    SIDCycleButton    syncDivBtn;         // 1/32 1/16 1/8 1/4 1/2 1/1 2/1 4/1
    SIDStepCurveEditor stepEditor;        // visible/interactive only when shape == STEP

    explicit SIDLFOPanel(int lfoNum)
        : SIDPanelBase("LFO " + std::to_string(lfoNum)) {}

    void init() override {
        addChild(&onBtn);
        addChild(&shapeBtn);
        addChild(&rateKnob);
        addChild(&syncBtn);
        addChild(&retrigBtn);
        addChild(&syncDivBtn);
        addChild(&stepEditor);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    // Call from onRender() to switch between free-rate and tempo-sync display.
    // In sync mode: the rate knob is hidden and the division selector is shown.
    // In free mode: the rate knob is shown and the sync-div selector is hidden.
    void setSyncActive(bool syncOn) {
        if (syncOn == syncActive_) return;
        syncActive_ = syncOn;
        resized();
        redraw();
    }

    // Enable/disable the step editor based on current shape selection
    void setStepEditorActive(bool active) {
        stepEditor.setEnabled(active);
    }

    void resized() override {
        const int W = width();
        // ON toggle — top-right corner
        onBtn.setBounds(W - 32, 2, 28, 14);
        // Shape selector
        shapeBtn.setBounds(4, 20, 70, 20);

        // Rate area: rate knob (free) XOR sync-div (sync)
        if (syncActive_) {
            rateKnob.setBounds(0, 0, 0, 0);
            syncDivBtn.setBounds(78, 20, 44, 20);
        } else {
            rateKnob.setBounds(78, 18, 44, 44);
            syncDivBtn.setBounds(0, 0, 0, 0);
        }

        // Sync / Retrig stacked to the right
        syncBtn.setBounds(126, 18, 28, 20);
        retrigBtn.setBounds(126, 42, 28, 20);

        // Step curve editor — bottom strip
        stepEditor.setBounds(4, 66, W - 8, 36);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        if (syncActive_) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("TIME", fonts_.label, visage::Font::kLeft, 78, 11, 44, 9);
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
        onBtn.setFonts(&fonts_);     onBtn.setLabels("ON", "OFF");
        shapeBtn.setFonts(&fonts_);
        shapeBtn.setOptions({"Sine", "Tri", "Saw", "RevSaw", "Sqr", "S&H", "STEP"});
        rateKnob.setFonts(&fonts_);  rateKnob.setLabel("RATE");
        syncBtn.setFonts(&fonts_);   syncBtn.setLabel("SYNC");
        retrigBtn.setFonts(&fonts_); retrigBtn.setLabel("RETRIG");
        syncDivBtn.setFonts(&fonts_);
        syncDivBtn.setOptions({"1/32","1/16","1/8","1/4","1/2","1/1","2/1","4/1"});
        syncDivBtn.setIndex(3); // default 1/4
    }

    SIDFonts fonts_;
    bool     syncActive_ = false;
};

// ============================================================
//  SIDArpPanel — 16-step arp / sequencer
// ============================================================
class SIDArpPanel : public SIDPanelBase {
public:
    static constexpr int kStepCount = 16;
    SIDStepButton   steps[kStepCount];
    SIDToggleButton syncBtn;

    // Top-bar controls
    SIDCycleButton  modeBtn;    // Up / Down / UpDn / Rand / Order
    SIDCycleButton  octaveBtn;  // 1 Oct .. 4 Oct
    SIDCycleButton  gateBtn;    // 10% .. 100%
    SIDCycleButton  swingBtn;   // 0% .. 50%
    SIDCycleButton  tempoBtn;   // 60 .. 200 BPM
    SIDToggleButton arpOnBtn;   // ARP enable toggle

    SIDArpPanel() : SIDPanelBase("ARP / SEQUENCER") {}

    void init() override {
        addChild(&modeBtn);
        addChild(&octaveBtn);
        addChild(&gateBtn);
        addChild(&swingBtn);
        addChild(&tempoBtn);
        addChild(&arpOnBtn);
        addChild(&syncBtn);
        for (auto& s : steps) addChild(&s);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int W = width();

        // ── Top control row (label text drawn in draw()) ──
        // Controls row — y=32, h=20.
        // Order: ON | SYNC | MODE | OCT | GATE | SWING | TEMPO
        // This avoids the arpOnBtn/syncBtn overlap that occurred at x≈290.
        const int bY = 32, bH = 20;
        arpOnBtn.setBounds (4,        bY, 38, bH);  // ON — leftmost, most prominent
        syncBtn.setBounds  (46,       bY, 38, bH);  // SYNC — second
        modeBtn.setBounds  (88,       bY, 54, bH);
        octaveBtn.setBounds(146,      bY, 54, bH);
        gateBtn.setBounds  (204,      bY, 42, bH);
        swingBtn.setBounds (250,      bY, 42, bH);
        tempoBtn.setBounds (296,      bY, W - 300, bH);

        // ── Step buttons ──
        // Fit 16 buttons centred; make them as wide as possible
        const int stepH = 36, gap = 4;
        const int available = W - 8;           // 8px total side padding
        const int stepW = (available - gap * (kStepCount - 1)) / kStepCount;
        int sx = 4;
        for (int i = 0; i < kStepCount; ++i) {
            steps[i].setBounds(sx, 58, stepW, stepH);
            sx += stepW + gap;
        }
    }

    void setPlayingStep(int step) {
        for (int i = 0; i < kStepCount; ++i)
            steps[i].setPlaying(i == step);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        // Small labels above the control buttons
        if (fonts_) {
            struct { int x; int w; const char* txt; } lbls[] = {
                {4,   38, "ON"},
                {46,  38, "SYNC"},
                {88,  54, "MODE"},
                {146, 54, "OCT"},
                {204, 42, "GATE"},
                {250, 42, "SWING"},
                {296, 40, "TEMPO"},
            };
            canvas.setColor(SIDColors::TEXT_LABEL);
            for (auto& l : lbls)
                canvas.text(l.txt, fonts_->label, visage::Font::kCenter,
                            l.x, 20, l.w, 10);
        }
    }

private:
    SIDFonts* fonts_ = nullptr;   // pointer — set from updateFonts()
    SIDFonts  fonts_storage_;

    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        fonts_ = &fonts_storage_;
        setFonts(fonts_);

        // Cycle button options
        modeBtn.setOptions  ({"Up","Down","UpDn","Rand","Order"});
        octaveBtn.setOptions({"1 Oct","2 Oct","3 Oct","4 Oct"});
        gateBtn.setOptions  ({"10%","25%","50%","75%","90%","100%"});
        gateBtn.setIndex(3);   // default 75%
        swingBtn.setOptions ({"0%","20%","33%","50%"});
        tempoBtn.setOptions ({"60","80","100","120","140","160","180","200"});
        tempoBtn.setIndex(4); // default 140

        modeBtn.setFonts  (fonts_);
        octaveBtn.setFonts(fonts_);
        gateBtn.setFonts  (fonts_);
        swingBtn.setFonts (fonts_);
        tempoBtn.setFonts (fonts_);
        arpOnBtn.setFonts (fonts_);
        arpOnBtn.setLabel ("ARP");
        syncBtn.setFonts  (fonts_);
        syncBtn.setLabel("SYNC");

        for (int i = 0; i < kStepCount; ++i) {
            steps[i].setFonts(fonts_);
            steps[i].setNumber(i + 1);
        }
    }
};

// ============================================================
//  SIDEffectsPanel — Chorus / Delay / Reverb
// ============================================================
class SIDEffectsPanel : public SIDPanelBase {
public:
    SIDToggleButton chorusBtn, delayBtn, reverbBtn;
    SIDKnob chorusRate, chorusDepth, chorusMix;
    SIDKnob delayFeedback, delayMix;
    SIDCycleButton delayTimeBtn;  // 1/32, 1/16, 1/8, 1/4, 1/2, 1/1, 2/1
    SIDKnob reverbSize, reverbDamping, reverbMix;

    SIDEffectsPanel() : SIDPanelBase("EFFECTS") {}

    void init() override {
        addChild(&chorusBtn);
        addChild(&chorusRate);
        addChild(&chorusDepth);
        addChild(&chorusMix);
        addChild(&delayBtn);
        addChild(&delayTimeBtn);
        addChild(&delayFeedback);
        addChild(&delayMix);
        addChild(&reverbBtn);
        addChild(&reverbSize);
        addChild(&reverbDamping);
        addChild(&reverbMix);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int ks = 32;   // knob size
        // Row Y: 22 (chorus), 82 (delay), 142 (reverb)
        static constexpr int kY[3] = { 22, 82, 142 };

        // LED toggles — small square left of label
        chorusBtn.setBounds(4,  kY[0] + 2, 14, 14);
        delayBtn.setBounds(4,   kY[1] + 2, 14, 14);
        reverbBtn.setBounds(4,  kY[2] + 2, 14, 14);

        // Knobs aligned right-side
        const int kx0 = 84;
        chorusRate.setBounds(kx0,             kY[0] - 3, ks, ks);
        chorusDepth.setBounds(kx0 + ks + 4,   kY[0] - 3, ks, ks);
        chorusMix.setBounds(kx0 + 2*(ks+4),   kY[0] - 3, ks, ks);

        delayTimeBtn.setBounds(kx0,            kY[1] - 3, 46, 16);   // time division picker
        delayFeedback.setBounds(kx0 + 50,      kY[1] - 3, ks, ks);
        delayMix.setBounds(kx0 + 50 + ks + 4, kY[1] - 3, ks, ks);

        reverbSize.setBounds(kx0,              kY[2] - 3, ks, ks);
        reverbDamping.setBounds(kx0 + ks + 4,  kY[2] - 3, ks, ks);
        reverbMix.setBounds(kx0 + 2*(ks+4),    kY[2] - 3, ks, ks);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        // Effect name labels (drawn next to the LED squares)
        if (fonts_.label.size() > 0) {
            static constexpr int kY[3] = { 22, 82, 142 };
            static constexpr const char* kNames[3] = { "CHORUS", "DELAY", "REVERB" };
            for (int i = 0; i < 3; ++i) {
                canvas.setColor(SIDColors::TEXT_PRIMARY);
                canvas.text(kNames[i], fonts_.button, visage::Font::kLeft,
                            22, kY[i] + 1, 58, 14);
            }
        }

        // Dividers between FX sections
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(4, 68, width() - 8, 1);
        canvas.fill(4, 128, width() - 8, 1);
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
        // LED toggles — just need fonts for potential future text
        chorusBtn.setFonts(&fonts_);
        delayBtn.setFonts(&fonts_);
        reverbBtn.setFonts(&fonts_);
        chorusRate.setFonts(&fonts_);    chorusRate.setLabel("RATE");
        chorusDepth.setFonts(&fonts_);   chorusDepth.setLabel("DEPTH");
        chorusMix.setFonts(&fonts_);     chorusMix.setLabel("MIX");
        delayTimeBtn.setFonts(&fonts_);
        delayTimeBtn.setOptions({"1/32","1/16","1/8","1/4","1/2","1/1","2/1"});
        delayTimeBtn.setIndex(3); // default 1/4 note
        delayFeedback.setFonts(&fonts_); delayFeedback.setLabel("FDBK");
        delayMix.setFonts(&fonts_);      delayMix.setLabel("MIX");
        reverbSize.setFonts(&fonts_);    reverbSize.setLabel("SIZE");
        reverbDamping.setFonts(&fonts_); reverbDamping.setLabel("DAMP");
        reverbMix.setFonts(&fonts_);     reverbMix.setLabel("MIX");
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDModMatrixPanel — 4-slot modulation routing (interactive)
// ============================================================
class SIDModMatrixPanel : public SIDPanelBase {
public:
    static constexpr int kSlots = 4;

    // Public widgets — bound in PluginEditor
    SIDDropdownButton srcBtn[kSlots];   // source selector per row (popup menu)
    SIDKnob           amtKnob[kSlots];  // bipolar amount knob per row
    SIDDropdownButton dstBtn[kSlots];   // destination selector per row (popup menu)

    SIDModMatrixPanel() : SIDPanelBase("MOD MATRIX") {}

    void init() override {
        for (int i = 0; i < kSlots; ++i) {
            addChild(&srcBtn[i]);
            addChild(&amtKnob[i]);
            addChild(&dstBtn[i]);
        }
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        // Columns: [4..83] src | [88..113] knob | [118..width-4] dst
        const int W = width();
        for (int i = 0; i < kSlots; ++i) {
            const int ry = 22 + i * 26;
            srcBtn[i].setBounds(4,       ry, 80, 22);
            amtKnob[i].setBounds(88,     ry - 1, 24, 24);
            dstBtn[i].setBounds(116,     ry, W - 120, 22);
        }
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        // Column header labels (drawn once, above row 0)
        if (fonts_.label.size() > 0) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("SOURCE", fonts_.label, visage::Font::kCenter,  4,  8,  80, 12);
            canvas.text("AMT",    fonts_.label, visage::Font::kCenter,  86, 8,  28, 12);
            canvas.text("DESTINATION", fonts_.label, visage::Font::kCenter, 116, 8, width() - 120, 12);
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
        static const std::vector<std::string> srcOpts = {
            "LFO 1", "LFO 2", "Velocity", "Mod Wheel", "Amp Env", "Key"
        };
        static const std::vector<std::string> dstOpts = {
            "Filter Cut", "OSC1 PW", "OSC2 PW", "OSC3 PW",
            "Amp Level",  "Filter Res", "OSC1 Fine", "OSC2 Fine"
        };
        for (int i = 0; i < kSlots; ++i) {
            srcBtn[i].setFonts(&fonts_); srcBtn[i].setOptions(srcOpts);
            amtKnob[i].setFonts(&fonts_);
            amtKnob[i].setRingColor(SIDColors::ACCENT_CYAN);
            dstBtn[i].setFonts(&fonts_); dstBtn[i].setOptions(dstOpts);
        }
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDMasterPanel — output + limiter + poly + voices
// ============================================================
class SIDMasterPanel : public SIDPanelBase {
public:
    SIDFader        outputFader;
    SIDToggleButton limiterBtn;
    SIDToggleButton polyModeBtn;    // POLY / MONO toggle
    SIDCycleButton  voiceCountBtn;  // Voice count selector

    SIDMasterPanel() : SIDPanelBase("MASTER") {}

    void init() override {
        addChild(&outputFader);
        addChild(&limiterBtn);
        addChild(&polyModeBtn);
        addChild(&voiceCountBtn);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        outputFader.setBounds(60, 20, 28, 180);
        limiterBtn.setBounds(100, 208, 44, 18);
        polyModeBtn.setBounds(4, 248, width() - 8, 20);
        voiceCountBtn.setBounds(4, 293, width() - 8, 20);
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
            canvas.text("LIMITER",    fonts_.label, visage::Font::kLeft, 4, 208, 90, 14);
            canvas.text("MONO / POLY",fonts_.label, visage::Font::kLeft, 4, 232, width() - 8, 14);
            // (polyModeBtn drawn by child — sits at y=248)
            canvas.text("VOICE COUNT",fonts_.label, visage::Font::kLeft, 4, 276, width() - 8, 14);
            // (voiceCountBtn child drawn at y=293)
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
        limiterBtn.setFonts(&fonts_);      limiterBtn.setLabel("ON");
        polyModeBtn.setFonts(&fonts_);     polyModeBtn.setLabels("POLY", "MONO");
        voiceCountBtn.setFonts(&fonts_);
        voiceCountBtn.setOptions({"1","2","4","6","8","10","12","16"});
        voiceCountBtn.setIndex(7);  // default: 16 voices
    }

    SIDFonts fonts_;
};

// ============================================================
//  SIDPresetBar — bottom preset / bank bar
// ============================================================
class SIDPresetBar : public visage::Frame {
public:
    SIDToggleButton saveBtn, saveAsBtn, initBtn;

    // Callbacks for preset navigation (Prev/Next arrows)
    std::function<void()> onPrev, onNext;

    void setPresetName(const std::string& name) { presetName_ = name; redraw(); }
    void setDirty(bool d)                        { dirty_      = d;    redraw(); }

    void init() override {
        addChild(&saveBtn);
        addChild(&saveAsBtn);
        addChild(&initBtn);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int bH = height() - 8, bY = 4;
        saveBtn.setBounds(width() - 186, bY, 52, bH);
        saveAsBtn.setBounds(width() - 130, bY, 62, bH);
        initBtn.setBounds(width() - 64,  bY, 56, bH);
    }

    // Click on the Prev (<) and Next (>) arrows (not child frames — drawn in draw())
    void mouseDown(const visage::MouseEvent& e) override {
        const int bH = height() - 8, bY = 4;
        const int nameX = 66, nameW = 260;
        const int arrowX = nameX + nameW + 4;
        const int nextX  = arrowX + bH + 3;

        const int ex = int(e.relative_position.x), ey = int(e.relative_position.y);
        if (ey >= bY && ey < bY + bH) {
            if (ex >= arrowX && ex < arrowX + bH) { if (onPrev) onPrev(); return; }
            if (ex >= nextX  && ex < nextX  + bH) { if (onNext) onNext(); return; }
        }
    }

    void draw(visage::Canvas& canvas) override {
        // Background strip
        canvas.setColor(SIDColors::BG_PANEL_DARK);
        canvas.fill(0, 0, width(), height());
        // Top border line
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(0, 0, width(), 1);

        if (!fonts_) return;
        const int bH = height() - 8, bY = 4;

        // "PRESET:" label
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("PRESET:", fonts_->label, visage::Font::kLeft, 8, 0, 54, height());

        // Preset name box — shows current preset name + dirty (*) marker
        const int nameX = 66, nameW = 260;
        canvas.setColor(SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(nameX, bY, nameW, bH, 3.0f);
        canvas.setColor(SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(nameX, bY, nameW, bH, 3.0f, 1.0f);

        const bool isInit = presetName_.empty();
        const std::string displayName = isInit ? "-- Init Patch --"
                                                : presetName_ + (dirty_ ? " *" : "");
        canvas.setColor(isInit ? SIDColors::TEXT_DIM : SIDColors::TEXT_PRIMARY);
        canvas.text(displayName, fonts_->label, visage::Font::kLeft,
                    nameX + 6, bY, nameW - 12, bH);

        // Prev (<) / Next (>) navigation arrows
        const int arrowX = nameX + nameW + 4;
        auto drawArrow = [&](int x, const char* lbl) {
            canvas.setColor(SIDColors::BTN_OFF_BG);
            canvas.roundedRectangle(x, bY, bH, bH, 2.0f);
            canvas.setColor(SIDColors::BTN_OFF_BORDER);
            canvas.roundedRectangleBorder(x, bY, bH, bH, 2.0f, 1.0f);
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(lbl, fonts_->button, visage::Font::kCenter, x, bY, bH, bH);
        };
        drawArrow(arrowX,             "<");
        drawArrow(arrowX + bH + 3,    ">");

        // "BANK:" label + dropdown (cosmetic — future feature)
        const int bankLabelX = arrowX + bH * 2 + 12;
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("BANK:", fonts_->label, visage::Font::kLeft, bankLabelX, 0, 44, height());

        const int bankX = bankLabelX + 48, bankW = 148;
        canvas.setColor(SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(bankX, bY, bankW, bH, 3.0f);
        canvas.setColor(SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(bankX, bY, bankW, bH, 3.0f, 1.0f);
        canvas.setColor(SIDColors::TEXT_DIM);
        canvas.text("Factory", fonts_->label, visage::Font::kLeft, bankX + 6, bY, bankW - 24, bH);
        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("v", fonts_->label, visage::Font::kRight, bankX, bY, bankW - 4, bH);

        // Right side: tagline + MOS chip logo
        canvas.setColor(SIDColors::TEXT_DIM);
        canvas.text("SID SYNTHESIZER FOR TRANCE MUSIC", fonts_->label, visage::Font::kRight,
                    width() - 330, 0, 130, height());
        canvas.setColor(SIDColors::ACCENT_CYAN);
        canvas.text("MOS", fonts_->section, visage::Font::kLeft,
                    width() - 196, 0, 40, height());
        canvas.setColor(SIDColors::TEXT_DIM);
        canvas.text("CHIP INSIDE", fonts_->label, visage::Font::kLeft,
                    width() - 196, height() / 2, 40, height() / 2);
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        fonts_ = &fonts_storage_;
        saveBtn.setFonts(fonts_);    saveBtn.setLabel("SAVE");
        saveAsBtn.setFonts(fonts_);  saveAsBtn.setLabel("SAVE AS");
        initBtn.setFonts(fonts_);    initBtn.setLabel("INIT");
    }

    SIDFonts*   fonts_     = nullptr;
    SIDFonts    fonts_storage_;
    std::string presetName_;        // empty = Init state
    bool        dirty_     = false; // true = unsaved changes
};

// ============================================================
//  SIDVoiceModPanel — Unison, Glide, OSC Sync/FM, Drive
// ============================================================
class SIDVoiceModPanel : public SIDPanelBase {
public:
    SIDCycleButton  uniVoicesBtn;   // 1..7
    SIDKnob         uniDetuneKnob;
    SIDKnob         uniSpreadKnob;

    SIDCycleButton  glideModeBtn;   // Off / Auto / Always
    SIDKnob         glideTimeKnob;

    SIDToggleButton oscSyncBtn;
    SIDKnob         fmAmtKnob;

    SIDCycleButton  driveTypeBtn;   // Tube / Digital / Crush / SIDGrit
    SIDKnob         driveAmtKnob;

    SIDVoiceModPanel() : SIDPanelBase("VOICE MOD") {}

    void init() override {
        addChild(&uniVoicesBtn);
        addChild(&uniDetuneKnob);
        addChild(&uniSpreadKnob);
        addChild(&glideModeBtn);
        addChild(&glideTimeKnob);
        addChild(&oscSyncBtn);
        addChild(&fmAmtKnob);
        addChild(&driveTypeBtn);
        addChild(&driveAmtKnob);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int W = width();
        // Group layout: 2 rows of knobs + mode buttons
        // Each group: label(10) + button+knobs(~70px)

        // ── UNISON (y=22..100) ─────────────────────────────
        uniVoicesBtn.setBounds(4,  34, 86, 20);
        uniDetuneKnob.setBounds(94, 22, W/2 - 97, 72);
        uniSpreadKnob.setBounds(W/2 + 1, 22, W/2 - 5, 72);

        // ── GLIDE (y=104..176) ────────────────────────────
        glideModeBtn.setBounds(4,  116, 86, 20);
        glideTimeKnob.setBounds(94, 104, W - 98, 72);

        // ── OSC MOD (y=182..254) ──────────────────────────
        oscSyncBtn.setBounds(4,  194, 86, 20);
        fmAmtKnob.setBounds(94, 182, W - 98, 72);

        // ── DRIVE (y=260..308) ────────────────────────────
        driveTypeBtn.setBounds(4,  272, 86, 20);
        driveAmtKnob.setBounds(94, 258, W - 98, 44);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        if (!fonts_) return;
        const float W = float(width());

        // Section labels
        auto lbl = [&](const char* t, int y) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(t, fonts_->label, visage::Font::kLeft, 4, y, 90, 10);
        };
        lbl("UNISON",  22);
        lbl("GLIDE",   104);
        lbl("OSC MOD", 182);
        lbl("DRIVE",   260);

        // Horizontal dividers between groups
        canvas.setColor(SIDColors::BORDER_INNER);
        for (int dy : {100, 178, 256}) {
            canvas.fill(4, dy, W - 8, 1);
        }
    }

private:
    SIDFonts  fonts_storage_;
    SIDFonts* fonts_ = nullptr;

    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        fonts_ = &fonts_storage_;
        setFonts(fonts_);

        // Cycle buttons
        uniVoicesBtn.setFonts(fonts_);
        uniVoicesBtn.setOptions({"1","2","3","4","5","6","7","8","9","10","11","12","13","14","15","16"});

        glideModeBtn.setFonts(fonts_);
        glideModeBtn.setOptions({"OFF","AUTO","ON"});

        driveTypeBtn.setFonts(fonts_);
        driveTypeBtn.setOptions({"TUBE","DIGITAL","CRUSH","GRIT"});

        // Toggle
        oscSyncBtn.setFonts(fonts_);
        oscSyncBtn.setLabel("SYNC");

        // Knobs
        uniDetuneKnob.setFonts(fonts_); uniDetuneKnob.setLabel("DETUNE");
        uniDetuneKnob.setRingColor(SIDColors::ACCENT_CYAN);
        uniSpreadKnob.setFonts(fonts_); uniSpreadKnob.setLabel("SPREAD");
        uniSpreadKnob.setRingColor(SIDColors::ACCENT_CYAN);
        glideTimeKnob.setFonts(fonts_); glideTimeKnob.setLabel("TIME");
        glideTimeKnob.setRingColor(0xFFFFAA00);
        fmAmtKnob.setFonts(fonts_);     fmAmtKnob.setLabel("FM AMT");
        fmAmtKnob.setRingColor(0xFF00FF7F);
        driveAmtKnob.setFonts(fonts_);  driveAmtKnob.setLabel("AMOUNT");
        driveAmtKnob.setRingColor(0xFFFF4400);
    }
};

// ============================================================
//  SIDMacroPanel — 4 large macro knobs (Brightness/Motion/Space/Drive)
// ============================================================
class SIDMacroPanel : public SIDPanelBase {
public:
    SIDKnob macro1, macro2, macro3, macro4;

    SIDMacroPanel() : SIDPanelBase("MACROS") {}

    void init() override {
        addChild(&macro1);
        addChild(&macro2);
        addChild(&macro3);
        addChild(&macro4);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int W = width(), H = height();
        const int kw = (W - 8) / 4 - 2;
        const int kh = H - 22;
        macro1.setBounds(4,              22, kw, kh);
        macro2.setBounds(4 + kw + 2,     22, kw, kh);
        macro3.setBounds(4 + 2*(kw+2),   22, kw, kh);
        macro4.setBounds(4 + 3*(kw+2),   22, kw, kh);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
    }

private:
    SIDFonts  fonts_storage_;

    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        setFonts(&fonts_storage_);
        macro1.setFonts(&fonts_storage_); macro1.setLabel("BRIGHT"); macro1.setLarge(true);
        macro1.setRingColor(0xFF2E8FD4);  macro1.setValue(0.5f);
        macro2.setFonts(&fonts_storage_); macro2.setLabel("MOTION"); macro2.setLarge(true);
        macro2.setRingColor(0xFF00FF7F);  macro2.setValue(0.5f);
        macro3.setFonts(&fonts_storage_); macro3.setLabel("SPACE");  macro3.setLarge(true);
        macro3.setRingColor(0xFFFFAA00);  macro3.setValue(0.5f);
        macro4.setFonts(&fonts_storage_); macro4.setLabel("DRIVE");  macro4.setLarge(true);
        macro4.setRingColor(0xFFFF4400);  macro4.setValue(0.0f);
    }
};

// ============================================================
//  SIDTranceGatePanel — 16-step volume gate sequencer
// ============================================================
class SIDTranceGatePanel : public SIDPanelBase {
public:
    static constexpr int kStepCount = 16;
    SIDStepButton   steps[kStepCount];
    SIDToggleButton gateOnBtn;
    SIDCycleButton  gateSwingBtn;

    SIDTranceGatePanel() : SIDPanelBase("TRANCE GATE") {}

    void init() override {
        addChild(&gateOnBtn);
        addChild(&gateSwingBtn);
        for (auto& s : steps) addChild(&s);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int W = width();
        const int bY = 32, bH = 20;
        gateOnBtn.setBounds  (4,  bY, 44, bH);
        gateSwingBtn.setBounds(52, bY, 72, bH);

        // 16 step buttons
        const int stepH = 36, gap = 3;
        const int available = W - 8;
        const int stepW = (available - gap * (kStepCount - 1)) / kStepCount;
        int sx = 4;
        for (int i = 0; i < kStepCount; ++i) {
            steps[i].setBounds(sx, 58, stepW, stepH);
            sx += stepW + gap;
        }
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        if (!fonts_) return;

        // Section labels
        struct { int x; int w; const char* t; } lbls[] = {
            {4,  44, "ON"},
            {52, 72, "SWING"},
        };
        canvas.setColor(SIDColors::TEXT_LABEL);
        for (auto& l : lbls)
            canvas.text(l.t, fonts_->label, visage::Font::kCenter, l.x, 20, l.w, 10);

        // Visual bar separating controls from steps
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(4, 56, float(width()) - 8, 1);
    }

private:
    SIDFonts  fonts_storage_;
    SIDFonts* fonts_ = nullptr;

    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            fonts_storage_.init(dpi,
                reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf),
                BinaryData::LatoRegular_ttfSize);
        }
        fonts_ = &fonts_storage_;
        setFonts(fonts_);

        gateOnBtn.setFonts(fonts_);    gateOnBtn.setLabel("GATE");
        gateSwingBtn.setFonts(fonts_); gateSwingBtn.setOptions({"0%","20%","33%","50%"});

        for (int i = 0; i < kStepCount; ++i) {
            steps[i].setFonts(fonts_);
            steps[i].setNumber(i + 1);
            steps[i].setState(true);  // default: all steps active
        }
    }
};

// ============================================================
//  SIDMainView — root frame, assembles all panels
// ============================================================
class SIDMainView : public visage::Frame {
public:
    // Top scopes
    SIDOscilloscopeView scope1, scope2, scope3;
    SIDFilterInfoView   filterScope;   // filter state info (not audio scope)

    // Oscillator panels
    SIDOscillatorPanel osc1 { 1 };
    SIDOscillatorPanel osc2 { 2 };
    SIDOscillatorPanel osc3 { 3 };

    // Filter / Amp / Master
    SIDFilterPanel    filterPanel;
    SIDAmplifierPanel ampPanel;
    SIDMasterPanel    masterPanel;

    // Bottom row
    SIDModMatrixPanel    modMatrix;
    SIDEffectsPanel      effectsPanel;
    SIDLFOPanel          lfo1 { 1 };
    SIDLFOPanel          lfo2 { 2 };
    SIDArpPanel          arpPanel;
    SIDTranceGatePanel   gatePanel;
    SIDMacroPanel        macroPanel;

    // Voice mod (middle section, right of master)
    SIDVoiceModPanel     voiceModPanel;

    // Preset bar
    SIDPresetBar         presetBar;

    void init() override {
        // CRITICAL: sets initialized_ = true so addChild() will call child->init()
        Frame::init();
        updateFonts();
        // Children are added in PluginEditor::addAllFrames() after init
    }

    void dpiChanged() override {
        updateFonts();
    }

    void resized() override {
        layoutAll();
    }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width());

        // ── Window background ──────────────────────────────────
        canvas.setColor(SIDColors::BG_VOID);
        canvas.fill(0, 0, width(), height());

        // ── Top info bar background (scopes + logo + branding) ─
        canvas.setColor(0xFF080F1C);
        canvas.roundedRectangle(4, 4, W - 8, 122, 5.0f);
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangleBorder(4, 4, W - 8, 122, 5.0f, 1.0f);

        // ── Commodore logo — circle with italic "C" (matches HTML template) ──
        {
            // Logo cell background
            canvas.setColor(0xFF050D18);
            canvas.roundedRectangle(10, 8, 76, 114, 5.0f);
            canvas.setColor(0xFF1A3A6B);
            canvas.roundedRectangleBorder(10, 8, 76, 114, 5.0f, 1.0f);

            // Circle border (the iconic Commodore badge shape)
            const float cx = 48.0f, cy = 56.0f, cr = 26.0f;
            // Draw circle as a filled ring: outer fill then inner cut
            canvas.setColor(0xFF1E6DA8);
            canvas.circle(cx - cr, cy - cr, cr * 2.0f);               // filled circle
            canvas.setColor(0xFF050D18);
            canvas.circle(cx - cr + 3.0f, cy - cr + 3.0f, (cr-3.0f)*2.0f); // punch out centre

            if (font_title_.size() > 0) {
                // Italic bold "C" inside the ring — the classic Commodore look
                canvas.setColor(0xFF2E8FD4);
                canvas.text("C", font_title_, visage::Font::kCenter, 10, 30, 76, 50);

                // "commodore" caption below circle
                canvas.setColor(0xFF3A6A8C);
                canvas.text("commodore", font_label_small_, visage::Font::kCenter, 10, 95, 76, 16);
            }

            // Vertical separator right of logo
            canvas.setColor(SIDColors::BORDER_INNER);
            canvas.fill(88, 8, 1, 114);
        }

        // ── Plugin branding — top right (matches HTML template) ───────────
        if (font_large_.size() > 0) {
            // Large "SID" — template blue #2e8fd4, right-aligned
            canvas.setColor(0xFF2E8FD4);
            canvas.text("SID", font_large_, visage::Font::kRight,
                        870, 6, 314, 62);
            // "TRANCE MACHINE" subtitle
            canvas.setColor(0xFF4A8AAB);
            canvas.text("TRANCE MACHINE", font_label_, visage::Font::kRight,
                        870, 68, 314, 18);
            // "C-MOS" model tag in lighter blue
            canvas.setColor(0xFF6AACCB);
            canvas.text("C-MOS", font_label_small_, visage::Font::kRight,
                        870, 88, 314, 14);

            // Branding separator
            canvas.setColor(SIDColors::BORDER_INNER);
            canvas.fill(868, 8, 1, 114);
        }
    }

private:
    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        if (BinaryData::LatoRegular_ttfSize > 0) {
            const auto* fd = reinterpret_cast<const unsigned char*>(BinaryData::LatoRegular_ttf);
            const int   fs = BinaryData::LatoRegular_ttfSize;
            font_large_      = visage::Font(48.0f, fd, fs, dpi);  // "SID" big text
            font_title_      = visage::Font(26.0f, fd, fs, dpi);  // C= logo
            font_label_      = visage::Font(13.0f, fd, fs, dpi);  // TRANCE MACHINE
            font_label_small_= visage::Font(9.0f,  fd, fs, dpi);  // G-MOS / commodore
        }
    }

    void layoutAll() {
        const int W = width();
        const int H = height();
        const int PAD = 8, GAP = 4;

        // ── Preset bar (bottom strip, fixed height) ─────────
        const int presetBarH = 36;
        presetBar.setBounds(0, H - presetBarH, W, presetBarH);

        // ── Header row — 3 OSC scopes + filter info display ───
        const int scopeY  = PAD + 2;
        const int scopeH  = 114;
        const int scopeL  = 106;
        const int scopeR  = 926;
        const int filterW = 210;
        const int oscScopeW = (scopeR - scopeL - filterW - 3 * GAP) / 3;
        scope1.setBounds(scopeL,                       scopeY, oscScopeW, scopeH);
        scope2.setBounds(scopeL + oscScopeW + GAP,     scopeY, oscScopeW, scopeH);
        scope3.setBounds(scopeL + 2*(oscScopeW + GAP), scopeY, oscScopeW, scopeH);
        filterScope.setBounds(scopeR - filterW,        scopeY, filterW,   scopeH);

        // ── Middle section ──────────────────────────────────
        const int midY = scopeY + scopeH + GAP;
        const int midH = 308;
        const int oscW = 200;
        osc1.setBounds(PAD,               midY, oscW, midH);
        osc2.setBounds(PAD + oscW + GAP,  midY, oscW, midH);
        osc3.setBounds(PAD + 2*(oscW+GAP),midY, oscW, midH);

        const int filterX = PAD + 3*(oscW+GAP);
        filterPanel.setBounds(filterX, midY, 220, 200);
        ampPanel.setBounds(filterX, midY + 204, 220, 104);
        masterPanel.setBounds(filterX + 224, midY, 148, midH);

        // Voice mod panel: right of master, fills remaining middle width
        const int voiceModX = filterX + 224 + 148 + GAP;
        const int voiceModW = W - voiceModX - PAD;
        voiceModPanel.setBounds(voiceModX, midY, std::max(voiceModW, 160), midH);

        // ── Bottom section ─────────────────────────────────
        const int botY = midY + midH + GAP;
        const int botH = H - presetBarH - PAD - botY;
        modMatrix.setBounds(PAD, botY, 296, botH);
        effectsPanel.setBounds(PAD + 300, botY, 196, botH);

        const int lfoX = PAD + 300 + 200;
        lfo1.setBounds(lfoX, botY, 160, 108);
        lfo2.setBounds(lfoX + 164, botY, 160, 108);

        // Macro panel: to the right of lfo2, full height of upper row
        const int macroX = lfoX + 328;
        const int macroW = W - macroX - PAD;
        macroPanel.setBounds(macroX, botY, macroW, 108);

        // Lower row: arpPanel + gatePanel side by side
        const int arpY    = botY + 112;
        const int arpRowH = botH - 112;
        const int arpW    = (W - lfoX - PAD - GAP) / 2;
        arpPanel.setBounds(lfoX,          arpY, arpW, arpRowH);
        gatePanel.setBounds(lfoX + arpW + GAP, arpY, arpW, arpRowH);

        (void)H;
    }

    visage::Font font_large_;
    visage::Font font_title_;
    visage::Font font_label_;
    visage::Font font_label_small_;
};
