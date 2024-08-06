#pragma once

#include "blend2d.h"

// OF Types
#include "ofMath.h"
#include "ofColor.h"
#include "ofPath.h"


// OF Glue
// - - - -
// todo: make this olColor_ templated ?
inline constexpr BLRgba32 toBLColor(ofFloatColor const& _c, float _forceAlpha=-1.f){
    // Note: Blend2d uses the BGRA notation
    return BLRgba32(255u*_c.b, 255u*_c.g, 255u*_c.r, (_forceAlpha<=0)?(255u*_c.a):(255u*_forceAlpha));
}

inline constexpr BLPoint toBLPoint(glm::vec2 const& _p){
    return BLPoint(_p.x, _p.y);
}
inline constexpr BLPoint toBLPoint(glm::vec3 const& _p){
    return BLPoint(_p.x, _p.y);
}

inline BLPath toBLPath(ofPath const& _p);

// Utility for making errors human-readable
std::string blResultToString(BLResult r);

// GL Glue
GLint blFormatToGlFormat(uint16_t blFormat);

// Missing in ofGLUtils : the reverse of ofGetGLFormatFromPixelFormat
// Note: lossy conversion, some glFormats have multiple corresponding ofFormats
ofPixelFormat ofxBlend2DGetOfPixelFormatFromGLFormat(const GLint glFormat);
