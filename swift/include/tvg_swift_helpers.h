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
