#include "ofApp.h"
#include "blend2d/blend2d-debug.h"

//--------------------------------------------------------------
void ofApp::setup(){
    ofSetWindowTitle("ofxBlend2D :: example-simple");

    blend2d.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);

    // Show Blend2d build info
    bl_debug_runtime_build_info();
}

//--------------------------------------------------------------
void ofApp::update(){
    // Flush the Blend2D pipeline from update()
    blend2d.update();
}

//--------------------------------------------------------------
void ofApp::draw(){
    // Data : p=point, h=handle (bezier)
    float variation = sin(ofGetElapsedTimef()*.5)*.5+.5;
    const glm::vec2 linePos(400, 40+variation*400);
    const static glm::vec2 p1(119, 49);
    const static glm::vec2 p2(275, 267);
    const static glm::vec2 p3(274, 430);
    const static glm::vec2 h1(259, 29);
    const static glm::vec2 h2(99, 279);
    const static glm::vec2 h3(537, 245);
    const static glm::vec2 h4(300, -170);

    const static ofRectangle rect1(40, 40, 440, 440);
    BLGradient linear(BLLinearGradientValues(rect1.x, rect1.y, rect1.width+rect1.x, rect1.height+rect1.x));
    linear.add_stop(0.0, BLRgba32(0xFFFFFFFF));
    linear.add_stop(variation, BLRgba32(0xFF5FAFDF));
    linear.add_stop(1.0, BLRgba32(0xFF2F5FDF));

    // - - - - - -
    // This displays the latest rendered texture, from a previous frame
    ofTexture& blTex = blend2d.getTexture();
    if(blTex.isAllocated()){
        ofFill();
        ofSetColor(ofColor::white);
        blTex.draw(0,0);
    }

    // - - - - - -
    // Help
    ofDrawBitmapStringHighlight("This examples demonstrates how to use the plain C++ Blend2D API.", {20,20});

    // - - - - - -
    // You can submit to the Blend2D API from anywhere, even a non-gl thread !
    // [The block block can either be in update(), in draw() or a thread]
    // BUT! You can get the context only if begin() returns true (a new frame is available for rendering)
    // Calling blend2d.getBlContext() right here will assert() !
    //blend2d.getBlContext();
    if(blend2d.begin()){
        // This scope will not be called every frame !

        // - - - - - -
        // Get the context
        BLContext ctx = blend2d.getBlContext();

        // Set style and draw the blend2d "getting started" gradient shape
        ctx.set_fill_style(linear);
        ctx.fill_round_rect(rect1.x, rect1.y, rect1.width, rect1.height, 45.5*variation);

        // You can also add a shape and a color
        BLRoundRect rect2(rect1.x*2+rect1.width, rect1.y*2+rect1.height, rect1.width, rect1.height, 45.5*variation);
        BLGradient radial(BLRadialGradientValues(rect2.x+rect2.w*.5, rect2.y+rect2.h*.5, rect2.x+rect2.w*.5, rect2.y+rect2.h*.5, rect2.w*variation));
        radial.add_stop(0.0, BLRgba32(0xFF00FF00));
        radial.add_stop(1.0, BLRgba32(0xFF0000FF));

        // Filled with a gradient
        ctx.fill_round_rect(rect2, radial);

        // The same shape but stroked
        ctx.stroke_round_rect(rect2, BLRgba32(0xFF000000));

        // - - - - - -
        // BL path construction is quite similar to ofPath, but without the rendering styles
        BLPath line;
        line.move_to(p1.x, p1.y);
        line.cubic_to(h1.x, h1.y, h2.x, h2.y, p2.x, p2.y);
        line.cubic_to(h3.x, h3.y, h4.x, h4.y, p3.x, p3.y);

        // Set some (advanced) stroking options
        ctx.set_stroke_width(15);
        ctx.set_stroke_start_cap(BL_STROKE_CAP_ROUND);
        ctx.set_stroke_end_cap(BL_STROKE_CAP_BUTT);

        // There's also matrix support like ofTransform(), scale, etc
        ctx.translate(linePos.x, linePos.y);

        // And composing options (blending modes)
        BLCompOp curCompOp = (BLCompOp)(uint32_t)round(glm::mod(ofGetElapsedTimef()*0.03, 1.)*(uint32_t)(BL_COMP_OP_MAX_VALUE-1)); // Randomly pick a compOp
        ctx.set_comp_op(curCompOp);
        // (or use their names)
        //ctx.setCompOp(BL_COMP_OP_DIFFERENCE);

        // There's a small helper to convert ofColor to BLColor
        ctx.stroke_path(line, toBLColor(ofFloatColor::purple));

        // Restore context
        ctx.translate(-linePos.x, -linePos.y);
        ctx.set_comp_op(BL_COMP_OP_SRC_OVER);

        // - - - - - -
        // There's also some other shapes available
        ctx.translate(rect1.x, rect1.y*2+rect1.height);
        ctx.rotate(variation*TWO_PI);
        ctx.fill_triangle(0,0, 400, 0, 200, 400, toBLColor(ofFloatColor::orange));

        unsigned int frameNum = ofGetFrameNum();
        blend2d.end(frameNum);
    }

    // - - - - - -
    // We can still draw to OF at a full framerate
    // Draw the beziers
    ofPushMatrix();
    ofTranslate(linePos);
    ofNoFill();
    ofSetColor(ofColor::black);
    ofSetLineWidth(1);
    ofDrawLine(p1, h1);
    ofDrawLine(p2, h2);
    ofDrawLine(p2, h3);
    ofDrawLine(p3, h4);
    ofFill();
    ofSetColor(ofColor::red);
    ofDrawCircle(p1, 3);
    ofDrawCircle(p2, 3);
    ofDrawCircle(p3, 3);
    ofDrawCircle(h1, 3);
    ofDrawCircle(h2, 3);
    ofDrawCircle(h3, 3);
    ofDrawCircle(h4, 3);
    ofPopMatrix();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
    blend2d.allocate(w,h);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
