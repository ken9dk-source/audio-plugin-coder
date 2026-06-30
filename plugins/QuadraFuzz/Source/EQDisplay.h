#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// EQ / filterbank window — a 1:1 recreation of the original QuadraFuzz display.
//   * 4 diamond handles  = per-band LEVEL  (drag vertically)   -> proc.bandLevelDb
//   * 3 triangle handles = crossover freqs (drag horizontally) -> proc.crossHz
//     (+ 2 fixed outer triangles marking the band-edge limits)
//   * blue plateau response curve
//
// CRITICAL: these handles read/write the processor's INTERNAL filterbank state,
// NOT the Band1..Band4 knob parameters. Turning a knob does nothing here, and
// dragging a handle does nothing to the knobs — exactly like the original DLL.

class EQDisplay : public juce::Component, private juce::Timer
{
public:
    explicit EQDisplay (QuadraFuzzAudioProcessor& p) : proc_ (p)
    {
        setOpaque (false);   // let the skin's navy + dB ruler + freq labels show through
        startTimerHz (20);   // reflect external (preset/state) changes
    }

    ~EQDisplay() override { stopTimer(); }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        const float W = (float) getWidth();
        const float H = (float) getHeight();

        // Transparent: the skin (1000.png) already provides the navy panel, the
        // dB ruler (20..-20) and the frequency labels. We only draw the live grid,
        // response curve and the draggable handles on top.

        // ---- grid -------------------------------------------------------
        g.setColour (juce::Colour (0x333a6c84));
        for (float f : { 50.f, 100.f, 200.f, 400.f, 800.f, 1600.f, 3200.f, 6400.f })
            g.drawVerticalLine (juce::roundToInt (freqToX (f)), 0.f, H);
        for (float db : { -15.f, -10.f, -5.f, 5.f, 10.f, 15.f })
            g.drawHorizontalLine (juce::roundToInt (dbToY (db)), 0.f, W);
        g.setColour (juce::Colour (0x553a6c84));
        g.drawHorizontalLine (juce::roundToInt (dbToY (0.f)), 0.f, W);

        // ---- geometry ---------------------------------------------------
        float edge[5];
        bandEdges (edge);
        float lvl[4];
        for (int b = 0; b < 4; ++b) lvl[b] = proc_.bandLevelDb[b].load();

        // ---- blue response curve (exactly like the original) -----------
        // A CONTINUOUS horizontal plateau across the bands (stepping vertically
        // where two band levels differ), with the outer edges dropping to the
        // triangles and thin vertical "stems" hanging from the plateau down to
        // the triangles at the 3 inner crossovers. Straight segments only —
        // no Bézier / interpolation.
        const float stemY = H - 14.f;    // bottom of the stems, at the triangles
        g.setColour (juce::Colour (0xff8fc4e8));
        const juce::PathStrokeType stroke (2.0f, juce::PathStrokeType::mitered,
                                                 juce::PathStrokeType::butt);

        juce::Path plateau;
        plateau.startNewSubPath (edge[0], stemY);            // bottom-left triangle
        plateau.lineTo           (edge[0], dbToY (lvl[0]));   // up to band 0 level
        for (int b = 0; b < 4; ++b)
        {
            plateau.lineTo (edge[b + 1], dbToY (lvl[b]));      // plateau across band b
            if (b < 3)
                plateau.lineTo (edge[b + 1], dbToY (lvl[b + 1])); // vertical step
        }
        plateau.lineTo (edge[4], stemY);                     // down to bottom-right
        g.strokePath (plateau, stroke);

        for (int i = 1; i <= 3; ++i)                          // inner crossover stems
        {
            const float yTop = juce::jmin (dbToY (lvl[i - 1]), dbToY (lvl[i]));
            juce::Path stem;
            stem.startNewSubPath (edge[i], yTop);
            stem.lineTo           (edge[i], stemY);
            g.strokePath (stem, stroke);
        }

