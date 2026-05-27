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
    // ── Blue → purple neon palette (matched to TranceSID Gui Design.png) ──
    // The PNG uses a deep navy void with cyan-to-magenta gradient edges.
    // All widget chrome should sit on top of the PNG and tint with these.

    // Backgrounds  (panel interior — semi-transparent to let the PNG show through)
    static constexpr uint32_t BG_VOID         = 0xFF05060A;   // user-spec'd void
    static constexpr uint32_t BG_PANEL        = 0xC00E1424;   // 75% alpha so PNG edges peek
    static constexpr uint32_t BG_PANEL_DARK   = 0xD00B1020;   // a bit darker for inner wells
    static constexpr uint32_t BG_SECTION      = 0xC0121A2E;

    // Borders — pick up the neon edge accents from the design
    static constexpr uint32_t BORDER_PANEL    = 0xFF1E90FF;   // glow blue
    static constexpr uint32_t BORDER_INNER    = 0xFF2E3A60;
    static constexpr uint32_t SEPARATOR       = 0xFF1A2244;

    // Accent cyan (left edge of the gradient)
    static constexpr uint32_t ACCENT_CYAN_BRIGHT = 0xFF00C8FF;   // cyan-accent
    static constexpr uint32_t ACCENT_CYAN        = 0xFF1E90FF;   // glow blue
    static constexpr uint32_t ACCENT_CYAN_DIM    = 0xFF1A4080;
    static constexpr uint32_t ACCENT_CYAN_GLOW   = 0x6600C8FF;

    // Accent magenta (right edge of the gradient — replaces green-only resonance)
    static constexpr uint32_t ACCENT_GREEN    = 0xFFC84DE0;   // magenta-accent
    static constexpr uint32_t ACCENT_GREEN_DIM= 0xFF66208C;
    static constexpr uint32_t ACCENT_MAGENTA  = 0xFFC84DE0;
    static constexpr uint32_t ACCENT_PURPLE   = 0xFF9B4DFF;   // glow purple
    static constexpr uint32_t ACCENT_PURPLE_DIM   = 0xFF5226A0;
    static constexpr uint32_t ACCENT_PURPLE_GLOW  = 0x669B4DFF;

    // Knob / fader
    static constexpr uint32_t KNOB_BODY       = 0xFF141A2C;
    static constexpr uint32_t KNOB_BODY_LARGE = 0xFF0E1424;
    static constexpr uint32_t KNOB_INDICATOR  = 0xFFD8DEE9;   // user-spec silver
    static constexpr uint32_t FADER_TRACK     = 0xFF0B1020;
    static constexpr uint32_t FADER_THUMB     = 0xFF1E2A50;
    static constexpr uint32_t FADER_THUMB_ACT = 0xFF2C3A78;

    // Step sequencer
    static constexpr uint32_t STEP_ACTIVE     = 0xFF1E90FF;   // blue (active)
    static constexpr uint32_t STEP_INACTIVE   = 0xFF0B1228;
    static constexpr uint32_t STEP_PLAYING    = 0xFF9B4DFF;   // purple (playing)
    static constexpr uint32_t STEP_BORDER     = 0xFF1E3A78;

    // Text
    static constexpr uint32_t TEXT_PRIMARY    = 0xFFD8DEE9;   // user-spec silver
    static constexpr uint32_t TEXT_LABEL      = 0xFF98A6C8;
    static constexpr uint32_t TEXT_DIM        = 0xFF52608A;
    static constexpr uint32_t TEXT_ACCENT     = 0xFF00C8FF;

    // Buttons
    static constexpr uint32_t BTN_OFF_BG      = 0xC00E1424;
    static constexpr uint32_t BTN_OFF_BORDER  = 0xFF2A3A66;
    static constexpr uint32_t BTN_ON_BG       = 0xFF1E2A60;
    static constexpr uint32_t BTN_ON_BORDER   = 0xFF1E90FF;
    static constexpr uint32_t BTN_ON_TEXT     = 0xFF00C8FF;
    static constexpr uint32_t LED_ON          = 0xFFC84DE0;   // magenta LED
    static constexpr uint32_t LED_OFF         = 0xFF2C1240;

    // Scope
    static constexpr uint32_t SCOPE_LINE      = 0xFF00C8FF;
    static constexpr uint32_t SCOPE_FILL      = 0x3300C8FF;
    static constexpr uint32_t SCOPE_BG        = 0xC005060A;

    // ADSR envelope colors (kept distinct for envelope display)
    static constexpr uint32_t ENV_ATTACK      = 0xFFFF7700;
    static constexpr uint32_t ENV_DECAY       = 0xFFFFCC00;
    static constexpr uint32_t ENV_SUSTAIN     = 0xFF1E90FF;
    static constexpr uint32_t ENV_RELEASE     = 0xFF9B4DFF;
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

    // Chromeless mode — the panel doesn't draw its own background fill,
    // border, header bar or title text.  Used when the editor's
    // background image already prints all of that, so layering the
    // code-drawn version on top would produce the doubled-chrome look.
    // Subclasses must check chromeless_ in their own draw() override
    // and skip their internal section labels / dividers too.
    void setChromeless(bool on) { chromeless_ = on; redraw(); }
    bool chromeless() const     { return chromeless_; }

