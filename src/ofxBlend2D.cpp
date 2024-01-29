#include "ofxBlend2D.h"
#include "ofAppRunner.h"
#include "ofUtils.h"
#include "ofLog.h"
#include <iostream>
#include "ofImage.h"

ofxBlend2DThreadedRenderer::ofxBlend2DThreadedRenderer(){
    createInfo.threadCount = 6; // Number of threads
    //createInfo.flags =

    codec.findByName("BMP"); // Sets codec to BMP

    allocate(ofGetWidth(), ofGetHeight());

    startBlThread();
}

ofxBlend2DThreadedRenderer::~ofxBlend2DThreadedRenderer() {
	stopBlThread();
	pixelDataFromThread.close();
	flushFrameSignal.close();
	waitForThread(false);
}

void ofxBlend2DThreadedRenderer::startBlThread(){
	startThread();
}

void ofxBlend2DThreadedRenderer::stopBlThread(){
	std::unique_lock<std::mutex> lck(mutex);
	stopThread();
}

// todo: destructor --> blImageCodecDestroy(&codec); ???

void ofxBlend2DThreadedRenderer::allocate(int _width, int _height){
    // todo: wait for thread first ?

    width = _width;
    height = _height;
    tex.allocate(_width, _height, GL_RGBA);
}

bool ofxBlend2DThreadedRenderer::begin(){
    assert(!isSubmittingDrawCmds); // begin() / end() call order mismatch !

    // Skip when dirty = let update()/thread finish the update ?
    if(isDirty){
#ifdef ofxBlend2D_DEBUG
        std::cout << "Skipping blend2d frame ! (thread not done yet)" << std::endl;
#endif
        return false;
    }

    // Init output image canvas
    img = BLImage(width, height, BL_FORMAT_PRGB32);

    // Create context for this image
    BLResult result = ctx.begin(img, createInfo);

    // Success ?
    if (result != BL_SUCCESS){
        ofLogError("ofxBlend2D::begin()") << "Error creating context !";
        return false;
    }

#ifdef ofxBlend2D_DEBUG
    std::cout << "Begin() : Building new ctx ! :D" << " dirty=" << isDirty << std::endl;
#endif
    // Remember
    isSubmittingDrawCmds = true;

    // Init context
    ctx.setRenderingQuality(BLRenderingQuality::BL_RENDERING_QUALITY_ANTIALIAS);

    // Todo: make this optional ?
    ctx.clearAll();
    ctx.fillAll(BLRgba32(255,255,255,0));

    return true;
}

bool ofxBlend2DThreadedRenderer::end(){
    assert(isSubmittingDrawCmds); // begin() / end() call order mismatch !
    isSubmittingDrawCmds = false;
    isDirty = true;

#ifdef ofxBlend2D_DEBUG
    std::cout << "GLThread generated new frame data !" << " dirty=" << isDirty << std::endl;
#endif
    flushFrameSignal.send(true);

    return true;
}

void ofxBlend2DThreadedRenderer::update(){
    assert(!isSubmittingDrawCmds);

    if(isDirty){

        bool newFrame = false;
        static ofBuffer bufFromThread;
        while(pixelDataFromThread.tryReceive(bufFromThread, 1)){ // Empty queue until most recent image to grab
            newFrame = true;
        }
        if(newFrame){
            // Start timer
            fpsCounter.begin();

            // Blocks ! Wait for render queue to finish
            // Note: should be flushed already; just calling in case...
            ctx.flush(BLContextFlushFlags::BL_CONTEXT_FLUSH_SYNC);

            // Grab the result
            //tex.allocate(width, height, GL_RGBA); // not needed, ofLoadImage seems to do it for us. ??
            ofLoadImage(tex, bufFromThread);

            // Close & release context
            ctx.end();

            isDirty = false;
            renderedFrames++;

            // Update timer
            fpsCounter.end();
            for(int i=0; i<ofxBlend2D_FPS_HISTORY_SIZE-1; ++i){
                fpsCounterHist[i]=fpsCounterHist[i+1];
            }

            fpsCounterHist[ofxBlend2D_FPS_HISTORY_SIZE-1] = fpsCounter.getFps();
#ifdef ofxBlend2D_DEBUG
            std::cout << "Update() -> Received a new frame !" << " dirty=" << isDirty << std::endl;
#endif
        }
#ifdef ofxBlend2D_DEBUG
        else std::cout << "Update() : Skipping, no new frame received yet." << " dirty=" << isDirty << std::endl;
#endif
    }
}

