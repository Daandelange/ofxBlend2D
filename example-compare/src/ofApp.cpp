#include "ofApp.h"


//--------------------------------------------------------------
void ofApp::setup(){
    // OF setup
    ofSetWindowTitle("ofxBlend2D :: example-compare");
    ofSetVerticalSync(true);

    // Gui Setup
    gui.setup();
    gui.add( useBlend2DForRendering.setup("Render using ofxBlend2D", true) );
    gui.add( numThreads.setup("Blend2D threads", 4, 1, 16) );
    gui.add( cols.setup("Columns", 20, 1, 5000) );
    gui.add( rows.setup("Rows", 20, 1, 5000) );
    gui.add( shapeColor.setup("Shape Color", ofFloatColor::orange, ofFloatColor(0, 0), ofFloatColor(1, 1)) );
    //gui.add( drawFilled.setup("Fill shapes", true) );
    //gui.add( drawStroked.setup("Stroke shapes", false) );
    gui.add( doAnimate.setup("Animate Shapes", false) );
    gui.add( showGrid.setup("Show Shape Grid", false) );

    numThreads.addListener(this, &ofApp::onThreadsChanged);

    // Blend2D setup
    blend2d.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA); // RGBA to stay identical to OF renderer

    // Define shape
    const double shapeSize = 50;
    ofVec2f p1(shapeSize*.1, shapeSize*.1);
    ofVec2f p2(shapeSize*.9, shapeSize*.1);
    ofVec2f p3(shapeSize*.9, shapeSize*.9);
    ofVec2f p4(shapeSize*.1, shapeSize*.9);
    ofVec2f h1i(p1.x-shapeSize*.3, p1.y-shapeSize*.0);
    ofVec2f h1o(p1.x+shapeSize*.3, p1.y-shapeSize*.0);
    ofVec2f h2i(p2.x-shapeSize*.0, p2.y-shapeSize*.3);
    ofVec2f h2o(p2.x-shapeSize*.0, p2.y+shapeSize*.3);
    ofVec2f h3i(p3.x+shapeSize*.3, p3.y-shapeSize*.0);
    ofVec2f h3o(p3.x-shapeSize*.3, p3.y-shapeSize*.0);
    ofVec2f h4i(p4.x-shapeSize*.0, p4.y+shapeSize*.3);
    ofVec2f h4o(p4.x-shapeSize*.0, p4.y-shapeSize*.3);

    // Load shapes in memory
    blShape.moveTo(p1.x, p1.y);
    blShape.cubicTo(h1o.x, h1o.y, h2i.x, h2i.y, p2.x, p2.y);
    blShape.cubicTo(h2o.x, h2o.y, h3i.x, h3i.y, p3.x, p3.y);
    blShape.cubicTo(h3o.x, h3o.y, h4i.x, h4i.y, p4.x, p4.y);
    blShape.cubicTo(h4o.x, h4o.y, h1i.x, h1i.y, p1.x, p1.y);
    blShape.close();

    ofShape.setFilled(true);
    ofFloatColor ofCol = (const ofColor) shapeColor;
    ofShape.setColor(shapeColor);
    ofShape.moveTo(p1.x, p1.y);
    ofShape.bezierTo(h1o.x, h1o.y, h2i.x, h2i.y, p2.x, p2.y);
    ofShape.bezierTo(h2o.x, h2o.y, h3i.x, h3i.y, p3.x, p3.y);
    ofShape.bezierTo(h3o.x, h3o.y, h4i.x, h4i.y, p4.x, p4.y);
    ofShape.bezierTo(h4o.x, h4o.y, h1i.x, h1i.y, p1.x, p1.y);
    ofShape.close();

    ofEnableBlendMode(OF_BLENDMODE_ALPHA);

    TIME_SAMPLE_SET_FRAMERATE(60.0f); //specify a target framerate
}