        // ---- diamonds (band levels) ------------------------------------
        for (int b = 0; b < 4; ++b)
        {
            const float cx = (edge[b] + edge[b + 1]) * 0.5f;
            const float cy = dbToY (lvl[b]);
            const bool  hot = (dragKind_ == Drag::level && dragIdx_ == b);
            juce::Path d;
            d.startNewSubPath (cx,        cy - 6.f);
            d.lineTo          (cx + 5.f,  cy);
            d.lineTo          (cx,        cy + 6.f);
            d.lineTo          (cx - 5.f,  cy);
            d.closeSubPath();
            g.setColour (hot ? juce::Colours::white : juce::Colour (0xffd6e2ea));
            g.fillPath (d);
            g.setColour (juce::Colour (0xff5a6e7a));
            g.strokePath (d, juce::PathStrokeType (1.f));
        }

        // ---- triangles (all 5 band edges — all draggable) --------------
        for (int i = 0; i < 5; ++i)
        {
            const float x   = edge[i];
            const bool  inner = (i >= 1 && i <= 3);
            const bool  hot   = (inner && dragKind_ == Drag::cross && dragIdx_ == (i - 1))
                              || (i == 0 && dragKind_ == Drag::edge && dragIdx_ == 0)
                              || (i == 4 && dragKind_ == Drag::edge && dragIdx_ == 1);
            juce::Path t;
            t.startNewSubPath (x - 5.f, H);
            t.lineTo (x + 5.f, H);
            t.lineTo (x,       H - 10.f);
            t.closeSubPath();
            g.setColour (hot ? juce::Colours::white : juce::Colour (0xffb8c8d2));
            g.fillPath (t);
        }
    }

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragKind_ = Drag::none;
        float edge[5]; bandEdges (edge);

        // diamonds first (band level)
        for (int b = 0; b < 4; ++b)
        {
            const float cx = (edge[b] + edge[b + 1]) * 0.5f;
            const float cy = dbToY (proc_.bandLevelDb[b].load());
            if (std::abs (e.position.x - cx) < 8.f && std::abs (e.position.y - cy) < 9.f)
            { dragKind_ = Drag::level; dragIdx_ = b; return; }
        }
        // inner crossovers — grabbable anywhere along their vertical divider (the
        // triangle + the stem above it), exactly like the original. Diamonds sit at
        // band CENTRES and are tested first, so there is no x-overlap to worry about.
        for (int i = 0; i < 3; ++i)
        {
            const float x = freqToX (proc_.crossHz[i].load());
            if (std::abs (e.position.x - x) < 9.f && e.position.y > 20.f)
            { dragKind_ = Drag::cross; dragIdx_ = i; return; }
        }
        // outer edges — the 2 outer triangles (band-0 low edge, band-3 high edge)
        if (std::abs (e.position.x - edge[0]) < 9.f && e.position.y > 20.f)
        { dragKind_ = Drag::edge; dragIdx_ = 0; return; }
        if (std::abs (e.position.x - edge[4]) < 9.f && e.position.y > 20.f)
        { dragKind_ = Drag::edge; dragIdx_ = 1; return; }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragKind_ == Drag::level)
        {
            const float db = juce::jlimit (-20.f, 20.f, yToDb (e.position.y));
            proc_.bandLevelDb[dragIdx_].store (db);
            repaint();
        }
        else if (dragKind_ == Drag::cross)
        {
            float f = juce::jlimit (F_MIN, F_MAX, xToFreq (e.position.x));
            // keep crossovers ordered, bounded by the neighbour cross or the outer edge
            const float lo = (dragIdx_ > 0) ? proc_.crossHz[dragIdx_ - 1].load() * 1.05f
                                            : proc_.edgeLoHz.load() * 1.05f;
            const float hi = (dragIdx_ < 2) ? proc_.crossHz[dragIdx_ + 1].load() * 0.95f
                                            : proc_.edgeHiHz.load() * 0.95f;
            f = juce::jlimit (lo, hi, f);
            proc_.crossHz[dragIdx_].store (f);
            repaint();
        }
        else if (dragKind_ == Drag::edge)
        {
            float f = juce::jlimit (F_MIN, F_MAX, xToFreq (e.position.x));
            if (dragIdx_ == 0)   // low edge — stays below cross0
            {
                f = juce::jlimit (F_MIN, proc_.crossHz[0].load() * 0.95f, f);
                proc_.edgeLoHz.store (f);
            }
            else                 // high edge — stays above cross2
            {
                f = juce::jlimit (proc_.crossHz[2].load() * 1.05f, F_MAX, f);
                proc_.edgeHiHz.store (f);
            }
            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent&) override
    {
        dragKind_ = Drag::none;
        repaint();
    }

    void mouseMove (const juce::MouseEvent& e) override
    {
        const bool overHandle = hitDiamond (e.position) || hitTriangle (e.position);
        setMouseCursor (overHandle ? juce::MouseCursor::PointingHandCursor
                                   : juce::MouseCursor::NormalCursor);
    }

