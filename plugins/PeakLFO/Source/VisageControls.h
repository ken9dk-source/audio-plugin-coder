#pragma once

// PeakLFO — Visage UI: tempo-synced volume LFO (tremolo).
// Controls: Volume (base), Depth (swing), Tension, Speed (steps), Phase (0/25/50/75%), Shape.
// FL "meter-box" look. Same Visage idiom as plugins/gnarly3.

#include <visage/ui.h>
#include <visage/graphics.h>
#include "BinaryData.h"
#include <juce_core/juce_core.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <functional>
#include <string>

class VisageMainView : public visage::Frame {
public:
    enum class ParamId { Volume = 0, Depth, Tension, Speed, Phase, Count };
    static constexpr int kNumWheels = static_cast<int>(ParamId::Count);

    using ParamChangeFn  = std::function<void(ParamId, float)>; // value 0..1
    using ShapeChangeFn  = std::function<void(int)>;            // 0..3

    VisageMainView() {
        const char* labels[kNumWheels] = { "VOLUME", "DEPTH", "TENSION", "SPEED", "PHASE" };
        const Type  types [kNumWheels] = { Type::Percent, Type::Percent, Type::Bipolar, Type::Speed, Type::Phase };
        const float defs  [kNumWheels] = { 0.75f, 0.5f, 0.5f, 0.666f, 0.0f };
        for (int i = 0; i < kNumWheels; ++i) {
            wheels_[i].label = labels[i];
            wheels_[i].type  = types[i];
            wheels_[i].value01 = defs[i];
        }
    }

    void init() override { updateFonts(); }
    void dpiChanged() override { updateFonts(); }
    void resized() override { layout(); laidW_ = width(); laidH_ = height(); }

    void setParamChangeCallback(ParamChangeFn fn) { on_param_ = std::move(fn); }
    void setShapeChangeCallback(ShapeChangeFn fn) { on_shape_ = std::move(fn); }
    void setWheel(ParamId id, float v01) { setWheelValue(static_cast<int>(id), v01, false); }
    void setShape(int s) { shape_ = std::clamp(s, 0, 3); redraw(); }
    void setMeters(float lfo, float gain) { meter_lfo_ = lfo; meter_gain_ = gain; redraw(); }

    // ================= draw =================
    void draw(visage::Canvas& canvas) override {
        ensureLayout();
        canvas.setColor(kBg);   canvas.fill(0, 0, width(), height());
        drawTitle(canvas);
        drawScope(canvas);
        panelBox(canvas, panel_);
        for (int i = 0; i < kNumWheels; ++i) drawWheel(canvas, wheels_[i]);
        drawShapeSelector(canvas);
    }

    // ================= mouse =================
    void mouseDown(const visage::MouseEvent& e) override {
        ensureLayout();
        // NOTE: VisageJuceHost fills window_position (not relative_position); this frame is
        // the root at (0,0) so window coords == our local layout coords.
        const auto p = e.windowPosition();
        for (int s = 0; s < 4; ++s)
            if (hit(seg_[s], p.x, p.y)) { shape_ = s; if (on_shape_) on_shape_(s); redraw(); return; }
        active_ = hitWheel(p.x, p.y);
        if (active_ >= 0) {
            drag_y_ = p.y; drag_v_ = wheels_[active_].value01; wheels_[active_].dragging = true;
        }
    }
    void mouseDrag(const visage::MouseEvent& e) override {
        if (active_ < 0) return;
        const auto p = e.windowPosition();
        const float d = (drag_y_ - p.y) * 0.006f * (e.isShiftDown() ? 0.25f : 1.0f);
        setWheelValue(active_, std::clamp(drag_v_ + d, 0.0f, 1.0f), true);
    }
    void mouseUp(const visage::MouseEvent&) override {
        if (active_ >= 0) wheels_[active_].dragging = false;
        active_ = -1;
    }

private:
    enum class Type { Percent, Bipolar, Speed, Phase };
    struct Rect  { float x=0,y=0,w=0,h=0; };
    struct Wheel { float cx=0,cy=0,r=0,value01=0; bool dragging=false; const char* label=""; Type type=Type::Percent; };

    static constexpr unsigned int kBg=0xff2b2f33, kPanel=0xff23262a, kBorder=0xff3a3f45,
        kWheel=0xff1c1f22, kRing=0xff4a5058, kGreen=0xff7ec850, kYellow=0xffe8d24a,
        kText=0xffd8dde2, kDim=0xff7a828a;

    void ensureLayout() { if (width() != laidW_ || height() != laidH_) { layout(); laidW_ = width(); laidH_ = height(); } }

