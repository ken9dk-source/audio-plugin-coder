#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"

// EQ / filterbank window — a 1:1 recreation of the original QuadraFuzz display.
//   * 4 diamond handles (top)    = per-band LEVEL  (drag vertically) -> bandLevelDb
//   * 5 diamond handles (bottom) = band edges / crossovers (drag horizontally)
//   * response curve: the DLL's STYLISED display (not the raw filter magnitude) —
//     a plateau at each band level, rounded shoulders converging to a thin stem at
//     each edge, faint 5 dB / per-octave grid, and a log frequency axis. All
//     colours + geometry sampled pixel-for-pixel from the original editor.
//
// The handles read/write the processor's INTERNAL filterbank state, NOT the
// Band1..Band4 knob parameters.

class EQDisplay : public juce::Component, private juce::Timer
{
public:
    explicit EQDisplay (QuadraFuzzAudioProcessor& p) : proc_ (p)
    {
        setOpaque (false);   // the skin provides the navy panel + dB ruler
        startTimerHz (20);
    }
    ~EQDisplay() override { stopTimer(); }

    //==========================================================================
    void paint (juce::Graphics& g) override
    {
        float edge[5]; bandEdges (edge);
        float lvl[4]; for (int b = 0; b < 4; ++b) lvl[b] = proc_.bandLevelDb[b].load();
        const float botY = dbToY (-20.f);

        drawGrid (g);

        // Clip curve/stems/diamonds to the plot rectangle: a high band pushed up
        // (edge above 12.8 kHz, e.g. the default edgeHi=22 kHz) must NOT draw past
        // the right edge into the dB ruler — the original clips it there too. The
        // frequency axis below the plot is drawn afterwards, unclipped.
        g.saveState();
        g.reduceClipRegion (juce::Rectangle<int> (0, 0,
                            juce::roundToInt (plotX0() + plotW()),
                            juce::roundToInt (dbToY (-20.f)) + 7));

        // ---- response curve: plateau at the band levels, ROUNDED shoulders that
        //      CONVERGE to a thin vertical stem at each edge (down to the -20 dB
        //      marker). Reproduces the DLL's stylised display: the stem starts at
        //      -2.3 dB (measured) and the neighbouring shoulders meet, they don't
        //      cross. Curve colour + grid sampled from the original editor. ------
        g.setColour (juce::Colour (0xff59a8d1));
        const juce::PathStrokeType curve (1.0f, juce::PathStrokeType::curved,
                                                juce::PathStrokeType::rounded);
        const float DIP = 7.f, R = 6.f;   // stem-top drop (~2.3 dB) / shoulder run
        for (int b = 0; b < 4; ++b)
        {
            const float yb = dbToY (lvl[b]);
            juce::Path c;
            c.startNewSubPath (edge[b], yb + DIP);                    // left shoulder foot
            c.quadraticTo (edge[b], yb, edge[b] + R, yb);            // round up to plateau
            c.lineTo (edge[b + 1] - R, yb);                          // plateau
            c.quadraticTo (edge[b + 1], yb, edge[b + 1], yb + DIP);  // round down to stem
            g.strokePath (c, curve);
        }
        for (int i = 0; i < 5; ++i)                                  // 5 stems -> markers
        {
            const float yTop = (i == 0) ? dbToY (lvl[0]) + DIP
                             : (i == 4) ? dbToY (lvl[3]) + DIP
                             : juce::jmin (dbToY (lvl[i - 1]), dbToY (lvl[i])) + DIP;
            juce::Path s; s.startNewSubPath (edge[i], yTop); s.lineTo (edge[i], botY);
            g.strokePath (s, juce::PathStrokeType (1.0f));
        }

        // ---- diamonds: 4 band levels (top) + 5 edges (bottom, -20) -------
        for (int b = 0; b < 4; ++b)
            drawDiamond (g, (edge[b] + edge[b + 1]) * 0.5f, dbToY (lvl[b]),
                         dragKind_ == Drag::level && dragIdx_ == b, true);   // tall
        for (int i = 0; i < 5; ++i)
        {
            const bool hot = (i >= 1 && i <= 3 && dragKind_ == Drag::cross && dragIdx_ == i - 1)
                           || (i == 0 && dragKind_ == Drag::edge && dragIdx_ == 0)
                           || (i == 4 && dragKind_ == Drag::edge && dragIdx_ == 1);
            drawDiamond (g, edge[i], botY, hot, false);                      // wide
        }

        g.restoreState();          // end plot clip — the axis below draws unclipped
        drawFreqAxis (g);
    }