protected:
    void drawPanelBase(visage::Canvas& canvas) {
        // When the editor is rendering on top of the new skeleton JPG,
        // all panel chrome (background, border, header text, separators)
        // is part of the printed background — drawing it again here
        // produces a doubled-frame look.  Skip in chromeless mode.
        if (chromeless_) return;

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
    bool        chromeless_ = false;
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
    // When false, the parameter name is never drawn (still shown as the
    // hover-value readout).  Used in the new skeleton layout where the
    // JPG already prints every knob's label underneath the knob slot.
    void setShowLabel(bool s)     { showLabel_ = s; }

    // Custom value→string formatter for the hover readout.  Default is
    // "<pct> %"; callers (PluginEditor binding helpers) can install a
    // format that prints Hz, dB, semitones, etc. based on the parameter
    // this knob is bound to.
    void setValueFormatter(std::function<std::string(float)> f) {
        formatter_ = std::move(f);
    }

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

        // ── Arc value fill — bloom in single accent + blue→purple gradient core ──
        const float valueSweep = kSweep * value_;
        if (valueSweep > 0.002f) {
            // Bloom glow stays single-colour cyan so the gradient core sits on top
            // of a soft halo (matches the neon-edge look of the design PNG).
            const uint32_t bloomCol = (SIDColors::ACCENT_CYAN_BRIGHT & 0x00FFFFFFu) | 0x30000000u;
            drawArcApprox(canvas, cx, cy, arcR, kStartAngle, valueSweep, bloomCol, trackWidth + 6.0f);
            const uint32_t midCol = (SIDColors::ACCENT_PURPLE & 0x00FFFFFFu) | 0x60000000u;
            drawArcApprox(canvas, cx, cy, arcR, kStartAngle, valueSweep, midCol,   trackWidth + 2.0f);
            // Core: linearly interpolated blue → purple along the value sweep.
            drawArcGradient(canvas, cx, cy, arcR, kStartAngle, valueSweep,
                            SIDColors::ACCENT_CYAN_BRIGHT,   // start = blue
                            SIDColors::ACCENT_PURPLE,        // end   = purple
                            trackWidth);
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

        // ── Label / value readout ──────────────────────────────
        // Hover always shows the value (so the user can read precise
        // settings).  The non-hover label is only drawn when
        // showLabel_ is true — in the new skeleton layout the JPG
        // already prints every parameter name under its knob slot, so
        // we leave it false to avoid doubled labels.
        if (fonts_) {
            std::string text;
            if (hovered_) {
                text = formatter_ ? formatter_(value_) : defaultFormat(value_);
            } else if (showLabel_) {
                text = label_;
            }
            if (!text.empty()) {
                canvas.setColor(hovered_ ? SIDColors::TEXT_ACCENT : SIDColors::TEXT_LABEL);
                canvas.text(text, fonts_->label, visage::Font::kCenter,
                            0, static_cast<int>(height()) - 14, width(), 12);
            }
        }
    }

    static std::string defaultFormat(float v) {
        char buf[16];
        snprintf(buf, sizeof(buf), "%.0f%%", v * 100.0f);
        return std::string(buf);
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

    // Same as drawArcApprox but linearly interpolates the colour from
    // colorStart at sweep=0 to colorEnd at sweep=full.  Used for the
    // blue→purple neon ring on knobs that matches the PNG's accent gradient.
    static uint32_t lerpColor(uint32_t c0, uint32_t c1, float t) {
        t = std::clamp(t, 0.0f, 1.0f);
        auto chan = [](uint32_t c, int shift) -> int { return int((c >> shift) & 0xFFu); };
        const int a0 = chan(c0, 24), r0 = chan(c0, 16), g0 = chan(c0, 8), b0 = chan(c0, 0);
        const int a1 = chan(c1, 24), r1 = chan(c1, 16), g1 = chan(c1, 8), b1 = chan(c1, 0);
        const int aa = int(std::round(a0 + (a1 - a0) * t));
        const int rr = int(std::round(r0 + (r1 - r0) * t));
        const int gg = int(std::round(g0 + (g1 - g0) * t));
        const int bb = int(std::round(b0 + (b1 - b0) * t));
        return (uint32_t(aa) << 24) | (uint32_t(rr) << 16) | (uint32_t(gg) << 8) | uint32_t(bb);
    }

    void drawArcGradient(visage::Canvas& canvas, float cx, float cy, float r,
                         float startAngle, float sweep,
                         uint32_t colorStart, uint32_t colorEnd, float lineWidth) {
        const int N = 16;
        for (int i = 0; i < N; ++i) {
            const float t0 = float(i)     / N;
            const float t1 = float(i + 1) / N;
            // Use the segment's midpoint colour so endpoint joints look smooth.
            canvas.setColor(lerpColor(colorStart, colorEnd, (t0 + t1) * 0.5f));
            const float a0 = startAngle + sweep * t0;
            const float a1 = startAngle + sweep * t1;
            const float x0 = cx + r * std::cos(a0);
            const float y0 = cy + r * std::sin(a0);
            const float x1 = cx + r * std::cos(a1);
            const float y1 = cy + r * std::sin(a1);
            canvas.segment(x0, y0, x1, y1, lineWidth, false);
        }
    }

    float       value_          = 0.5f;
    uint32_t    ringColor_      = SIDColors::ACCENT_CYAN;
    // Default OFF — the new skeleton JPG prints every knob's name into
    // the background.  Callers that want a code-drawn label (e.g. for a
    // standalone knob outside the skeleton) call setShowLabel(true).
    bool        showLabel_      = false;
    std::function<std::string(float)> formatter_;
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
//  SIDChipSwitch — 2-segment chip-model selector (6581 / 8580).
//
//  Click on a segment to select that chip.  Bound to the chip_model
//  AudioParameterChoice in PluginEditor::bindAllParameters() via the same
//  onChanged-callback pattern every other Visage widget in this plugin uses
//  (sets the parameter via setValueNotifyingHost; preset load /
//  automation refresh comes through stateJustLoaded → bindAllParameters,
//  which calls setIndex() on this widget).  Functionally equivalent to a
//  juce::AudioProcessorValueTreeState::ButtonAttachment.
// ============================================================
class SIDChipSwitch : public visage::Frame {
public:
    std::function<void(int)> onChanged;   // 0 = 6581, 1 = 8580

    void setIndex(int idx) {
        idx = std::clamp(idx, 0, 1);
        if (idx != index_) { index_ = idx; redraw(); }
    }
    int  getIndex() const           { return index_; }
    void setFonts(SIDFonts* f)      { fonts_ = f; }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());
        const float r = 3.0f;
        const float segW = W * 0.5f;

        // Inactive base
        canvas.setColor(SIDColors::BTN_OFF_BG);
        canvas.roundedRectangle(0, 0, W, H, r);

        // Active segment fill (left = 6581, right = 8580)
        const float ax = (index_ == 0) ? 0.0f : segW;
        canvas.setColor(SIDColors::BTN_ON_BG);
        canvas.roundedRectangle(ax, 0, segW, H, r);
        // Subtle top highlight on the active half
        canvas.setColor(0x1AFFFFFF);
        canvas.roundedRectangle(ax + 1, 1, segW - 2, H * 0.45f, r - 1.0f);

        // Outer border + inner divider
        canvas.setColor(SIDColors::BTN_OFF_BORDER);
        canvas.roundedRectangleBorder(0, 0, W, H, r, 1.0f);
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(segW - 0.5f, 2.0f, 1.0f, H - 4.0f);

        // Labels
        if (fonts_) {
            canvas.setColor(index_ == 0 ? SIDColors::BTN_ON_TEXT : SIDColors::TEXT_DIM);
            canvas.text("6581", fonts_->button, visage::Font::kCenter,
                        0, 0, int(segW), int(H));
            canvas.setColor(index_ == 1 ? SIDColors::BTN_ON_TEXT : SIDColors::TEXT_DIM);
            canvas.text("8580", fonts_->button, visage::Font::kCenter,
                        int(segW), 0, int(segW), int(H));
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        const float W = float(width());
        const int newIdx = (e.position.x < W * 0.5f) ? 0 : 1;
        if (newIdx != index_) {
            index_ = newIdx;
            redraw();
            if (onChanged) onChanged(index_);
        }
    }

    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit (const visage::MouseEvent&) override { hovered_ = false; redraw(); }

private:
    int       index_   = 1;     // default = 8580 (matches chip_model default)
    bool      hovered_ = false;
    SIDFonts* fonts_   = nullptr;
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
//  SIDChipClickArea — invisible click region overlaid on a SID chip
//  badge painted into the background PNG.  Hosts the chip-model
//  selector now that the standalone SIDChipSwitch widget is gone.
//
//  When `active_` is true we draw a soft cyan→purple glow ring
//  around the chip so the user can see which model is selected;
//  when inactive we draw nothing and the PNG art shows through.
// ============================================================
// ============================================================
//  SIDOutputMeter — peak/VU meter for the top-header master area.
//  Two vertical bars (L/R) with a neon gradient.  Driven by the
//  editor calling setLevels() each render frame from the
//  processor's outPeakL / outPeakR atomics.
// ============================================================
class SIDOutputMeter : public visage::Frame {
public:
    void setLevels(float l, float r) {
        if (std::abs(l - levelL_) > 1e-4f || std::abs(r - levelR_) > 1e-4f) {
            levelL_ = std::clamp(l, 0.0f, 1.5f);
            levelR_ = std::clamp(r, 0.0f, 1.5f);
            redraw();
        }
    }

    void draw(visage::Canvas& canvas) override {
        const int W = width(), H = height();
        if (W < 4 || H < 8) return;

        const int barGap = 2;
        const int barW   = std::max(3, (W - 3 * barGap) / 2);
        const int barH   = H - 4;
        const int barY   = 2;
        const int xL     = (W - 2 * barW - barGap) / 2;
        const int xR     = xL + barW + barGap;

        auto drawBar = [&](int x, float lvl) {
            // Track background
            canvas.setColor(SIDColors::BG_PANEL_DARK);
            canvas.roundedRectangle(x, barY, barW, barH, 2.0f);
            canvas.setColor(SIDColors::BORDER_INNER);
            canvas.roundedRectangleBorder(x, barY, barW, barH, 2.0f, 1.0f);

            // Active fill — bottom-up, blue → purple → magenta (clip warn)
            const float clamped = std::clamp(lvl, 0.0f, 1.2f);
            const int fillH = int(std::round(clamped / 1.2f * barH));
            if (fillH < 1) return;
            const int fy0 = barY + barH - fillH;

            // Split fill into 3 colour zones so the user can read the level:
            //   bottom 60% : cyan
            //   60–95%     : purple
            //   95%+       : magenta (clip warning)
            const int seg1 = int(barH * 0.6f);     // up to 0.6 = "comfortable"
            const int seg2 = int(barH * 0.95f);    // up to 0.95 = "hot"

            auto paintSegment = [&](int y0, int y1, uint32_t col) {
                if (y1 <= y0) return;
                canvas.setColor(col);
                canvas.fill(x + 1, y0, barW - 2, y1 - y0);
            };

            const int topY = barY + barH;          // bar bottom
            const int yA = std::max(fy0, topY - seg1);
            const int yB = std::max(fy0, topY - seg2);
            paintSegment(yA, topY, SIDColors::ACCENT_CYAN_BRIGHT);
            paintSegment(yB, yA,   SIDColors::ACCENT_PURPLE);
            // Clip zone
            if (clamped > 0.95f) {
                paintSegment(fy0, yB, SIDColors::LED_ON);   // magenta
            }
        };

        drawBar(xL, levelL_);
        drawBar(xR, levelR_);
    }

private:
    float levelL_ = 0.0f;
    float levelR_ = 0.0f;
};

class SIDChipClickArea : public visage::Frame {
public:
    std::function<void()> onClicked;

    void setActive(bool on) { if (on != active_) { active_ = on; redraw(); } }
    bool getActive() const  { return active_; }

    void draw(visage::Canvas& canvas) override {
        const float W = float(width()), H = float(height());
        if (active_) {
            // Outer halo
            canvas.setColor((SIDColors::ACCENT_CYAN_BRIGHT & 0x00FFFFFFu) | 0x33000000u);
            canvas.roundedRectangle(-2, -2, W + 4, H + 4, 8.0f);
            // Inner crisp ring
            canvas.setColor(SIDColors::ACCENT_CYAN_BRIGHT);
            canvas.roundedRectangleBorder(1, 1, W - 2, H - 2, 6.0f, 2.0f);
            // Faint purple secondary ring
            canvas.setColor((SIDColors::ACCENT_PURPLE & 0x00FFFFFFu) | 0x55000000u);
            canvas.roundedRectangleBorder(0, 0, W, H, 7.0f, 1.0f);
        } else if (hovered_) {
            canvas.setColor((SIDColors::ACCENT_CYAN_BRIGHT & 0x00FFFFFFu) | 0x22000000u);
            canvas.roundedRectangleBorder(0, 0, W, H, 6.0f, 1.0f);
        }
        // Otherwise: draw nothing so the painted chip badge underneath shows.
    }

    void mouseDown(const visage::MouseEvent&) override {
        if (onClicked) onClicked();
    }
    void mouseEnter(const visage::MouseEvent&) override { hovered_ = true;  redraw(); }
    void mouseExit (const visage::MouseEvent&) override { hovered_ = false; redraw(); }

private:
    bool active_  = false;
    bool hovered_ = false;
};

// ============================================================
//  SIDPopupOverlay — single full-canvas popup that every dropdown
//  widget goes through.  Replaces visage::PopupMenu (whose internal
//  PopupMenuFrame has no mouseDown/mouseUp of its own, so clicks
//  outside the list silently vanish instead of dismissing the popup).
//
//  One instance lives on SIDMainView at the top of the z-order.  When
//  shown it intercepts every click in the editor and either picks an
//  option or hides itself — so click-on-button-again, click-outside,
//  and pick-option all reliably close the popup.
// ============================================================
class SIDPopupOverlay : public visage::Frame {
public:
    // Lightweight font setter — overlay only needs one label font, so we
    // store a visage::Font by value (no SIDFonts dependency).  Called from
    // SIDMainView::updateFonts() with the same Lato 13 face used for labels.
    void setFont(visage::Font f) { font_ = std::move(f); }

    void init() override { setVisible(false); }

    // Open the popup with `opts`, the current selected index, and a
    // callback for the picked index.  `anchorWindowX/Y` is the source
    // button's top-left in window-relative coords (call
    // source->positionInWindow() and pass that in); we draw the list
    // box directly below the button.
    void open(std::vector<std::string> opts, int currentIdx,
              int anchorWindowX, int anchorWindowY, int anchorH, int anchorW,
              std::function<void(int)> onPicked) {
        options_   = std::move(opts);
        current_   = currentIdx;
        hover_     = -1;
        onPicked_  = std::move(onPicked);

        const int rows = int(options_.size());
        const int W = std::max(anchorW, 110);
        const int H = std::max(1, rows) * kRowH + 6;
        // Default: below the button.  If it would overflow the bottom of
        // the editor, flip above.
        int x = anchorWindowX;
        int y = anchorWindowY + anchorH + 2;
        if (y + H > height()) y = anchorWindowY - H - 2;
        if (x + W > width())  x = width() - W - 4;
        if (x < 4) x = 4;
        if (y < 4) y = 4;
        boxX_ = x; boxY_ = y; boxW_ = W; boxH_ = H;

        setVisible(true);
        setOnTop(true);
        redraw();
    }

    void close() {
        if (!isVisible()) return;
        setVisible(false);
        options_.clear();
        onPicked_ = nullptr;
        current_ = -1;
        hover_ = -1;
        redraw();
    }

    bool isOpen() const { return isVisible(); }

    void draw(visage::Canvas& canvas) override {
        if (!isVisible() || options_.empty()) return;

        // Translucent backdrop dims the editor so the popup pops out.
        canvas.setColor(0x55000000);
        canvas.fill(0, 0, width(), height());

        // Box
        canvas.setColor(SIDColors::BG_PANEL_DARK);
        canvas.roundedRectangle(boxX_, boxY_, boxW_, boxH_, 4.0f);
        canvas.setColor(SIDColors::BORDER_PANEL);
        canvas.roundedRectangleBorder(boxX_, boxY_, boxW_, boxH_, 4.0f, 1.0f);

        // Rows
        for (int i = 0; i < (int)options_.size(); ++i) {
            const int ry = boxY_ + 3 + i * kRowH;
            if (i == hover_) {
                canvas.setColor(SIDColors::BTN_ON_BG);
                canvas.roundedRectangle(boxX_ + 2, ry, boxW_ - 4, kRowH, 2.0f);
            } else if (i == current_) {
                canvas.setColor((SIDColors::ACCENT_CYAN_BRIGHT & 0x00FFFFFFu) | 0x22000000u);
                canvas.roundedRectangle(boxX_ + 2, ry, boxW_ - 4, kRowH, 2.0f);
            }
            if (font_.size() > 0) {
                canvas.setColor(i == current_ ? SIDColors::TEXT_ACCENT : SIDColors::TEXT_PRIMARY);
                canvas.text(options_[i], font_, visage::Font::kLeft,
                            boxX_ + 8, ry, boxW_ - 16, kRowH);
            }
        }
    }

    void mouseDown(const visage::MouseEvent& e) override {
        if (!isVisible() || options_.empty()) { close(); return; }
        const int x = int(e.position.x), y = int(e.position.y);
        if (x >= boxX_ && x < boxX_ + boxW_ && y >= boxY_ + 3 && y < boxY_ + boxH_ - 3) {
            const int idx = (y - boxY_ - 3) / kRowH;
            if (idx >= 0 && idx < (int)options_.size()) {
                auto cb = onPicked_;          // copy: close() resets it
                const int picked = idx;
                close();
                if (cb) cb(picked);
                return;
            }
        }
        // Click anywhere else: cancel
        close();
    }

    void mouseMove(const visage::MouseEvent& e) override {
        if (!isVisible() || options_.empty()) return;
        const int x = int(e.position.x), y = int(e.position.y);
        int h = -1;
        if (x >= boxX_ && x < boxX_ + boxW_ && y >= boxY_ + 3 && y < boxY_ + boxH_ - 3)
            h = (y - boxY_ - 3) / kRowH;
        if (h >= (int)options_.size()) h = -1;
        if (h != hover_) { hover_ = h; redraw(); }
    }

private:
    static constexpr int kRowH = 22;
    std::vector<std::string> options_;
    int current_ = -1;
    int hover_   = -1;
    int boxX_ = 0, boxY_ = 0, boxW_ = 0, boxH_ = 0;
    std::function<void(int)> onPicked_;
    visage::Font font_;
};

// ============================================================
//  SIDCycleButton — click to step through labeled options
// ============================================================
class SIDCycleButton : public visage::Frame {
public:
    std::function<void(int)> onChanged;

    // Pointer to the shared popup overlay (set externally by SIDMainView).
    void setPopupOverlay(SIDPopupOverlay* p) { popupOverlay_ = p; }

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

    // Click → open popup menu (true dropdown).
    // Drag up/down → fine-select index directly without opening the popup.
    // Wheel → cycle one step.
    // Click again while popup is open → toggle-close (suppress reopening).
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
        if (!dragged_ && !options_.empty() && popupOverlay_ != nullptr) {
            // If the overlay is already open, treat this click as a
            // toggle-close — the overlay itself will hide on its next
            // mouseDown anyway, but closing here keeps the click feel
            // consistent (single click toggles).
            if (popupOverlay_->isOpen()) {
                popupOverlay_->close();
                return;
            }
            const auto pos = positionInWindow();
            popupOverlay_->open(options_, index_,
                                int(pos.x), int(pos.y), height(), width(),
                                [this](int picked) {
                                    if (picked >= 0 && picked < (int)options_.size() && picked != index_) {
                                        index_ = picked;
                                        redraw();
                                        if (onChanged) onChanged(index_);
                                    }
                                });
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
    SIDPopupOverlay* popupOverlay_ = nullptr;
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

    // Click → open the shared SIDPopupOverlay (set via setPopupOverlay).
    // Click again while open → toggle-close.  The overlay handles
    // click-outside dismissal reliably (visage::PopupMenu does not).
    void setPopupOverlay(SIDPopupOverlay* p) { popupOverlay_ = p; }

    void mouseDown(const visage::MouseEvent&) override {
        if (options_.empty() || popupOverlay_ == nullptr) return;
        if (popupOverlay_->isOpen()) {
            popupOverlay_->close();
            return;
        }
        const auto pos = positionInWindow();
        popupOverlay_->open(options_, index_,
                            int(pos.x), int(pos.y), height(), width(),
                            [this](int picked) {
                                if (picked >= 0 && picked < (int)options_.size() && picked != index_) {
                                    index_ = picked;
                                    redraw();
                                    if (onChanged) onChanged(index_);
                                }
                            });
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
    int       index_      = 0;
    bool      hovered_    = false;
    SIDFonts* fonts_      = nullptr;
    SIDPopupOverlay* popupOverlay_ = nullptr;
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
    // ADSR — knobs (matched style across OSC / Amp / Filter Env after the
    // uniform-style rework).  Used to be SIDFader; the per-knob hover
    // value readout works the same way and the rest of the synth uses knobs.
    SIDKnob  attackKnob, decayKnob, sustainKnob, releaseKnob;

    // SUPERSAW mode controls — JP-8000-style multi-saw, independent of the
    // wave selector above.  superBtn toggles the mode on/off; voicesBtn
    // picks 7 / 9 / 11 voices; detuneKnob (0-1) drives the exponential
    // spread curve; mixKnob (0-1) is the centre↔sides balance using the
    // classic JP-8000 mix curve.
    SIDToggleButton superBtn;
    SIDCycleButton  superVoicesBtn;
    SIDKnob         superDetuneKnob;
    SIDKnob         superMixKnob;

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
        addChild(&attackKnob);
        addChild(&decayKnob);
        addChild(&sustainKnob);
        addChild(&releaseKnob);
        addChild(&superBtn);
        addChild(&superVoicesBtn);
        addChild(&superDetuneKnob);
        addChild(&superMixKnob);
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

        // ADSR knobs — match the style used by Amp + Filter Env.  4 knobs
        // centred in the panel; size scales with the panel width so they
        // stay readable even on narrower OSC fields.
        const int ek   = std::clamp((w - 32) / 4, 38, 48);
        const int egap = 4;
        const int totEnv = 4 * ek + 3 * egap;
        const int eX = std::max(4, (w - totEnv) / 2);
        const int eY = 116;
        attackKnob.setBounds (eX,                       eY, ek, ek);
        decayKnob.setBounds  (eX + (ek + egap),         eY, ek, ek);
        sustainKnob.setBounds(eX + 2 * (ek + egap),     eY, ek, ek);
        releaseKnob.setBounds(eX + 3 * (ek + egap),     eY, ek, ek);

        // SUPERSAW row — fits between ADSR (ends ~y=160) and the envelope
        // display (was y=206).  4 controls in one row.  Layout adapts to
        // the panel width: toggle + voice picker take fixed-ish space on
        // the left, the two knobs take the remaining width on the right.
        const int sY      = 174;                       // top of SUPER row
        const int sToggleW = 56;
        const int sVoicesW = 44;
        const int sKnob    = std::clamp((w - sToggleW - sVoicesW - 16) / 2 - 4, 28, 38);
        const int sToggleH = 18;
        superBtn.setBounds      (6,                                       sY + 4, sToggleW, sToggleH);
        superVoicesBtn.setBounds(6 + sToggleW + 4,                        sY + 4, sVoicesW, sToggleH);
        const int sKnobY = sY - 4;
        const int sKnobX1 = 6 + sToggleW + 4 + sVoicesW + 6;
        const int sKnobX2 = sKnobX1 + sKnob + 4;
        superDetuneKnob.setBounds(sKnobX1, sKnobY, sKnob, sKnob);
        superMixKnob.setBounds   (sKnobX2, sKnobY, sKnob, sKnob);

        // Envelope display + volume knob — pushed down a bit to make room
        // for the supersaw row above.
        envelopeDisplay.setBounds(4, 220, w - 62, 46);
        volumeKnob.setBounds(w - 58, 220, ks, ks);
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
        attackKnob.setFonts(&fonts_);    attackKnob.setLabel("A");
        attackKnob.setRingColor(SIDColors::ENV_ATTACK);
        decayKnob.setFonts(&fonts_);     decayKnob.setLabel("D");
        decayKnob.setRingColor(SIDColors::ENV_DECAY);
        sustainKnob.setFonts(&fonts_);   sustainKnob.setLabel("S");
        sustainKnob.setRingColor(SIDColors::ENV_SUSTAIN);
        releaseKnob.setFonts(&fonts_);   releaseKnob.setLabel("R");
        releaseKnob.setRingColor(SIDColors::ENV_RELEASE);
        // Supersaw row
        superBtn.setFonts(&fonts_);          superBtn.setLabel("SUPER");
        superVoicesBtn.setFonts(&fonts_);    superVoicesBtn.setOptions({"7", "9", "11"});
        superDetuneKnob.setFonts(&fonts_);   superDetuneKnob.setLabel("DTN");
        superDetuneKnob.setRingColor(SIDColors::ACCENT_CYAN_BRIGHT);
        superMixKnob.setFonts(&fonts_);      superMixKnob.setLabel("MIX");
        superMixKnob.setRingColor(SIDColors::ACCENT_PURPLE);
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
        // The Filter field is ~188 effective px in the PNG-mapped layout.
        // Every row below now fits inside that width with a small margin,
        // and the spacing between Cutoff → Resonance is identical to
        // Resonance → Env Amount (matches the requested layout).
        // ── Row 1: Cutoff, Resonance, Env Amount (uniform size + gap) ─
        const int ks  = 58;       // shared knob size for the top row
        const int gap = 4;
        const int row1Y = 18;
        const int row1X = 4;
        cutoffKnob.setBounds   (row1X,                          row1Y, ks, ks);
        resonanceKnob.setBounds(row1X + (ks + gap),             row1Y, ks, ks);
        envAmountKnob.setBounds(row1X + 2 * (ks + gap),         row1Y, ks, ks);

        // ── Row 2: Type cycle + Slope toggle ─────────────────
        typeBtn.setBounds (4,   92, 104, 20);
        slopeBtn.setBounds(112, 92, 72,  20);

        // ── Row 3: Filter ENV knobs ───────────────────────────
        // Bumped 36 → 44 (much more readable), still fits the ~188-px
        // effective field width: 4 × 44 + 3 × 4 = 188.  Centred for safety
        // in case host scaling drifts the panel width slightly.
        const int ek = 44;
        const int egap = 4;
        const int totEnv = 4 * ek + 3 * egap;            // 188
        int ex = std::max(2, (W - totEnv) / 2);
        envAttackKnob.setBounds (ex, 122, ek, ek);  ex += ek + egap;
        envDecayKnob.setBounds  (ex, 122, ek, ek);  ex += ek + egap;
        envSustainKnob.setBounds(ex, 122, ek, ek);  ex += ek + egap;
        envReleaseKnob.setBounds(ex, 122, ek, ek);

        // ── Row 4: Key track + Velocity ──────────────────────
        const int bw = 84;
        keyTrackBtn.setBounds(4,           height() - 26, bw, 20);
        velocityBtn.setBounds(W - 4 - bw,  height() - 26, bw, 20);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        // "FILTER ENV" section label only when this panel is drawing its
        // own chrome — in chromeless mode (the new skeleton) the JPG
        // already prints every label.
        if (!chromeless_ && fonts_) {
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
        // Amp field is ~199 effective px wide and is now ~150 tall (the
        // master strip that used to share this column is now ≈56 tall).
        // Bump knob size from 36 → 40 — fits with gap=2.  Vertically
        // centre the row inside the available height so labels have
        // breathing room.
        const int W   = width();
        const int H   = height();
        const int ks  = 40;
        const int gap = 2;
        const int total = 5 * ks + 4 * gap;                  // 208 — barely over
        int x = std::max(2, (W - total) / 2);
        // If even tight gap doesn't fit, fall back to 38 + gap 2.
        int useKs = ks;
        if (total > W - 4) { useKs = 38; }
        const int useTotal = 5 * useKs + 4 * gap;
        x = std::max(2, (W - useTotal) / 2);
        // Knobs include a label below — leave ~18 px of label space.
        const int y = std::max(22, (H - useKs - 18) / 2 + 6);
        attackKnob.setBounds (x, y, useKs, useKs); x += useKs + gap;
        decayKnob.setBounds  (x, y, useKs, useKs); x += useKs + gap;
        sustainKnob.setBounds(x, y, useKs, useKs); x += useKs + gap;
        releaseKnob.setBounds(x, y, useKs, useKs); x += useKs + gap;
        volumeKnob.setBounds (x, y, useKs, useKs);
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

    // Visage MouseEvent semantics (from visage_app/window_event_handler.cpp):
    //   e.position          = local coords relative to the frame that received
    //                         mouseDown, continuously updated during drag.
    //   e.relative_position = MOVEMENT DELTA since the last mouse event (≈0 at
    //                         mouseDown, small per-frame deltas during drag).
    //                         NOT a position — using it as one always selects
    //                         bar 0 on click, which was the original bug.
    //   e.window_position   = absolute window coordinates.
    // Therefore: use e.position everywhere for step-editor hit testing.
    void mouseDown(const visage::MouseEvent& e) override {
        if (!enabled_) return;
        dragStep_ = getStepAt(float(e.position.x));
        setValueFromY(dragStep_, float(e.position.y));
    }

    void mouseDrag(const visage::MouseEvent& e) override {
        if (!enabled_ || dragStep_ < 0) return;
        // Continuously hit-test the current local position so dragging across
        // bars paints them, and dragging vertically adjusts the height.
        dragStep_ = getStepAt(float(e.position.x));
        setValueFromY(dragStep_, float(e.position.y));
    }

    void mouseUp(const visage::MouseEvent&) override { dragStep_ = -1; }

private:
    float values_[kSteps] = {};
    bool  enabled_        = false;
    int   dragStep_       = -1;

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
        syncDivBtn.setOptions({
            "1/32","1/16","1/8","1/4","1/2","1/1","2/1","4/1",   // straight
            "1/4T","1/8T","1/16T",                                // triplets
            "1/4D","1/8D"                                          // dotted
        });
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
        // ARP section labels — only when drawing own chrome.  In
        // chromeless mode the JPG already prints them.
        if (!chromeless_ && fonts_) {
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
        // The panel now lives in the narrow left vertical strip (~111 px
        // wide, ~325 px tall after PNG-mapping).  The old horizontal layout
        // (knobs at x=84+) ran off the right edge.  Stack the three FX
        // sections vertically with three knobs centred in each section.
        const int W = width(), H = height();
        const int titleArea = 22;                         // top area for panel title
        const int sectionH  = (H - titleArea) / 3;
        const int ks   = 28;
        const int kgap = 4;
        const int row3W = 3 * ks + 2 * kgap;              // 92
        const int rowX  = std::max(4, (W - row3W) / 2);

        auto placeSection = [&](int idx,
                                SIDToggleButton& led,
                                visage::Frame& a, visage::Frame& b, visage::Frame& c) {
            const int y0 = titleArea + idx * sectionH;
            led.setBounds(6, y0 + 6, 12, 12);
            const int ry = y0 + 24;
            a.setBounds(rowX,                    ry, ks, ks);
            b.setBounds(rowX + (ks + kgap),      ry, ks, ks);
            c.setBounds(rowX + 2 * (ks + kgap),  ry, ks, ks);
        };

        placeSection(0, chorusBtn, chorusRate,    chorusDepth,   chorusMix);

        // Delay row: time-picker (smaller than a knob) + FDBK + MIX
        {
            const int y0 = titleArea + 1 * sectionH;
            delayBtn.setBounds(6, y0 + 6, 12, 12);
            const int ry = y0 + 24;
            const int pickerW = ks;                       // match knob width
            const int pickerH = 16;
            const int pickerY = ry + (ks - pickerH) / 2;  // vertically centred
            delayTimeBtn.setBounds(rowX,                            pickerY, pickerW, pickerH);
            delayFeedback.setBounds(rowX + (ks + kgap),             ry,      ks, ks);
            delayMix.setBounds     (rowX + 2 * (ks + kgap),         ry,      ks, ks);
        }

        placeSection(2, reverbBtn, reverbSize,    reverbDamping, reverbMix);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        // Per-section name labels + dividers — only when drawing own
        // chrome.  In chromeless mode the JPG already prints them.
        if (!chromeless_ && fonts_.label.size() > 0) {
            const int W = width(), H = height();
            const int titleArea = 22;
            const int sectionH  = (H - titleArea) / 3;
            static constexpr const char* kNames[3] = { "CHORUS", "DELAY", "REVERB" };
            canvas.setColor(SIDColors::BORDER_INNER);
            for (int i = 1; i < 3; ++i)
                canvas.fill(4, titleArea + i * sectionH - 1, W - 8, 1);
            for (int i = 0; i < 3; ++i) {
                const int y0 = titleArea + i * sectionH;
                canvas.setColor(SIDColors::TEXT_PRIMARY);
                canvas.text(kNames[i], fonts_.button, visage::Font::kLeft,
                            22, y0 + 4, W - 26, 14);
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
        // Column header labels — only when own chrome is drawn.
        if (!chromeless_ && fonts_.label.size() > 0) {
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
// SIDMasterPanel — slim horizontal strip with limiter / poly / voice-count.
// The output fader was removed; master volume is now the rotary at the
// top-right (next to the TranceSID logo).  This panel is rendered below
// the amplifier panel and is intentionally short so the amp can use the
// reclaimed vertical space for bigger ADSR knobs.
class SIDMasterPanel : public SIDPanelBase {
public:
    SIDToggleButton limiterBtn;
    SIDToggleButton polyModeBtn;    // POLY / MONO toggle
    SIDCycleButton  voiceCountBtn;  // Voice count selector

    SIDMasterPanel() : SIDPanelBase("MASTER") {}

    void init() override {
        addChild(&limiterBtn);
        addChild(&polyModeBtn);
        addChild(&voiceCountBtn);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        // Three controls in a single horizontal row.  Labels are drawn
        // in draw() above each control.  Layout adapts to the panel
        // width so the controls share the space evenly.
        const int W = width();
        const int rowY = 28;
        const int rowH = 18;
        const int gap  = 6;
        const int colW = (W - 2 * 4 - 2 * gap) / 3;
        const int x0 = 4;
        const int x1 = x0 + colW + gap;
        const int x2 = x1 + colW + gap;
        limiterBtn.setBounds  (x0, rowY, colW, rowH);
        polyModeBtn.setBounds (x1, rowY, colW, rowH);
        voiceCountBtn.setBounds(x2, rowY, colW, rowH);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        // Master section labels — only when own chrome is drawn.
        if (!chromeless_ && fonts_.label.size() > 0) {
            const int W = width();
            const int gap  = 6;
            const int colW = (W - 2 * 4 - 2 * gap) / 3;
            const int labelY = 14;
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text("LIMITER", fonts_.label, visage::Font::kCenter,
                        4,                    labelY, colW, 12);
            canvas.text("MONO/POLY", fonts_.label, visage::Font::kCenter,
                        4 + colW + gap,       labelY, colW, 12);
            canvas.text("VOICES",  fonts_.label, visage::Font::kCenter,
                        4 + 2*(colW + gap),   labelY, colW, 12);
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
    // chipSwitch moved out — it now lives at the top of the editor as a
    // separate top-level child of SIDMainView so it can sit directly over
    // the two chip badges drawn into the PNG background.

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

    // Click on the Prev (<) and Next (>) arrows (drawn in draw(), not child
    // frames).  IMPORTANT: e.relative_position is the per-event mouse MOVEMENT
    // delta in Visage (≈0 at mouseDown), not a local coordinate — using it
    // here would always read (0,0) and miss every arrow.  e.position is the
    // local coord relative to this frame and is what we want.
    void mouseDown(const visage::MouseEvent& e) override {
        const int bH = height() - 8, bY = 4;
        const int nameX = 66, nameW = 260;
        const int arrowX = nameX + nameW + 4;
        const int nextX  = arrowX + bH + 3;

        const int ex = int(e.position.x), ey = int(e.position.y);
        if (ey >= bY && ey < bY + bH) {
            if (ex >= arrowX && ex < arrowX + bH) { if (onPrev) onPrev(); return; }
            if (ex >= nextX  && ex < nextX  + bH) { if (onNext) onNext(); return; }
        }
    }

    // Chromeless mode — when the editor's skeleton background already
    // prints the PRESET slot, label, and < / > arrows, draw NOTHING here
    // except the actual preset name text inside the slot.  setChromeless()
    // is invoked from SIDMainView::init() in the new layout.
    void setChromeless(bool on) { chromeless_ = on; redraw(); }

    void draw(visage::Canvas& canvas) override {
        if (!fonts_) return;

        if (chromeless_) {
            // Skeleton draws everything (background, label, slot border,
            // arrows, bank text).  We only paint the preset name inside
            // the printed slot — centred horizontally so it fits.
            const bool isInit = presetName_.empty();
            const std::string displayName = isInit ? "-- Init Patch --"
                                                   : presetName_ + (dirty_ ? " *" : "");
            canvas.setColor(isInit ? SIDColors::TEXT_DIM : SIDColors::TEXT_PRIMARY);
            // Name slot in the JPG is roughly the centre 60 % of the bar's
            // width; align there so the text reads inside the printed box.
            const int margin = std::max(40, int(width()) / 6);
            canvas.text(displayName, fonts_->label, visage::Font::kCenter,
                        margin, 0, int(width()) - 2 * margin, int(height()));
            return;
        }

        // Legacy chrome-drawing path (kept for the fallback when no
        // skeleton image is loaded).
        canvas.setColor(SIDColors::BG_PANEL_DARK);
        canvas.fill(0, 0, width(), height());
        canvas.setColor(SIDColors::BORDER_INNER);
        canvas.fill(0, 0, width(), 1);

        const int bH = height() - 8, bY = 4;

        canvas.setColor(SIDColors::TEXT_LABEL);
        canvas.text("PRESET:", fonts_->label, visage::Font::kLeft, 8, 0, 54, height());

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
    bool        chromeless_ = false;
};

// ============================================================
//  SIDVoiceModPanel — Unison, Glide, OSC Sync/FM, Drive
// ============================================================
class SIDVoiceModPanel : public SIDPanelBase {
public:
    SIDCycleButton  uniVoicesBtn;   // 1..16
    SIDKnob         uniDetuneKnob;
    SIDKnob         uniSpreadKnob;

    SIDCycleButton  glideModeBtn;   // Off / Auto / Always
    SIDKnob         glideTimeKnob;

    SIDToggleButton oscSyncBtn;
    SIDKnob         fmAmtKnob;

    SIDCycleButton  driveTypeBtn;   // Tube / Digital / Crush / SIDGrit
    SIDKnob         driveAmtKnob;

    // STEREO ENGINE — new section: random-phase toggle + per-OSC pan knobs
    // + per-OSC Haas knobs.  All 7 controls share the new STEREO column.
    SIDToggleButton randPhaseBtn;
    SIDKnob         oscPanKnob[3];
    SIDKnob         oscHaasKnob[3];

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
        addChild(&randPhaseBtn);
        for (auto& k : oscPanKnob)  addChild(&k);
        for (auto& k : oscHaasKnob) addChild(&k);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        // The Voice Mod field is a SHORT WIDE strip (~414 × 101 effective).
        // The previous vertical layout (controls down to y=308) was getting
        // clipped — most users only ever saw the UNISON row.  Rebuild as
        // five horizontal sections side-by-side: every control is now
        // visible within the field's actual bounds.
        const int W = width(), H = height();
        const int labelH = 14;            // top row reserved for section names
        const int rowY   = labelH + 6;    // controls start here
        const int rowH   = H - rowY - 4;  // remaining vertical space

        // Section x-divisions — relative weights so we adapt to host scaling.
        // UNISON (110) | GLIDE (62) | OSC MOD (62) | DRIVE (62) | STEREO (118)
        const int wUni    = std::max(96,  W * 110 / 414);
        const int wGlide  = std::max(54,  W *  62 / 414);
        const int wOscMod = std::max(54,  W *  62 / 414);
        const int wDrive  = std::max(54,  W *  62 / 414);
        const int wStereo = W - wUni - wGlide - wOscMod - wDrive - 16;  // remainder

        const int xUni    = 4;
        const int xGlide  = xUni    + wUni    + 4;
        const int xOscMod = xGlide  + wGlide  + 4;
        const int xDrive  = xOscMod + wOscMod + 4;
        const int xStereo = xDrive  + wDrive  + 4;

        // UNISON — voices cycle + Detune + Spread knobs (2 small).
        const int kU = std::min(rowH, (wUni - 38) / 2);
        uniVoicesBtn.setBounds(xUni,           rowY,      32,        18);
        uniDetuneKnob.setBounds(xUni + 36,     rowY,      kU,        kU);
        uniSpreadKnob.setBounds(xUni + 36 + kU + 2, rowY, kU,        kU);

        // GLIDE — mode cycle + time knob.
        const int kG = std::min(rowH, wGlide - 4);
        glideModeBtn.setBounds(xGlide,                    rowY + rowH - 18, wGlide, 18);
        glideTimeKnob.setBounds(xGlide + (wGlide - kG) / 2, rowY,            kG, rowH - 22);

        // OSC MOD — sync toggle + FM amount knob.
        const int kM = std::min(rowH, wOscMod - 4);
        oscSyncBtn.setBounds(xOscMod,                       rowY + rowH - 18, wOscMod, 18);
        fmAmtKnob.setBounds(xOscMod + (wOscMod - kM) / 2,   rowY,             kM, rowH - 22);

        // DRIVE — type cycle + amount knob.
        const int kD = std::min(rowH, wDrive - 4);
        driveTypeBtn.setBounds(xDrive,                     rowY + rowH - 18, wDrive, 18);
        driveAmtKnob.setBounds(xDrive + (wDrive - kD) / 2, rowY,             kD, rowH - 22);

        // STEREO — RND PHASE toggle on top, then 3 PAN + 3 HAAS knobs in
        // two rows of three.  Knobs are small (20-26 px) but legible.
        const int sToggleH = 16;
        randPhaseBtn.setBounds(xStereo, rowY, wStereo, sToggleH);
        const int sRowAY  = rowY + sToggleH + 2;
        const int sRemH   = rowH - sToggleH - 2;
        const int sRowH   = sRemH / 2;
        const int kS      = std::max(18, std::min(sRowH, (wStereo - 8) / 3));
        const int sGap    = std::max(2, (wStereo - 3 * kS) / 4);
        for (int i = 0; i < 3; ++i) {
            const int sx = xStereo + sGap + i * (kS + sGap);
            oscPanKnob[i].setBounds(sx, sRowAY,                  kS, kS);
            oscHaasKnob[i].setBounds(sx, sRowAY + sRowH,         kS, kS);
        }
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);
        if (!fonts_) return;
        const int W = width();

        // Recompute section x positions for the labels (same math as resized()).
        const int wUni    = std::max(96,  W * 110 / 414);
        const int wGlide  = std::max(54,  W *  62 / 414);
        const int wOscMod = std::max(54,  W *  62 / 414);
        const int wDrive  = std::max(54,  W *  62 / 414);
        const int wStereo = W - wUni - wGlide - wOscMod - wDrive - 16;

        const int xUni    = 4;
        const int xGlide  = xUni    + wUni    + 4;
        const int xOscMod = xGlide  + wGlide  + 4;
        const int xDrive  = xOscMod + wOscMod + 4;
        const int xStereo = xDrive  + wDrive  + 4;

        auto sectionLbl = [&](const char* t, int x, int w) {
            canvas.setColor(SIDColors::TEXT_LABEL);
            canvas.text(t, fonts_->label, visage::Font::kLeft, x, 2, w, 12);
        };
        sectionLbl("UNISON",  xUni,    wUni);
        sectionLbl("GLIDE",   xGlide,  wGlide);
        sectionLbl("OSC MOD", xOscMod, wOscMod);
        sectionLbl("DRIVE",   xDrive,  wDrive);
        sectionLbl("STEREO",  xStereo, wStereo);

        // Vertical dividers between sections
        canvas.setColor(SIDColors::BORDER_INNER);
        for (int dx : { xUni + wUni + 2, xGlide + wGlide + 2,
                        xOscMod + wOscMod + 2, xDrive + wDrive + 2 }) {
            canvas.fill(dx, 4, 1, height() - 8);
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

        // STEREO section
        randPhaseBtn.setFonts(fonts_);  randPhaseBtn.setLabel("RND PH");
        static const char* kPanLabels [3] = { "P1", "P2", "P3" };
        static const char* kHaasLabels[3] = { "H1", "H2", "H3" };
        for (int i = 0; i < 3; ++i) {
            oscPanKnob[i].setFonts(fonts_);
            oscPanKnob[i].setLabel(kPanLabels[i]);
            oscPanKnob[i].setRingColor(SIDColors::ACCENT_CYAN_BRIGHT);
            oscHaasKnob[i].setFonts(fonts_);
            oscHaasKnob[i].setLabel(kHaasLabels[i]);
            oscHaasKnob[i].setRingColor(SIDColors::ACCENT_PURPLE);
        }
    }
};

// ============================================================
//  SIDNoisePanel — compact noise generator section.
//  Type selector (White / Pink / Vinyl / Sweep) + dedicated ADSR
//  knobs + a level knob.  Routed through the global filter + amp
//  as an extra source — great for risers/uplifters when using the
//  Sweep type with a long attack.
// ============================================================
class SIDNoisePanel : public SIDPanelBase {
public:
    SIDCycleButton  typeBtn;     // White / Pink / Vinyl / Sweep
    SIDKnob         attackKnob, decayKnob, sustainKnob, releaseKnob;
    SIDKnob         levelKnob;

    SIDNoisePanel() : SIDPanelBase("NOISE") {}

    void init() override {
        addChild(&typeBtn);
        addChild(&attackKnob);
        addChild(&decayKnob);
        addChild(&sustainKnob);
        addChild(&releaseKnob);
        addChild(&levelKnob);
        updateFonts();
    }
    void dpiChanged() override { updateFonts(); }

    void resized() override {
        const int W = width(), H = height();

        // Row 1 — type cycle button (full width, slim).
        typeBtn.setBounds(6, 24, W - 12, 18);

        // Row 2 — 4 small ADSR knobs in a row.  Sized to fit the panel
        // width with a tight gap (no overflow).
        const int ks  = std::clamp((W - 22) / 4, 24, 36);
        const int gap = 3;
        const int totEnv = 4 * ks + 3 * gap;
        const int eX = std::max(2, (W - totEnv) / 2);
        const int eY = 52;
        attackKnob.setBounds (eX,                  eY, ks, ks);
        decayKnob.setBounds  (eX + (ks + gap),     eY, ks, ks);
        sustainKnob.setBounds(eX + 2 * (ks + gap), eY, ks, ks);
        releaseKnob.setBounds(eX + 3 * (ks + gap), eY, ks, ks);

        // Row 3 — level knob, larger and centred.
        const int lk = std::clamp(H - eY - ks - 24, 36, 56);
        const int lY = eY + ks + 16;
        levelKnob.setBounds((W - lk) / 2, lY, lk, lk);
    }

    void draw(visage::Canvas& canvas) override {
        drawPanelBase(canvas);

        if (fonts_.label.size() > 0) {
            const int W = width();
            // "LEVEL" section label above the bigger knob (visual anchor)
            canvas.setColor(SIDColors::TEXT_DIM);
            canvas.text("ENV", fonts_.label, visage::Font::kCenter,
                        4, 44, W - 8, 10);
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
        typeBtn.setFonts(&fonts_);
        typeBtn.setOptions({"WHITE","PINK","VINYL","SWEEP"});
        attackKnob.setFonts(&fonts_);   attackKnob.setLabel("A");
        attackKnob.setRingColor(SIDColors::ENV_ATTACK);
        decayKnob.setFonts(&fonts_);    decayKnob.setLabel("D");
        decayKnob.setRingColor(SIDColors::ENV_DECAY);
        sustainKnob.setFonts(&fonts_);  sustainKnob.setLabel("S");
        sustainKnob.setRingColor(SIDColors::ENV_SUSTAIN);
        releaseKnob.setFonts(&fonts_);  releaseKnob.setLabel("R");
        releaseKnob.setRingColor(SIDColors::ENV_RELEASE);
        levelKnob.setFonts(&fonts_);    levelKnob.setLabel("LEVEL");
        levelKnob.setRingColor(SIDColors::ACCENT_PURPLE);
        levelKnob.setLarge(true);
    }

    SIDFonts fonts_;
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
        if (!fonts_ || chromeless_) return;
        // Section labels + divider — only when own chrome is drawn.
        struct { int x; int w; const char* t; } lbls[] = {
            {4,  44, "ON"},
            {52, 72, "SWING"},
        };
        canvas.setColor(SIDColors::TEXT_LABEL);
        for (auto& l : lbls)
            canvas.text(l.t, fonts_->label, visage::Font::kCenter, l.x, 20, l.w, 10);

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

    // Noise generator — shares the bottom-right field with Arp + Gate.
    // Compact panel: type cycle + 4 ADSR knobs + 1 level knob.
    SIDNoisePanel        noisePanel;

    // Chip-model selectors — two invisible click regions overlaid on the
    // 6581 / 8580 badges painted into the background PNG.  Each has its own
    // onClicked callback so PluginEditor can wire them to the chip_model
    // parameter (and call the other one's setActive(false) for highlight).
    SIDChipClickArea     chip6581Area;
    SIDChipClickArea     chip8580Area;

    // Master Volume — rotary knob placed in the square empty field next to
    // the TranceSID logo in the header.  Bound to master_volume in
    // PluginEditor (master fader was removed from SIDMasterPanel; this is
    // now the only master-volume control).
    SIDKnob              masterVolKnob;

    // Output VU meter — sits to the right of the master rotary so the user
    // can see the post-limiter output level without scanning the screen.
    SIDOutputMeter       outputMeter;

    // Preset bar
    SIDPresetBar         presetBar;

    // Shared popup overlay — every dropdown widget routes through this so
    // click-outside / click-again-on-button reliably dismiss the popup.
    // Added as a top-level onTop child so it intercepts all clicks while open.
    SIDPopupOverlay      popupOverlay;

    // Toggle for the debug overlay (light yellow field rectangles).  Set
    // from PluginEditor — when true the layout map is drawn on top of
    // the PNG so you can verify each module sits inside its field.
    void setDebugOverlay(bool on) { debugOverlay_ = on; redraw(); }

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
        const int W = width(), H = height();

        // ── Background skeleton — TranSID GUI.jpg, 1672×941 (same aspect as
        //    the editor's 1280×720).  Embedded via juce_add_binary_data and
        //    decoded by Visage's stb_image-based canvas.image() at draw
        //    time.  Two passes at full + half alpha give a screen-style
        //    "lighten" so the neon edges glow as brightly as the
        //    foreground module controls.
        const auto* img = reinterpret_cast<const unsigned char*>(
            BinaryData::TranSID_GUI_jpg);
        const int imgSize = BinaryData::TranSID_GUI_jpgSize;
        if (img != nullptr && imgSize > 0) {
            canvas.setColor(0xFFFFFFFF);
            canvas.image(img, imgSize, 0, 0, W, H);
            canvas.setColor(0x55FFFFFF);
            canvas.image(img, imgSize, 0, 0, W, H);
        } else {
            canvas.setColor(SIDColors::BG_VOID);
            canvas.fill(0, 0, W, H);
        }

        // ── Debug overlay — translucent yellow rectangles around every
        //    field so you can verify modules sit fully inside their panel
        //    artwork.  Toggle via setDebugOverlay(true) from PluginEditor.
        if (debugOverlay_) {
            for (const auto& f : currentFields_) {
                canvas.setColor(0x33FFFF00);                    // 20% yellow fill
                canvas.fill(f.x, f.y, f.w, f.h);
                canvas.setColor(0xCCFFE000);                    // bright outline
                canvas.fill(f.x, f.y,             f.w, 1);     // top
                canvas.fill(f.x, f.y + f.h - 1,   f.w, 1);     // bottom
                canvas.fill(f.x, f.y,             1,   f.h);   // left
                canvas.fill(f.x + f.w - 1, f.y,   1,   f.h);   // right
            }
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
            popupOverlay.setFont(visage::Font(12.0f, fd, fs, dpi));
            // Master volume knob inherits the small-label font for its caption.
            masterVolKnob.setFonts(&masterVolFonts_);
            masterVolFonts_.init(dpi, fd, fs);
            masterVolKnob.setFonts(&masterVolFonts_);
            masterVolKnob.setLarge(true);
            masterVolKnob.setLabel("MASTER");
            masterVolKnob.setRingColor(SIDColors::ACCENT_CYAN_BRIGHT);
        }
    }

    // Field rectangles measured against the new TranSID GUI.jpg skeleton
    // (1672 × 941).  resized() scales them to the current editor size so
    // the layout stays glued to the artwork at any host scaling.
    //
    // Several panels in the previous design no longer have a place in this
    // skeleton (AMP, MASTER as panel, voiceMod, LFO1/2, noise).  Their
    // parameters still exist in APVTS; the UI just doesn't expose them
    // here.  Those panels get hidden bounds in layoutAll().
    struct FieldRect { int x, y, w, h; };
    static constexpr int kPngW = 1672;
    static constexpr int kPngH = 941;

    // ── HEADER (top row, y ≈ 10-110) ────────────────────────────────
    // MACROS column moved from the previous top-of-header position to the
    // LEFT VERTICAL STRIP — 8 slots stacked.  Only the first 4 macros are
    // backed by params (macro1..4); the lower 4 are visual placeholders
    // drawn into the background.
    static constexpr FieldRect kSklMacros      {  20,   10,  210, 555 };
    static constexpr FieldRect kSklChip6581    { 244,   16,  198,  98 };
    static constexpr FieldRect kSklChip8580    { 458,   16,  198,  98 };
    static constexpr FieldRect kSklPreset      { 678,   16,  490,  90 };
    static constexpr FieldRect kSklMaster      {1184,   16,  286,  90 };
    // ── OSC row + FX RACK (y ≈ 120-395) ────────────────────────────
    static constexpr FieldRect kSklOsc1        { 244,  124,  320, 280 };
    static constexpr FieldRect kSklOsc2        { 568,  124,  330, 280 };
    static constexpr FieldRect kSklOsc3        { 904,  124,  274, 280 };
    static constexpr FieldRect kSklFxRack      {1186,  124,  470, 280 };
    // ── FILTER + HYPERSAW row (y ≈ 415-560) ────────────────────────
    static constexpr FieldRect kSklFilter      { 244,  414,  704, 174 };
    static constexpr FieldRect kSklHypersaw    { 968,  414,  696, 174 };
    // ── MOD MATRIX / CHORDS / ARP row (y ≈ 580-770) ────────────────
    static constexpr FieldRect kSklModMatrix   { 244,  584,  300, 188 };
    static constexpr FieldRect kSklChordEng    { 552,  584,  306, 188 };
    static constexpr FieldRect kSklArp         { 866,  584,  800, 188 };
    // ── TRANCE GATE strip (y ≈ 790-935) ────────────────────────────
    static constexpr FieldRect kSklTranceGate  {  20,  784, 1490, 154 };
    static constexpr FieldRect kSklGateControls{1516,  784,  148, 154 };

    void layoutAll() {
        const int W = width(), H = height();
        const float sx = float(W) / float(kPngW);
        const float sy = float(H) / float(kPngH);

        auto map = [&](const FieldRect& f, int margin = 4) {
            return FieldRect{
                int(std::round(f.x * sx)) + margin,
                int(std::round(f.y * sy)) + margin,
                int(std::round(f.w * sx)) - 2 * margin,
                int(std::round(f.h * sy)) - 2 * margin };
        };
        auto place = [&](visage::Frame& frame, const FieldRect& f, int margin = 4) {
            const auto r = map(f, margin);
            frame.setBounds(r.x, r.y, r.w, r.h);
            currentFields_.push_back(r);   // record for the debug overlay
        };

        currentFields_.clear();

        // Popup overlay covers the whole editor — must sit on top so it
        // intercepts every click while a dropdown is open.
        popupOverlay.setBounds(0, 0, W, H);

        // ── HIDDEN PANELS — no equivalent in the new skeleton ──────────
        // The following panels have backing parameters that are still in
        // APVTS, but the new skeleton doesn't show their controls.  Place
        // them off-screen so they don't intercept clicks and don't draw
        // any panel chrome on top of the JPG.  When future skeleton
        // revisions expose them, just give them real bounds.
        const FieldRect off { -1000, -1000, 1, 1 };
        ampPanel.setBounds   (off.x, off.y, off.w, off.h);
        masterPanel.setBounds(off.x, off.y, off.w, off.h);
        voiceModPanel.setBounds(off.x, off.y, off.w, off.h);
        lfo1.setBounds(off.x, off.y, off.w, off.h);
        lfo2.setBounds(off.x, off.y, off.w, off.h);
        noisePanel.setBounds(off.x, off.y, off.w, off.h);
        filterScope.setBounds(off.x, off.y, off.w, off.h);
        scope1.setBounds(off.x, off.y, off.w, off.h);
        scope2.setBounds(off.x, off.y, off.w, off.h);
        scope3.setBounds(off.x, off.y, off.w, off.h);

        // ── HEADER (top row) ────────────────────────────────────────────
        // Two chip click-areas overlaying the painted SID 6581 / 8580
        // badges in the skeleton.
        place(chip6581Area, kSklChip6581, /*margin=*/0);
        place(chip8580Area, kSklChip8580, /*margin=*/0);
        // Preset browser sits in the wide rounded slot at the top.
        place(presetBar,    kSklPreset,   /*margin=*/0);
        // Master volume + slim VU meter in the right header slot.
        {
            const auto r = map(kSklMaster, /*margin=*/6);
            const int meterW = std::max(18, r.w / 5);
            const int gap    = 6;
            const int knobAreaW = r.w - meterW - gap;
            const int side = std::max(24, std::min(knobAreaW, r.h));
            const int kx = r.x + (knobAreaW - side) / 2;
            const int ky = r.y + (r.h - side) / 2;
            masterVolKnob.setBounds(kx, ky, side, side);
            outputMeter.setBounds(r.x + knobAreaW + gap, r.y + 2,
                                  meterW, r.h - 4);
            currentFields_.push_back(r);
        }

        // ── MACROS LEFT VERTICAL COLUMN ────────────────────────────────
        // Skeleton shows 8 macro slots; macro1..4 are functional, the
        // rest are placeholder positions left for future DSP.  Place the
        // existing 4-knob macroPanel inside the TOP HALF of the column
        // so the bottom-half placeholder slots in the skeleton remain
        // visible (the panel chrome doesn't cover them).
        {
            const auto r = map(kSklMacros);
            const int topH = (r.h * 4) / 8;            // top 4 of 8 slots
            macroPanel.setBounds(r.x, r.y, r.w, topH);
            currentFields_.push_back({r.x, r.y, r.w, topH});
        }

        // ── OSC 1/2/3 ──────────────────────────────────────────────────
        // Each OSC panel placed inside its skeleton field.  The scope and
        // filter-info displays from the previous design are hidden (the
        // skeleton doesn't carve out room for them).
        auto placeOscField = [&](SIDOscillatorPanel& panel, const FieldRect& field) {
            const auto r = map(field);
            panel.setBounds(r.x, r.y, r.w, r.h);
            currentFields_.push_back(r);
        };
        placeOscField(osc1, kSklOsc1);
        placeOscField(osc2, kSklOsc2);
        placeOscField(osc3, kSklOsc3);

        // ── FILTER / DRIVE ─────────────────────────────────────────────
        // Filter panel takes the entire FILTER/DRIVE field (no separate
        // filter-info display in this skeleton).
        place(filterPanel, kSklFilter);

        // ── FX RACK ────────────────────────────────────────────────────
        // The skeleton's FX rack shows 6 effect rows (Chorus/Delay/Reverb
        // + Phaser/Dimension/Compressor).  Only the first three have
        // backing parameters; effectsPanel renders them in its existing
        // 3-row vertical layout inside the rack's column.
        place(effectsPanel, kSklFxRack);

        // ── MOD MATRIX ─────────────────────────────────────────────────
        // Existing modMatrix has 4 rows; the skeleton shows 8.  Place
        // modMatrix inside the upper half of the field; the lower 4 rows
        // in the skeleton remain visible as placeholders.
        {
            const auto r = map(kSklModMatrix);
            const int topH = r.h / 2 + r.h / 4;        // ~75 % of field
            modMatrix.setBounds(r.x, r.y, r.w, topH);
            currentFields_.push_back({r.x, r.y, r.w, topH});
        }

        // ── ARP ────────────────────────────────────────────────────────
        place(arpPanel, kSklArp);

        // ── TRANCE GATE ────────────────────────────────────────────────
        // Existing gatePanel takes the wide bottom strip.  The Velocity
        // / Probability / Swing sub-rows in the skeleton are placeholders
        // (no per-step velocity / probability params exist yet).
        place(gatePanel, kSklTranceGate);
    }

    visage::Font font_large_;
    visage::Font font_title_;
    visage::Font font_label_;
    visage::Font font_label_small_;

    // Font cache used by the standalone master-volume knob.
    SIDFonts masterVolFonts_;

    // Debug overlay
    bool debugOverlay_ = false;
    std::vector<FieldRect> currentFields_;
};
