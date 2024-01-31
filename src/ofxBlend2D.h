#pragma once

#include "blend2d.h"
#include "ofTexture.h"
#include "ofxFps.h"

// Threads & locks
#include "ofThread.h"
#include "ofThreadChannel.h"
//#include <atomic>

// Tips:
// Blend2D debugging help : Set breakpoint @blTraceError in blend2d/src/api.h

// Mutex logics in this class :
// BLContext and BLxxx vars are protected by a channel mutex.
// The GL thread sends a message to the threaded worker, which handles the data and sends back the result
// BLTypes are only accessible between begin() and end() for thread safety.

#define ofxBlend2D_FPS_HISTORY_SIZE 120

// Uncomment or set compiler flag to enable ImGui helper widgets
//#define ofxBlend2D_ENABLE_IMGUI

// Uncomment to embed an fps counter !
//#define ofxBlend2D_ENABLE_OFXFPS

// Uncomment to enable very verbose threading debug messages
//#define ofxBlend2D_DEBUG

#ifdef ofxBlend2D_ENABLE_IMGUI
#include "imgui.h"
#endif

// OF Glue
// - - - -
inline constexpr BLRgba32 toBLColor(ofFloatColor const& _c){
    return BLRgba32(_c.r*255, _c.g*255, _c.b*255, _c.a*255);
}

class ofxBlend2DThreadedRenderer : protected ofThread {

    public:
        ofxBlend2DThreadedRenderer();
        ~ofxBlend2DThreadedRenderer();

        // Sets the size
        void allocate(int _width, int _height);

        // Start submitting draw commands to context
        bool begin();
        bool end();
        void update();

        BLContext& getBlContext();
        std::string getContextErrors();

        ofTexture& getTexture();

#ifdef ofxBlend2D_ENABLE_OFXFPS
        float getFps();
        const float& getFpsHist() const;
        float getSyncTime();
        unsigned int getRenderedFrames() const;
#endif

        glm::vec2 getSize() const {
            return {width, height};
        }

        // Threads
        void stopBlThread();
        void startBlThread();

#ifdef ofxBlend2D_ENABLE_IMGUI
        void drawImGuiSettings();
#endif

        // todo: fps
    protected:
        //bool syncTexture();
        int width = 0;
        int height = 0;
        // Internal state
        bool isDirty = false; // Note: also protects some threaded variables
        bool isSubmittingDrawCmds = false;
        unsigned int renderedFrames = 0;
        bool bRenderHD = true;

        // Blend2D objects
        // Protected as channels
        BLContext ctx; // Canvas context
        BLImage img; // Canvas
        BLImageCodec codec;

        // OF Objects
        ofTexture tex; //
        BLContextCreateInfo createInfo = {};

#ifdef ofxBlend2D_ENABLE_OFXFPS
        ofxFps fpsCounter;
        float fpsCounterHist[ofxBlend2D_FPS_HISTORY_SIZE];
#endif

        // Threads
        void threadedFunction() override;
        ofThreadChannel<ofBuffer> pixelDataFromThread;
        ofThreadChannel<bool> flushFrameSignal;
};