    //==========================================================================
    void mouseDown (const juce::MouseEvent& e) override
    {
        dragKind_ = Drag::none;
        float edge[5]; bandEdges (edge);
        for (int b = 0; b < 4; ++b)                         // top diamonds (level)
        {
            const float cx = (edge[b] + edge[b + 1]) * 0.5f;
            const float cy = dbToY (proc_.bandLevelDb[b].load());
            if (std::abs (e.position.x - cx) < 8.f && std::abs (e.position.y - cy) < 9.f)
            { dragKind_ = Drag::level; dragIdx_ = b; return; }
        }
        for (int i = 0; i < 5; ++i)                         // bottom diamonds (edges)
        {
            if (edge[i] > plotX0() + plotW() + 2.f) continue;   // off-plot marker: not grabbable
            if (std::abs (e.position.x - edge[i]) < 9.f && e.position.y > 20.f)
            {
                if      (i == 0) { dragKind_ = Drag::edge;  dragIdx_ = 0; }
                else if (i == 4) { dragKind_ = Drag::edge;  dragIdx_ = 1; }
                else             { dragKind_ = Drag::cross; dragIdx_ = i - 1; }
                return;
            }
        }
    }

    void mouseDrag (const juce::MouseEvent& e) override
    {
        if (dragKind_ == Drag::level)
        {
            proc_.bandLevelDb[dragIdx_].store (juce::jlimit (-20.f, 20.f, yToDb (e.position.y)));
            repaint();
        }
        else if (dragKind_ == Drag::cross)
        {
            float f = juce::jlimit (F_MIN, F_MAX, xToFreq (e.position.x));
            const float lo = (dragIdx_ > 0) ? proc_.crossHz[dragIdx_ - 1].load() * 1.05f
                                            : proc_.edgeLoHz.load() * 1.05f;
            const float hi = (dragIdx_ < 2) ? proc_.crossHz[dragIdx_ + 1].load() * 0.95f
                                            : proc_.edgeHiHz.load() * 0.95f;
            proc_.crossHz[dragIdx_].store (juce::jlimit (lo, hi, f));
            repaint();
        }
        else if (dragKind_ == Drag::edge)
        {
            float f = juce::jlimit (F_MIN, F_MAX, xToFreq (e.position.x));
            if (dragIdx_ == 0) proc_.edgeLoHz.store (juce::jlimit (F_MIN, proc_.crossHz[0].load() * 0.95f, f));
            else               proc_.edgeHiHz.store (juce::jlimit (proc_.crossHz[2].load() * 1.05f, F_MAX, f));
            repaint();
        }
    }

    void mouseUp (const juce::MouseEvent&) override { dragKind_ = Drag::none; repaint(); }

    void mouseMove (const juce::MouseEvent& e) override
    {
        setMouseCursor (overHandle (e.position) ? juce::MouseCursor::PointingHandCursor
                                                : juce::MouseCursor::NormalCursor);
    }

private:
    QuadraFuzzAudioProcessor& proc_;
    enum class Drag { none, level, cross, edge };
    Drag dragKind_ = Drag::none;
    int  dragIdx_  = -1;

    void timerCallback() override { if (dragKind_ == Drag::none) repaint(); }

    // grey lozenge handle. TOP (level) handles are TALL (7x11, drag up/down);
    // BOTTOM (edge) handles are WIDE (11x7, drag left/right) — measured off the
    // original, which orients each marker to hint its drag axis.
    void drawDiamond (juce::Graphics& g, float cx, float cy, bool hot, bool tall) const
    {
        const float hw = tall ? 3.5f : 5.5f;
        const float hh = tall ? 5.5f : 3.5f;
        juce::Path d;
        d.startNewSubPath (cx, cy - hh); d.lineTo (cx + hw, cy);
        d.lineTo (cx, cy + hh);          d.lineTo (cx - hw, cy);
        d.closeSubPath();
        g.setColour (hot ? juce::Colours::white : juce::Colour (0xffc8c8c8));
        g.fillPath (d);
    }