private:
    QuadraFuzzAudioProcessor& proc_;
    enum class Drag { none, level, cross, edge };
    Drag dragKind_ = Drag::none;
    int  dragIdx_  = -1;

    static constexpr float DB_RANGE = 20.f;

    void timerCallback() override { if (dragKind_ == Drag::none) repaint(); }

    // band edges in x: [edgeLo, cross0, cross1, cross2, edgeHi] — all 5 draggable
    void bandEdges (float (&edge)[5]) const
    {
        edge[0] = freqToX (proc_.edgeLoHz.load());
        edge[1] = freqToX (proc_.crossHz[0].load());
        edge[2] = freqToX (proc_.crossHz[1].load());
        edge[3] = freqToX (proc_.crossHz[2].load());
        edge[4] = freqToX (proc_.edgeHiHz.load());
    }

    bool hitDiamond (juce::Point<float> p) const
    {
        float edge[5]; bandEdges (edge);
        for (int b = 0; b < 4; ++b)
        {
            const float cx = (edge[b] + edge[b + 1]) * 0.5f;
            const float cy = dbToY (proc_.bandLevelDb[b].load());
            if (std::abs (p.x - cx) < 8.f && std::abs (p.y - cy) < 9.f) return true;
        }
        return false;
    }
    bool hitTriangle (juce::Point<float> p) const
    {
        if (p.y <= 20.f) return false;
        float edge[5]; bandEdges (edge);
        for (int i = 0; i < 5; ++i)              // all 5 edges are draggable
            if (std::abs (p.x - edge[i]) < 9.f) return true;
        return false;
    }

    // x-axis = the skin's frequency ruler: 025 (25 Hz) at the left .. 12.8 kHz at
    // the right, octaves evenly spaced. The plotting area is 263/290 of the panel
    // width (the dB ruler 20..-20 occupies the right ~27 px). Measured from 1000.png.
    static constexpr float F_MIN     = 25.f;
    static constexpr float F_MAX     = 12800.f;
    static constexpr float LOG2_SPAN = 9.f;            // log2(12800/25) = log2(512) = 9
    static constexpr float PLOT_FRAC = 263.f / 290.f;  // plotting width / panel width
    float plotW () const noexcept { return (float) getWidth() * PLOT_FRAC; }
    float freqToX (float f) const noexcept
    { return juce::jlimit (0.f, plotW(), plotW() * std::log2 (f / F_MIN) / LOG2_SPAN); }
    float xToFreq (float x) const noexcept
    { return F_MIN * std::pow (2.f, (x / plotW()) * LOG2_SPAN); }
    // 0 dB sits where the original's default diamonds sit (clean y99 = component
    // 69), ~3.55 px per dB so the skin ruler's ±20 spans the grid exactly.
    static constexpr float ZERO_Y    = 69.f;
    static constexpr float PX_PER_DB = 3.55f;
    float dbToY (float db) const noexcept { return ZERO_Y - db * PX_PER_DB; }
    float yToDb (float y)  const noexcept { return (ZERO_Y - y) / PX_PER_DB; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQDisplay)
};