//bool ofxBlend2DThreadedRenderer::syncTexture(){
//    assert(!isSubmittingDrawCmds);

//    BLArray<uint8_t> resultData;
//    BLResult resultWrite = img.writeToData(resultData, codec);

//    if(resultWrite != BL_SUCCESS){
//        // failed !
//        ofLogWarning("ofxBlend2DThreadedRenderer::syncTexture()") << "Couldn't load texture! Error=" << resultWrite << " and " << getContextErrors();
//        return false;
//    }

//    //ofTexture tex;
//    ofBuffer b((char*)const_cast<unsigned char*>(resultData.data()), resultData.size());
//    //tex.allocate(, GL_RGBA); // not needed, ofLoadImage seems to do it for us.
//    ofLoadImage(tex, b);
//    return true;
//}

BLContext& ofxBlend2DThreadedRenderer::getBlContext(){
    assert(isSubmittingDrawCmds); // Only accessible between begin() and end() calls !
    return ctx;
}

std::string ofxBlend2DThreadedRenderer::getContextErrors(){
    BLContextErrorFlags errorFlags = ctx.accumulatedErrorFlags();
    std::ostringstream ret;
    ret << "Context_Error_Flags=" << errorFlags << " (";

    //! The rendering context returned or encountered `BL_ERROR_INVALID_VALUE`, which is mostly related to the function
    //! argument handling. It's very likely some argument was wrong when calling `BLContext` API.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_INVALID_VALUE) ret << "INVALID_VALUE, ";

    // Invalid state describes something wrong, for example a pipeline compilation error.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_INVALID_STATE) ret << "INVALID_STATE, ";

    //! The rendering context has encountered invalid geometry.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_INVALID_GEOMETRY) ret << "INVALID_GEOMETRY, ";

    //! The rendering context has encountered invalid glyph.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_INVALID_GLYPH) ret << "INVALID_GLYPH, ";

    //! The rendering context has encountered invalid or uninitialized font.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_INVALID_FONT) ret << "INVALID_FONT, ";

    //! Thread pool was exhausted and couldn't acquire the requested number of threads.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_THREAD_POOL_EXHAUSTED) ret << "THREAD_POOL_EXHAUSTED, ";

    //! Out of memory condition.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_OUT_OF_MEMORY) ret << "OUT_OF_MEMORY, ";

    //! Unknown error, which we don't have flag for.
    if(errorFlags & BL_CONTEXT_ERROR_FLAG_UNKNOWN_ERROR) ret << "UNKNOWN_ERROR, ";

    ret << ")";
    return ret.str();
}

ofTexture& ofxBlend2DThreadedRenderer::getTexture(){
    return tex;
}

float ofxBlend2DThreadedRenderer::getFps() {
    return fpsCounter.getFps();
}

const float& ofxBlend2DThreadedRenderer::getFpsHist() const{
    return fpsCounterHist[0];
}

float ofxBlend2DThreadedRenderer::getSyncTime() {
    return fpsCounter.getFrameTimef();
}

unsigned int ofxBlend2DThreadedRenderer::getRenderedFrames() const {
    return renderedFrames;
}


void ofxBlend2DThreadedRenderer::threadedFunction(){

    bool newFrame = false;
    ofBuffer pixelDataInThread;

    while(flushFrameSignal.receive(newFrame)){
#ifdef ofxBlend2D_DEBUG
        std::cout << "Thread : encoding new frame !" << std::endl;
#endif

        // Lock mutex : from here, we can use the protected vars (expecting that the gl thread doesn't use them anymore)
        // Protected vars : ctx, , all linked BLPaths
        std::unique_lock<std::mutex> lock(mutex);


        // Flush the blend2d pipeline !
        // Blocks ! Waits for threaded render queue to finish
        ctx.flush(BLContextFlushFlags::BL_CONTEXT_FLUSH_SYNC);

        // Grab the result
        BLArray<uint8_t> resultData;
        BLResult resultWrite = img.writeToData(resultData, codec);

        if(resultWrite != BL_SUCCESS){
            ofLogWarning("ofxBlend2DThreadedRenderer") << "Couldn't load texture! Error=" << resultWrite << " and " << getContextErrors();
            // TODO : handle error in thread ?
        }
        else {
            pixelDataInThread = ofBuffer((char*)const_cast<unsigned char*>(resultData.data()), resultData.size());
        }


#if __cplusplus>=201103
		pixelDataFromThread.send(std::move(pixelDataInThread));
#else
		pixelDataFromThread.send(pixelDataInThread);
#endif
	}
}