    // faint background grid: 5 dB horizontals + per-octave verticals, colour
    // (6,62,87) sampled from the original editor (the clone skin has no grid).
    void drawGrid (juce::Graphics& g) const
    {
        g.setColour (juce::Colour (0xff063e57));
        const float xL = freqToX (25.f), xR = freqToX (12800.f);
        for (int db = -20; db <= 20; db += 5)
            g.drawHorizontalLine (juce::roundToInt (dbToY ((float) db)), xL, xR);
        const float yT = dbToY (20.f), yB = dbToY (-20.f);
        for (int oct = 0; oct <= 9; ++oct)
            g.drawVerticalLine (juce::roundToInt (freqToX (25.f * (float) (1 << oct))), yT, yB);
    }

    // frequency axis: log labels 025..12.8 kHz + minor ticks, below the plot.
    void drawFreqAxis (juce::Graphics& g) const
    {
        const float ty = dbToY (-20.f) + 8.f;
        g.setColour (juce::Colour (0xff6796a8));
        g.setFont (juce::Font (juce::FontOptions().withHeight (9.5f)));
        static const std::pair<float, const char*> maj[] = {
            { 25.f,"025" },{ 50.f,"0.05" },{ 100.f,"0.1" },{ 200.f,"0.2" },{ 400.f,"0.4" },
            { 800.f,"0.8" },{ 1600.f,"1.6" },{ 3200.f,"3.2" },{ 6400.f,"6.4" },{ 12800.f,"12.8" } };
        for (auto& m : maj)
        {
            const int x = juce::roundToInt (freqToX (m.first));
            g.drawVerticalLine (x, ty, ty + 4.f);
            g.drawText (m.second, x - 15, juce::roundToInt (ty) + 4, 30, 11,
                        juce::Justification::centred, false);
        }
        for (int oct = 0; oct < 9; ++oct)                    // 3 minor ticks per octave
            for (float s : { 1.25f, 1.5f, 1.75f })           // linear quarter-octave (measured)
            {
                const float f = 25.f * (float) (1 << oct) * s;
                if (f < 12800.f) g.drawVerticalLine (juce::roundToInt (freqToX (f)), ty, ty + 2.5f);
            }
        g.drawText ("kHz", juce::roundToInt (freqToX (12800)) + 9, juce::roundToInt (ty) + 4,
                    26, 11, juce::Justification::left, false);
    }

    void bandEdges (float (&edge)[5]) const
    {
        edge[0] = freqToX (proc_.edgeLoHz.load());
        edge[1] = freqToX (proc_.crossHz[0].load());
        edge[2] = freqToX (proc_.crossHz[1].load());
        edge[3] = freqToX (proc_.crossHz[2].load());
        edge[4] = freqToX (proc_.edgeHiHz.load());
    }
    bool overHandle (juce::Point<float> p) const
    {
        float edge[5]; bandEdges (edge);
        for (int b = 0; b < 4; ++b)
        {
            const float cx = (edge[b] + edge[b + 1]) * 0.5f;
            const float cy = dbToY (proc_.bandLevelDb[b].load());
            if (std::abs (p.x - cx) < 8.f && std::abs (p.y - cy) < 9.f) return true;
        }
        if (p.y > 20.f)
            for (int i = 0; i < 5; ++i)
                if (edge[i] <= plotX0() + plotW() + 2.f && std::abs (p.x - edge[i]) < 9.f) return true;
        return false;
    }

    // x-axis: 25 Hz@x392 .. 12.8 kHz@x605 (component x15..228 of the 290-wide panel).
    static constexpr float F_MIN     = 25.f;
    static constexpr float F_MAX     = 12800.f;
    static constexpr float LOG2_SPAN = 9.f;
    static constexpr float PLOT_X0F  = 15.f  / 290.f;
    static constexpr float PLOT_WF   = 213.f / 290.f;
    float plotX0 () const noexcept { return (float) getWidth() * PLOT_X0F; }
    float plotW  () const noexcept { return (float) getWidth() * PLOT_WF; }
    float freqToX (float f) const noexcept
    { return plotX0() + plotW() * std::log2 (f / F_MIN) / LOG2_SPAN; }
    float xToFreq (float x) const noexcept
    { return F_MIN * std::pow (2.f, ((x - plotX0()) / plotW()) * LOG2_SPAN); }

    // y-axis: 0 dB at component y69 (editor y99); -20 dB at the bottom diamond row
    // (editor y159) -> 3.0 px/dB.
    static constexpr float ZERO_Y    = 69.f;
    static constexpr float PX_PER_DB = 3.0f;
    float dbToY (float db) const noexcept { return ZERO_Y - db * PX_PER_DB; }
    float yToDb (float y)  const noexcept { return (ZERO_Y - y) / PX_PER_DB; }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (EQDisplay)
};
