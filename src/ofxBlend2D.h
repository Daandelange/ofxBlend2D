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

#ifdef ofxBlend2D_ENABLE_OFXFPS
#   include "ofxFps.h"
#endif


class ofxBlend2DThreadedRenderer : protected ofThread {

    public:
        ofxBlend2DThreadedRenderer();
        ~ofxBlend2DThreadedRenderer();

        // Sets the size
        void allocate(int _width, int _height, int glPixelType=0);

        // Start submitting draw commands to context
        bool begin();
        bool end(unsigned int frameNum, std::string frameFileToSave);
        void update(const bool waitForThread=false);

        BLContext& getBlContext();
        std::string getContextErrors();

        ofTexture& getTexture();
        GLint getTexturePixelFormat(){
            return glInternalFormatTexture;
        }

#ifdef ofxBlend2D_ENABLE_OFXFPS
        float getFps();
        const float& getFpsHist() const;
        float getSyncTime();
        unsigned int getRenderedFrames() const;
        bool pauseHistogram = false;
#endif

        glm::vec2 getSize() const {
            return {width, height};
        }
        bool isDirty() const {
            return bIsDirty;
        }

        // Threads
        void stopBlThread();
        void startBlThread();

#ifdef ofxBlend2D_ENABLE_IMGUI
        void drawImGuiSettings();
#endif

        // A struct send to the thread with additional parameters
        // /!\ Takes ownership of ctx
        struct ofxBlend2DThreadedRendererData {
            BLContext ctx;
            unsigned int frameNum;
            std::string fileToSave; // saves frame to location if not empty (threaded)
            bool isValid;
            //ofxBlend2DThreadedRendererData() = delete;
            ofxBlend2DThreadedRendererData() :
                ctx(),
                frameNum(0u),
                fileToSave(""),
                isValid(false)
            {

            }
            ofxBlend2DThreadedRendererData(BLContext&& _ctx, unsigned int _frameNum, std::string _fileToSave="") :
                ctx(std::move(_ctx)),
                frameNum(_frameNum),
                fileToSave(_fileToSave),
                isValid(true)
            {

            }
            ~ofxBlend2DThreadedRendererData(){
                // Releases ctx when done
                ctx.end();
                isValid = false;
            };
        };

        // todo: fps
    protected:
        // Internal state
        unsigned int width = 0;
        unsigned int height = 0;
        GLint glInternalFormatTexture = 0;
        BLFormat blInternalFormat = BLFormat::BL_FORMAT_NONE;


        bool bIsDirty = false; // Note: also protects some threaded variables
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
        float fpsCounterHist[ofxBlend2D_FPS_HISTORY_SIZE] = {0};
#endif

        // Threads
        void threadedFunction() override;
        ofThreadChannel<BLArray<uint8_t> > pixelDataFromThread;
        ofThreadChannel<ofxBlend2DThreadedRendererData> flushFrameSignal;

        // BMP loading
        bool loadBmpStreamIntoTexture(const uint8_t* data, std::size_t size);
#ifdef ofxBlend2D_BMP_PARSER_INTERNAL
        // Threadable fn !
        struct BmpHeaderInfo {
            unsigned int width, height;
            bool isFlipped;
            GLint glFormat;
            unsigned int dataOffset;
            unsigned short bitCount;
        };
        static bool threadableParseBmpStream(BmpHeaderInfo& info, const uint8_t* data, std::size_t size);
#endif
};

// ImGui Helpers
#ifdef ofxBlend2D_ENABLE_IMGUI
namespace ImGuiEx {
    bool Blend2DFlattenMode(BLFlattenMode& mode, const char* label);
    // BlCompOp selector, returns true on change
    bool Blend2DCompOp(BLCompOp& op, const char* label="Composition Blending Operator");
    bool Blend2DFlattenMode(BLFlattenMode& mode, const char* label="Flatten Mode");
    bool Blend2DFlattenMode(uint8_t& mode, const char* label="Flatten Mode");
    bool Blend2DFlattenTolerance(double& flattenTolerance, const char* label="Flatten Tolerance");
    bool Blend2DSimplifyTolerance(double& simplifyTolerance, const char* label="Simplify Tolerance");
    bool Blend2DOffsetParam(double& offset, const char* label="Offset Parameter");
    bool Blend2DRenderingQuality(BLRenderingQuality& value, const char* label="Rendering Quality");
    bool Blend2DPatternQuality(BLPatternQuality& value, const char* label="Pattern Quality");
    bool Blend2DGradientQuality(BLGradientQuality& value, const char* label="Gradient Quality");
    bool Blend2DGlobalAlpha(double& globalAlpha, const char* label="Global Alpha");
    bool Blend2DFillAlpha(double& fillAlpha, const char* label="Fill Alpha");
    bool Blend2DOffsetMode(BLOffsetMode& value, const char* label="Offset Mode");
    bool Blend2DOffsetMode(uint8_t& value, const char* label="Offset Mode");
    bool Blend2DApproximationOptions(BLApproximationOptions& approxOptions, const char* label="Approximation Options");
    void Blend2DContextInfo(BLContext& ctx);

}
#endif

