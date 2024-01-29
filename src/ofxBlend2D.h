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

// Uncomment to enable very verbose threading debug messages
//#define ofxBlend2D_DEBUG

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

        float getFps();
        const float& getFpsHist() const;
        float getSyncTime();
        unsigned int getRenderedFrames() const;

        // Threads
        void stopBlThread();
        void startBlThread();

        // todo: fps
    protected:
        //bool syncTexture();
        int width = 0;
        int height = 0;
        // Internal state
        bool isDirty = false; // Note: also protects some threaded variables
        bool isSubmittingDrawCmds = false;
        unsigned int renderedFrames = 0;

        // Blend2D objects
        // Protected as channels
        BLContext ctx; // Canvas context
        BLImage img; // Canvas
        BLImageCodec codec;

        // OF Objects
        ofTexture tex; //
        BLContextCreateInfo createInfo = {};

        ofxFps fpsCounter;
        float fpsCounterHist[ofxBlend2D_FPS_HISTORY_SIZE];

        // Threads
        void threadedFunction() override;
        ofThreadChannel<ofBuffer> pixelDataFromThread;
        ofThreadChannel<bool> flushFrameSignal;
};