    void layout() {
        const float W = (float) width(), H = (float) height(), pad = 12.0f;
        title_ = { 0, 0, W, 26 };
        scope_ = { pad, 30, W - 2*pad, 62 };
        panel_ = { pad, 98, W - 2*pad, H - 98 - pad };

        const float r = 26.0f;
        const float col3 = panel_.w / 3.0f;
        const float row1 = panel_.y + 44.0f;
        const float row2 = row1 + 82.0f;
        // row 1: Volume, Depth, Tension
        for (int i = 0; i < 3; ++i) { wheels_[i].cx = panel_.x + col3*(i+0.5f); wheels_[i].cy = row1; wheels_[i].r = r; }
        // row 2: Speed (left third), Phase (mid third)
        wheels_[3].cx = panel_.x + col3*0.5f; wheels_[3].cy = row2; wheels_[3].r = r;
        wheels_[4].cx = panel_.x + col3*1.5f; wheels_[4].cy = row2; wheels_[4].r = r;
        // shape selector: right third, 2x2 buttons
        const float sx = panel_.x + col3*2.0f + 8.0f, sw = col3 - 20.0f;
        const float sh = 22.0f, gap = 6.0f, sbw = (sw - gap) / 2.0f;
        const float sy = row2 - 34.0f;
        seg_[0] = { sx,               sy,             sbw, sh };
        seg_[1] = { sx + sbw + gap,   sy,             sbw, sh };
        seg_[2] = { sx,               sy + sh + gap,  sbw, sh };
        seg_[3] = { sx + sbw + gap,   sy + sh + gap,  sbw, sh };
    }

    // ---------- draw ----------
    void drawTitle(visage::Canvas& c) {
        if (!fonts_ready_) return;
        c.setColor(kText);   c.text("PEAK LFO",  title_font_, visage::Font::kCenter, 12, 5, 160, 18);
        c.setColor(kDim);    c.text("volume lfo", label_font_, visage::Font::kCenter, (float)width()-160, 7, 148, 14);
    }

    void panelBox(visage::Canvas& c, const Rect& p) {
        c.setColor(kPanel);  c.rectangle(p.x,p.y,p.w,p.h); c.fill(p.x,p.y,p.w,p.h);
        c.setColor(kBorder); c.rectangleBorder(p.x,p.y,p.w,p.h,1.0f);
        if (fonts_ready_) { c.setColor(kDim); c.text("LFO", label_font_, visage::Font::kCenter, p.x+4, p.y+4, 60, 14); }
    }

    void drawScope(visage::Canvas& c) {
        const Rect& s = scope_;
        c.setColor(kPanel);  c.rectangle(s.x,s.y,s.w,s.h); c.fill(s.x,s.y,s.w,s.h);
        c.setColor(kBorder); c.rectangleBorder(s.x,s.y,s.w,s.h,1.0f);
        // one LFO cycle
        c.setColor(kGreen);
        const int steps = 96; float lx=s.x, ly=s.y+s.h*0.5f;
        for (int i=0;i<=steps;++i){ float t=i/(float)steps; float v=shapeSample(shape_,t);
            float px=s.x+t*s.w, py=s.y+s.h*0.5f - v*(s.h*0.40f);
            if(i>0) c.segment(lx,ly,px,py,1.5f,true); lx=px; ly=py; }
        // current gain level bar (right edge)
        const float bw=6.0f, bx=s.x+s.w-bw-3.0f;
        c.setColor(kWheel); c.rectangle(bx,s.y+3,bw,s.h-6); c.fill(bx,s.y+3,bw,s.h-6);
        c.setColor(kYellow); float gh=(s.h-6)*std::clamp(meter_gain_,0.0f,1.0f);
        c.rectangle(bx,s.y+s.h-3-gh,bw,gh); c.fill(bx,s.y+s.h-3-gh,bw,gh);
    }

    void drawWheel(visage::Canvas& c, const Wheel& k) {
        c.setColor(kWheel);  c.circle(k.cx-k.r,k.cy-k.r,k.r*2.0f);
        c.setColor(k.dragging?0xff5a616a:kRing); c.ring(k.cx-k.r,k.cy-k.r,k.r*2.0f,2.0f);
        // value arc (fill from min, or from centre for bipolar) — approximated by short segments
        const float deg = 3.14159265f/180.0f;
        auto ang = [&](float v){ return (-225.0f + 270.0f*v) * deg; };
        const float startV = (k.type==Type::Bipolar) ? 0.5f : 0.0f;
        const float rr = k.r*0.9f;
        const float a0 = ang(startV), aEnd = ang(k.value01);
        const int n = std::max(1, (int)std::ceil(std::abs(k.value01-startV)*28.0f));
        c.setColor(kGreen);
        float px=k.cx+std::cos(a0)*rr, py=k.cy+std::sin(a0)*rr;
        for (int i=1;i<=n;++i){ float aa=a0+(aEnd-a0)*(float)i/n; float x=k.cx+std::cos(aa)*rr, y=k.cy+std::sin(aa)*rr;
            c.segment(px,py,x,y,2.5f,true); px=x; py=y; }
        // pointer
        const float a = ang(k.value01);
        const float len=k.r*0.72f;
        c.setColor(k.dragging?kYellow:kGreen);
        c.segment(k.cx, k.cy, k.cx+std::cos(a)*len, k.cy+std::sin(a)*len, 2.0f, true);
        if (fonts_ready_) {
            c.setColor(kText); c.text(k.label, value_font_, visage::Font::kCenter, k.cx-k.r-6, k.cy+k.r+3, k.r*2.0f+12, 14);
            c.setColor(kDim);
            const std::string val = formatValue(k);
            c.text(val.c_str(), value_font_, visage::Font::kCenter, k.cx-k.r-10, k.cy+k.r+17, k.r*2.0f+20, 14);
        }
    }

