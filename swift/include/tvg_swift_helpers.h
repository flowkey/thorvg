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

// Scene effect wrappers — Swift C++ interop cannot call variadic functions directly.
inline tvg::Result tvgSceneAddGaussianBlur(tvg::Scene* s, double sigma, int direction, int border, int quality) {
    return s->add(tvg::SceneEffect::GaussianBlur, sigma, direction, border, quality);
}
inline tvg::Result tvgSceneClearEffects(tvg::Scene* s) {
    return s->add(tvg::SceneEffect::Clear);
}

// Scene::add(Paint*, Paint*) has two overloads; the insert-before variant needs an explicit
// helper so Swift doesn't ambiguate with add(SceneEffect, ...).
inline tvg::Result tvgSceneAddPaintBefore(tvg::Scene* scene, tvg::Paint* target, tvg::Paint* before) {
    return scene->add(target, before);
}

// Picture::load(const uint32_t*, ...) — Swift C++ interop can't resolve the raw-pointer overload directly.
// Always copies (copy=true) so the caller's buffer can be freed after the call.
inline tvg::Result tvgPictureLoadPixels(tvg::Picture* pic, const uint32_t* data, uint32_t w, uint32_t h) {
    return pic->load(data, w, h, tvg::ColorSpace::ARGB8888, true);
}

// Paint::opacity(uint8_t) setter — Swift C++ interop only sees the const getter overload.
inline tvg::Result tvgPaintSetOpacity(tvg::Paint* p, uint8_t opacity) {
    return p->opacity(opacity);
}
