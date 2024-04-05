#pragma once

#include "blend2d.h"

// OF Types
#include "ofTexture.h"
#include "ofFileUtils.h" // ofBuffer
#include "ofColor.h"
#include "ofPath.h"
#include "ofLog.h"


// OF Glue
// - - - -
//inline constexpr BLRgba32 toBLColor(const ofFloatColor& _c); // todo: make this olColor_ templated ?
//inline constexpr BLRgba32 toBLColor(ofFloatColor const& _c); // todo: make this olColor_ templated ?
inline constexpr BLRgba32 toBLColor(ofFloatColor const& _c, float _forceAlpha=-1.f){
    return BLRgba32(_c.r*255, _c.g*255, _c.b*255, (_forceAlpha<0)?(_c.a*255):(_forceAlpha));
}

inline constexpr BLPoint toBLPoint(glm::vec2 const& _p){
    return BLPoint(_p.x, _p.y);
}
inline constexpr BLPoint toBLPoint(glm::vec3 const& _p){
    return BLPoint(_p.x, _p.y);
}

inline BLPath toBLPath(ofPath const& _p){
    if(_p.getMode()!=ofPath::Mode::COMMANDS){
        ofLogWarning("ofxBlend2D::toBLPath") << "The path is not in CMD mode, returning an empty BLPath !";
    }
    bool warnUnsupportedCmd = false;
    BLPath ret;
    for( const ofPath::Command& cmd : _p.getCommands()){
        // Warning: ofxSVG, via tinyxml, doesn't reveal all SVG commands, some are converted, some are ignored !
        switch(cmd.type){
            case ofPath::Command::Type::moveTo:
                // First point of a shape
                ret.moveTo( toBLPoint(cmd.to) );
                break;
            case ofPath::Command::Type::lineTo:
                ret.lineTo(toBLPoint(cmd.to));
                break;
            case ofPath::Command::Type::curveTo:
                ret.lineTo(toBLPoint(cmd.to)); // todo ! (uses a line instead of an arc)
                warnUnsupportedCmd = true;
                break;
            case ofPath::Command::Type::bezierTo:
                ret.cubicTo(toBLPoint(cmd.cp1), toBLPoint(cmd.cp2), toBLPoint(cmd.to));
                break;
            case ofPath::Command::Type::quadBezierTo:
                warnUnsupportedCmd = true;
                //ret.quadTo(toBLPoint(cmd.cp1),toBLPoint(cmd.cp2),cmd.);
                break;
            case ofPath::Command::Type::arc:
                warnUnsupportedCmd = true;
                //ret.arc(cmd.to,cmd.radiusX,cmd.radiusY,cmd.angleBegin,cmd.angleEnd);
                break;
            case ofPath::Command::Type::arcNegative:
                warnUnsupportedCmd = true;
                //ret.arcNegative(cmd.to,cmd.radiusX,cmd.radiusY,cmd.angleBegin,cmd.angleEnd);
                break;
            case ofPath::Command::Type::close:
                ret.close();
                break;
        }
    }
    if(warnUnsupportedCmd){
        ofLogWarning("ofxBlend2D::toBLPath") << "The path contains a yet unimplemented CMD, the conversion was lossy !";
    }
    return ret;
}