    void drawShapeSelector(visage::Canvas& c) {
        const char* nm[4]={"Sin","Tri","Sqr","Rnd"};
        if (fonts_ready_) { c.setColor(kDim); c.text("SHAPE", value_font_, visage::Font::kCenter, seg_[0].x-4, seg_[0].y-16, 120, 14); }
        for (int s=0;s<4;++s){ const Rect& r=seg_[s]; bool on=(s==shape_);
            c.setColor(on?kGreen:kWheel); c.rectangle(r.x,r.y,r.w,r.h); c.fill(r.x,r.y,r.w,r.h);
            c.setColor(kBorder); c.rectangleBorder(r.x,r.y,r.w,r.h,1.0f);
            if (fonts_ready_){ c.setColor(on?kBg:kDim); c.text(nm[s], value_font_, visage::Font::kCenter, r.x, r.y+4, r.w, 14);} }
    }

    // ---------- value / hit ----------
    std::string formatValue(const Wheel& k) const {
        char b[24];
        switch (k.type) {
            case Type::Percent: std::snprintf(b,sizeof(b),"%d%%", (int)std::lround(k.value01*100.0f)); break;
            case Type::Bipolar: std::snprintf(b,sizeof(b),"%+d", (int)std::lround((k.value01*2.0f-1.0f)*100.0f)); break;
            case Type::Speed: { static const char* s[10]={"1/2","1","2","3","4","8","16","32","64","128"};
                                int i=std::clamp((int)std::lround(k.value01*9.0f),0,9); std::snprintf(b,sizeof(b),"%s st",s[i]); break; }
            case Type::Phase: { static const int p[4]={0,25,50,75};
                                int i=std::clamp((int)std::lround(k.value01*3.0f),0,3); std::snprintf(b,sizeof(b),"%d%%",p[i]); break; }
        }
        return b;
    }
    void setWheelValue(int i, float v01, bool fromUser) {
        if (i<0||i>=kNumWheels) return;
        if (wheels_[i].dragging && !fromUser) return;
        wheels_[i].value01 = std::clamp(v01,0.0f,1.0f);
        if (fromUser && on_param_) on_param_(static_cast<ParamId>(i), wheels_[i].value01);
        redraw();
    }
    int hitWheel(float x, float y) const {
        for (int i=0;i<kNumWheels;++i){ float dx=x-wheels_[i].cx, dy=y-wheels_[i].cy;
            if (dx*dx+dy*dy <= wheels_[i].r*wheels_[i].r) return i; }
        return -1;
    }
    static bool hit(const Rect& r, float x, float y) { return x>=r.x&&x<=r.x+r.w&&y>=r.y&&y<=r.y+r.h; }

    static float shapeSample(int shape, float t) {
        switch (shape) {
            case 0: return std::sin(2.0f*3.14159265f*t);
            case 1: return t<0.25f? 4*t : t<0.75f? 2-4*t : 4*t-4;
            case 2: return t<0.5f? 1.0f : -1.0f;
            default: { unsigned int h=(unsigned int)(t*16.0f); h=h*1664525u+1013904223u; return (float)((int)(h>>9)/4194303.0-1.0); }
        }
    }

    void updateFonts() {
        const float dpi = std::max(1.0f, dpiScale());
        const auto* fd = reinterpret_cast<const unsigned char*>(PeakLFO_BinaryData::LatoRegular_ttf);
        title_font_ = visage::Font(18.0f, fd, PeakLFO_BinaryData::LatoRegular_ttfSize, dpi);
        label_font_ = visage::Font(12.0f, fd, PeakLFO_BinaryData::LatoRegular_ttfSize, dpi);
        value_font_ = visage::Font(11.0f, fd, PeakLFO_BinaryData::LatoRegular_ttfSize, dpi);
        fonts_ready_ = true;
    }

    // ---------- state ----------
    bool fonts_ready_=false;
    visage::Font title_font_, label_font_, value_font_;
    std::array<Wheel, kNumWheels> wheels_ {};
    Rect title_, scope_, panel_, seg_[4];
    int shape_=0;
    float meter_lfo_=0.5f, meter_gain_=1.0f;
    int active_=-1; float drag_y_=0, drag_v_=0;
    float laidW_=-1, laidH_=-1;
    ParamChangeFn on_param_; ShapeChangeFn on_shape_;
};
