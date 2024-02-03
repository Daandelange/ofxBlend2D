#pragma once

#include "blend2d.h"
#include "ofxBlend2DGlue.h"
#include "ofTexture.h"
#include "ofFileUtils.h" // ofBuffer
#include "ofColor.h"
#include "ofPath.h"

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

// Set default BMP decrypter (default & recommended : ofxBlend2D_BMP_PARSER_INTERNAL)
// Note : OF's freetype based BMP loader doesn't work with the headers sent by Blend2D in Linux+Windows.
// On OSX it works fine but is quite slow, so ofxBlend2D_BMP_PARSER_INTERNAL is also recommended.
//#define ofxBlend2D_BMP_PARSER_OF
#if !defined(ofxBlend2D_BMP_PARSER_OF) && !defined(ofxBlend2D_BMP_PARSER_INTERNAL)
#define ofxBlend2D_BMP_PARSER_INTERNAL
#endif

#ifdef ofxBlend2D_ENABLE_IMGUI
#   include "imgui.h"
#endif

#ifdef ofxBlend2D_ENABLE_OFXFPS
#   include "ofxFps.h"
#endif


class ofxBlend2DThreadedRenderer : protected ofThread {

    public:
        ofxBlend2DThreadedRenderer();
        ~ofxBlend2DThreadedRenderer();

        // Sets the size
        void allocate(int _width, int _height, int glPixelType=GL_RGBA);

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
        // Internal state
        int width = 0;
        int height = 0;
        unsigned int numChannels = 0;
        int glInternalFormatTexture = 0;
        BLFormat blInternalFormat = BLFormat::BL_FORMAT_NONE;


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
        ofThreadChannel<BLArray<uint8_t> > pixelDataFromThread;
        ofThreadChannel<bool> flushFrameSignal;

        // BMP loading
        bool loadBmpStreamIntoTexture(const uint8_t* data, std::size_t size);
};

