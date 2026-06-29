#pragma once
#include "../../inc/thorvg.h"

#ifndef SWIFT_RETURNS_UNRETAINED
#  if __has_attribute(swift_attr)
#    define SWIFT_RETURNS_UNRETAINED __attribute__((swift_attr("returns_unretained")))
#  else
#    define SWIFT_RETURNS_UNRETAINED
#  endif
#endif

// Upcasts (Swift cxx-interop doesn't do implicit C++ inheritance casts)
inline tvg::Paint* toPaint(tvg::Shape* p) SWIFT_RETURNS_UNRETAINED { return static_cast<tvg::Paint*>(p); }
inline tvg::Paint* toPaint(tvg::Scene* p) SWIFT_RETURNS_UNRETAINED { return static_cast<tvg::Paint*>(p); }
inline tvg::Paint* toPaint(tvg::Picture* p) SWIFT_RETURNS_UNRETAINED { return static_cast<tvg::Paint*>(p); }


inline void refPaint(tvg::Paint* p) { p->ref(); }
inline void refShape(tvg::Shape* p) { p->ref(); }
inline void refScene(tvg::Scene* p) { p->ref(); }
inline void refPicture(tvg::Picture* p) { p->ref(); }

inline void unrefPaint(tvg::Paint* p) { p->unref(); }
inline void unrefShape(tvg::Shape* p) { p->unref(); }
inline void unrefScene(tvg::Scene* p) { p->unref(); }
inline void unrefPicture(tvg::Picture* p) { p->unref(); }

// Initializer and SwCanvas destructor aren't directly callable from Swift.
inline void tvgInit() { tvg::Initializer::init(0); }
inline void tvgCanvasDestroy(tvg::SwCanvas* c) { delete c; }

// Animation is not a ref-counted Paint — Swift can call `tvg.Animation.gen()`
// directly via the apinotes immortal-reference import, but the destructor needs
// a free function (Swift cxx-interop can't call `delete` on an opaque pointer).
inline void tvgAnimationDestroy(tvg::Animation* a) { delete a; }

// Scene effect wrapper — Swift C++ interop cannot call variadic functions directly.
inline tvg::Result tvgSceneAddGaussianBlur(tvg::Scene* s, double sigma, int direction, int border, int quality) {
    return s->add(tvg::SceneEffect::GaussianBlur, sigma, direction, border, quality);
}

// Builds a rectangle [x, x+w] × [y, y+h] filled with a horizontal linear gradient, optionally masked
// to `maskScene`'s alpha (MaskMethod::Alpha) so the gradient shows only where the mask is opaque while
// keeping its own colour. Building the Fill/LinearGradient here keeps it off Swift, whose cxx-interop
// can't construct gradients or disambiguate mask()'s getter/setter overload. Returns an unretained Shape.
struct TvgGradientStop { float offset; uint8_t r, g, b, a; };

inline tvg::Shape* tvgMakeMaskedGradientRect(float x, float y, float w, float h,
                                             const TvgGradientStop* stops, uint32_t count,
                                             tvg::Scene* maskScene) SWIFT_RETURNS_UNRETAINED {
    auto rect = tvg::Shape::gen();
    rect->appendRect(x, y, w, h);
    auto gradient = tvg::LinearGradient::gen();
    gradient->linear(x, y, x + w, y);
    if (count > 16) count = 16;
    tvg::Fill::ColorStop cs[16];
    for (uint32_t i = 0; i < count; i++) {
        cs[i].offset = stops[i].offset;
        cs[i].r = stops[i].r; cs[i].g = stops[i].g; cs[i].b = stops[i].b; cs[i].a = stops[i].a;
    }
    gradient->colorStops(cs, count);
    gradient->spread(tvg::FillSpread::Pad);
    rect->fill(gradient);
    // Alpha mask (not Intersect, which would paint the mask's own colour) keeps the gradient's colour
    // and shows it only where the mask is opaque.
    if (maskScene) rect->mask(static_cast<tvg::Paint*>(maskScene), tvg::MaskMethod::Alpha);
    return rect;
}