//--------------------------------------------------------------
void ofApp::update(){

    // Note: hasNewFrame is for not measuring the ±0 time when there's no new frame
    if(useBlend2DForRendering && blend2d.hasNewFrame()){
        TS_STOP_NIF("Blend2D/RendererDelay");

        TS_START("Blend2D/UploadTexture(CPU)");
        TSGL_START("Blend2D/UploadTexture(GPU)");

        // Flush the Blend2D pipeline from update()
        if(blend2d.update(false)){
            // Track Blend2d new frame
            blRendererFps.begin();
            blRendererFps.end();
        }

        TSGL_STOP("Blend2D/UploadTexture(GPU)");
        TS_STOP("Blend2D/UploadTexture(CPU)");
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    // Clean frame
    ofClearAlpha();

    // Pre-compute
    unsigned int stepX = glm::max(1.0, (ofGetWindowWidth() / (double)cols));
    unsigned int stepY = glm::max(1.0, (ofGetWindowHeight() / (double)rows));
    float loopProgress = glm::mod(ofGetElapsedTimef()/5.f, 1.f);

    // - - - - - -
    // Blend2D rendering
    if(useBlend2DForRendering){

        // Submit draw calls
        bool didNewFrame = false;
        if(blend2d.begin()){
            // This scope will not be called every frame !
            TS_START("Blend2D/SubmitGeometry");

            // - - - - - -
            // Get the context
            BLContext ctx = blend2d.getBlContext();
            ctx.setCompOp(BL_COMP_OP_SRC_OVER); // match OF's comp mode
            auto blColor = toBLColor(ofFloatColor((const ofColor) shapeColor));
            // Draw shapes
            for(unsigned int posY=0; posY<rows; posY+=1){
                if(posY > 0) ctx.translate(0, stepY);
                for(unsigned int posX=0; posX<cols; posX+=1){
                    if(posX==0){
                        if(posY != 0) ctx.translate(-1.0*((cols-1)*stepX), 0);
                    }
                    else ctx.translate(stepX, 0);

                    // Draw the shape !
                    if(doAnimate) ctx.rotate(loopProgress*TWO_PI);
                    ctx.fillPath(blShape, blColor);
                    if(doAnimate) ctx.rotate(loopProgress*TWO_PI*-1);
                }
            }

            unsigned int frameNum = ofGetFrameNum();

            if(bSaveNextFrame){
                blend2d.end(frameNum, "lastFrame_blend2d.png");
                bSaveNextFrame = false;
            }
            else blend2d.end(frameNum);

            TS_STOP("Blend2D/SubmitGeometry");

            didNewFrame = true;
        }

        // Draw Blend2D texture
        TS_START("Blend2D/DrawTexture");
        TSGL_START("Blend2D/DrawTexture(GPU)");

        ofTexture& blTex = blend2d.getTexture();
        if(blTex.isAllocated() && blTex.getWidth() > 0){
            ofFill();
            ofSetColor(ofColor::white);
            // because Blend2D pre-multiplies alpha !
            // See: https://apoorvaj.io/alpha-compositing-opengl-blending-and-premultiplied-alpha/
            glBlendEquation(GL_FUNC_ADD);
            glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
            blTex.draw(0,0);
            ofEnableBlendMode(OF_BLENDMODE_ALPHA); // reset blend mode. fixme: what if user changed default blend mode ???
        }

        TSGL_STOP("Blend2D/DrawTexture(GPU)");
        TS_STOP("Blend2D/DrawTexture");

        if(didNewFrame){
            TS_START_NIF("Blend2D/RendererDelay");
        }
    }

    // - - - - - -
    // OpenFrameworks rendering
    else {
        TS_SCOPE("OpenFrameworks/SubmitGeometry");
        TSGL_START("OpenFrameworks/DrawGeometry(GPU)");

        ofPushStyle();
        ofFill();
        ofShape.setColor(shapeColor);
        ofPushMatrix();

        for(unsigned int posY=0; posY<rows; posY+=1){
            if(posY > 0) ofTranslate(0, stepY);
            for(unsigned int posX=0; posX<cols; posX+=1){
                if(posX==0){
                    if(posY != 0) ofTranslate(-1.0*((cols-1)*stepX), 0);
                }
                else ofTranslate(stepX, 0);

                if(doAnimate) ofRotateRad(loopProgress*TWO_PI);
                ofShape.draw();
                if(doAnimate) ofRotateRad(loopProgress*TWO_PI*-1);
            }
        }

        ofPopMatrix();
        ofPopStyle();

        TSGL_STOP("OpenFrameworks/DrawGeometry(GPU)");

        // Save OF frame
        if(bSaveNextFrame){
            // Todo: Move this to ofFbo to also save transparency.
            ofSaveViewport("lastFrame_openframeworks.png");
            bSaveNextFrame = false;
        }
    }

    // Helper Grid
    if(showGrid){
        ofPushStyle();
        ofNoFill();
        ofSetLineWidth(2);
        ofSetColor(ofColor::black, 100);
        for(unsigned int posY=0; posY<rows; posY+=1){
            ofDrawLine(0, posY*stepY, ofGetWindowWidth(), posY*stepY);
        }
        for(unsigned int posX=0; posX<cols; posX+=1){
            ofDrawLine(posX*stepX, 0, posX*stepX, ofGetWindowHeight());
        }
        ofPopStyle();
    }


    // - - - - - -
    // Help
    int yPos = 0;
    ofDrawBitmapStringHighlight("This examples compares Blend2D rendering with OpenFrameworks rendering in terms of performance and graphical quality.", {20, yPos+=20});
    ofDrawBitmapStringHighlight("You can toggle rendering to compare the resulting FPS by pressing X.", {20, yPos+=20});
    yPos+=10;
    ofDrawBitmapStringHighlight( ofToString("       Blend2D FPS = ")+blRendererFps.toString(), {20,yPos+=20});
    ofDrawBitmapStringHighlight( ofToString("OpenFrameworks FPS = ")+ofToString(ofGetFrameRate()), {20, yPos+=20});
    yPos+=10;
    ofDrawBitmapStringHighlight( "Press space to record 1 frame as a PNG file in the data folder.", {20, yPos+=20});

    // Non-release optimisations warning
#ifdef DEBUG
    ofDrawBitmapStringHighlight("Warning, this is a non-release build, Blend2D is less performant !", {ofGetScreenWidth()*.5f-200, ofGetScreenHeight()*.5f});
#endif

    // Draw GUI
    gui.draw();

}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
    if(key==' '){
        bSaveNextFrame = true;
    }
    else if(key=='x'){
        useBlend2DForRendering = !useBlend2DForRendering;

        // Make current texture transparent to prevent a glitch when the old frame is displayed
        if(useBlend2DForRendering){
            blend2d.clearTexture();
        }
    }
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

    //screenSize = ofToString(w) + "x" + ofToString(h);
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
