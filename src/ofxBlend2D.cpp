#include "ofxBlend2D.h"
#include "ofAppRunner.h"
#include "ofUtils.h"
#include "ofLog.h"
#include <iostream>
#include <cassert>
#include "ofImage.h"
#include "ofPixels.h"

#ifdef ofxBlend2D_ENABLE_IMGUI
#   include "imgui.h"
#   include "ofxImGui.h"
#endif

ofxBlend2DThreadedRenderer::ofxBlend2DThreadedRenderer(){
    createInfo.threadCount = 4; // Number of threads
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

void ofxBlend2DThreadedRenderer::allocate(int _width, int _height, int glPixelType){
    // todo: wait for thread first ?

    // If default pixeltype, set it to the previous one.
    // If no previous, set to default.
    // This way you can change the pixels without remembering the initial format.
    if(glPixelType==0){
        glPixelType = glInternalFormatTexture==0?GL_RGB:glInternalFormatTexture;
    }

    width = _width;
    height = _height;
    tex.allocate(_width, _height, glPixelType);

    // Texture always allocates to the requested format
    glInternalFormatTexture = glPixelType;

    // Set Blend2D buffer equivalent
    switch(glPixelType){
        case GL_RGBA:
            blInternalFormat = BLFormat::BL_FORMAT_PRGB32;
            //glInternalFormatTexture = GL_BGRA;
            break;
        case GL_RGB:
            blInternalFormat = BLFormat::BL_FORMAT_XRGB32;
            //glInternalFormatTexture = GL_BGR;
            break;
        case GL_LUMINANCE:
            blInternalFormat = BLFormat::BL_FORMAT_A8;
            //glInternalFormatTexture = GL_LUMINANCE;
            break;
        default:
            // By default, set blend2d to full RGBA buffer
            blInternalFormat = BLFormat::BL_FORMAT_PRGB32;
            ofLogWarning("ofxBlend2DThreadedRenderer::allocate") << "Unsupported pixel type, using the default BMP32 with alpha.";
    }
}

bool ofxBlend2DThreadedRenderer::begin(){
    assert(!isSubmittingDrawCmds); // begin() / end() call order mismatch !

    // Skip when dirty = let update()/thread finish the update ?
    if(bIsDirty){
#ifdef ofxBlend2D_DEBUG
        std::cout << ofGetFrameNum() << "f__ "  << "Skipping blend2d frame ! (thread not done yet)" << std::endl;
#endif
        return false;
    }

    // Init output image canvas
    img = BLImage(width, height, blInternalFormat);

    // Create context for this image
    BLResult result = ctx.begin(img, createInfo);

    // Success ?
    if (result != BL_SUCCESS){
        ofLogError("ofxBlend2D::begin()") << "Error creating context !";
        return false;
    }

#ifdef ofxBlend2D_DEBUG
    std::cout << ofGetFrameNum() << "f__ " << "Begin() : Building new ctx ! :D" << " dirty=" << bIsDirty << std::endl;
#endif
    // Remember
    isSubmittingDrawCmds = true;

    // Init context
    ctx.setRenderingQuality(bRenderHD ? BLRenderingQuality::BL_RENDERING_QUALITY_ANTIALIAS : BLRenderingQuality::BL_RENDERING_QUALITY_ANTIALIAS); // No effect as on jan 2024, will auto-enable ? (both consts are equal)
    // Todo: make this optional ?
    ctx.clearAll();
    ctx.fillAll(BLRgba32(255,255,255,0));

    return true;
}

bool ofxBlend2DThreadedRenderer::end(unsigned int frameNum, std::string frameFileToSave){
    assert(isSubmittingDrawCmds); // begin() / end() call order mismatch !
    isSubmittingDrawCmds = false;
    bIsDirty = true;

#ifdef ofxBlend2D_DEBUG
    std::cout << ofGetFrameNum() << "f__ "  << "GLThread generated new frame data !" << " dirty=" << bIsDirty << std::endl;
#endif

    flushFrameSignal.send(ofxBlend2DThreadedRendererData{std::move(ctx), frameNum, frameFileToSave});

    return true;
}

// Returns true if frame was received
bool ofxBlend2DThreadedRenderer::update(const bool waitForThread, const bool noFrameSkipping){
    assert(!isSubmittingDrawCmds);

    if(bIsDirty){
        static BLImageData bufFromThread;
        // Wait long for once ?
        bool newFrame = pixelDataFromThread.tryReceive(bufFromThread, waitForThread?999999:0);
        // Empty queue until most recent image to grab (should never happen)
        if(!noFrameSkipping) while(pixelDataFromThread.tryReceive(bufFromThread, 0)){
            newFrame = true;
        }
        if(newFrame){
#ifdef ofxBlend2D_ENABLE_OFXFPS
            // Start timer
            fpsCounter.begin();
#endif

            // Blocks ! Wait for render queue to finish
            // Note: should be flushed already; just calling in case the user illegally submitted render data
            // Todo: can we access ctx here ???
            //ctx.flush(BLContextFlushFlags::BL_CONTEXT_FLUSH_SYNC);

            // Grab the result
            if(!loadImageDataIntoTexture(&bufFromThread)){
                ofLogWarning("ofxBlend2D") << "Could not load data from pixels !" << std::endl;
            }

            // Close & release context
            //ctx.end();

            bIsDirty = false;
            renderedFrames++;

#ifdef ofxBlend2D_ENABLE_OFXFPS
            // Update timer. Todo: update also when no new frames & gui not visible ?
            fpsCounter.end();
            if(!pauseHistogram){
                for(int i=0; i<ofxBlend2D_FPS_HISTORY_SIZE-1; ++i){
                    fpsCounterHist[i]=fpsCounterHist[i+1];
                }
                fpsCounterHist[ofxBlend2D_FPS_HISTORY_SIZE-1] = fpsCounter.getFps();
            }
#endif

#ifdef ofxBlend2D_DEBUG
            std::cout << ofGetFrameNum() << "f__ "  << "Update() -> Received a new frame !" << " dirty=" << bIsDirty << std::endl;
#endif
        }
#ifdef ofxBlend2D_DEBUG
        else std::cout << ofGetFrameNum() << "f__ "  << "Update() : Skipping, no new frame received yet." << " dirty=" << bIsDirty << std::endl;
#endif
        return newFrame;
    }
    return false;
}

// Returns true if frame is available (at time of call)
bool ofxBlend2DThreadedRenderer::hasNewFrame(){
    if(bIsDirty){
        return !pixelDataFromThread.empty();
    }
    return false;
}

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

#ifdef ofxBlend2D_ENABLE_OFXFPS
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
#endif

void ofxBlend2DThreadedRenderer::threadedFunction(){

    bool newFrame = false;
    ofxBlend2DThreadedRendererData frameData;
    BLArray<uint8_t> pixelDataInThread;

    while(flushFrameSignal.receive(frameData)){
#ifdef ofxBlend2D_DEBUG
        std::cout << "Thread : encoding new frame !" << std::endl;
#endif

        // Simulate renderer lag !
        //std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Lock mutex : from here, we can use the protected vars (expecting that the gl thread doesn't use them anymore)
        // Protected vars : ctx, , all linked BLPaths
        std::unique_lock<std::mutex> lock(mutex);


        // Flush the blend2d pipeline !
        // Blocks ! Waits for threaded render queue to finish
        // Todo: Call end() rather ?
        frameData.ctx.end();

        // Grab the result
        BLArray<uint8_t> resultData;
        BLImageData imgData;
        BLResult resultDataGet = img.getData(&imgData);
        if(resultDataGet != BL_SUCCESS){
            ofLogWarning("ofxBlend2DThreadedRenderer") << "Couldn't load texture! Error=" << resultDataGet << "(" << blResultToString(resultDataGet) << ") and ContextError=" << getContextErrors();
            return;
        }

        std::size_t resultSize = imgData.size.h * imgData.stride;

        // Gotta save the file ?
        if(frameData.fileToSave.length()>0){
            // Decode the data
            GLint glFormat = blFormatToGlFormat(imgData.format);
            auto numChannels = ofGetNumChannelsFromGLFormat(glFormat);

            // Build pixels object
            ofPixels pixels;
            pixels.setFromAlignedPixels((const unsigned char*)imgData.pixelData, imgData.size.w, imgData.size.h, ofxBlend2DGetOfPixelFormatFromGLFormat(glFormat), imgData.stride);

            // Save the data !
            if(!ofSaveImage(pixels, ofToDataPath(frameData.fileToSave), OF_IMAGE_QUALITY_BEST)){
                ofLogError("ofxBlend2DThreadedRenderer::threadedFunction()") << "Couldn't write frame to file. Frame=" << frameData.frameNum;
            }
        }

        // Forward data to thread !
#if __cplusplus>=201103
        pixelDataFromThread.send(std::move(imgData));
#else
        pixelDataFromThread.send(imgData);
#endif
    }
}

bool ofxBlend2DThreadedRenderer::loadImageDataIntoTexture(const BLImageData* data){
    if(data==nullptr) return false;

    // Parse pixel format
    GLint glFormat = blFormatToGlFormat(data->format);
    auto numChannels = ofGetNumChannelsFromGLFormat(glFormat);

    // Ensure alocated size is the same
    // Checkme : compare width with blend2d size or bmp header size ???
    if(
        (!tex.isAllocated()) || // not allocated ?
        (tex.getTextureData().glInternalFormat != glInternalFormatTexture) || // Different pixel format ?
        (tex.getTextureData().width != data->size.w) || // different width ?
        (tex.getTextureData().height != data->size.h) // different width ?
    ){
        if(
                (glFormat != glInternalFormatTexture) ||
                (data->size.w != width) || // different width ?
                (data->size.h != height) // different width ?
        ){
           ofLogWarning("ofxBlend2DThreadedRenderer::loadImageDataIntoTexture") << "The returned BMP header is not the same resolution as the configured size ! Resizing texture to match BMP data !";
        }
        tex.allocate(data->size.w, data->size.h, glInternalFormatTexture);
    }

    // Load the pixel data into the texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, data->stride/numChannels); // Allow stride within data
    tex.loadData((const void*)data->pixelData, data->size.w, data->size.h, glFormat, ofGetGLTypeFromInternal(glFormat)); // GL_UNSIGNED_BYTE
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0); // resets GL value
    return true;
}

#ifdef ofxBlend2D_ENABLE_IMGUI
void ofxBlend2DThreadedRenderer::drawImGuiSettings(){
    ImGui::PushID("Blend2DSettings");
    ImGui::SeparatorText("Blend2D");

#ifdef DEBUG
    ImGui::SameLine();
    ImGui::TextDisabled("  [!] ");
    if(ImGui::IsItemHovered()){
        if(ImGui::BeginTooltip()){
            ImGui::Text("Compiled in debug mode.");
            ImGui::Text("(bad for performance)");
            ImGui::EndTooltip();
        }
    }
#endif

#ifdef ofxBlend2D_BMP_PARSER_OF
    ImGui::SameLine();
    ImGui::TextDisabled("  [!]\ ");
    if(ImGui::IsItemHovered()){
        if(ImGui::BeginTooltip()){
            ImGui::Text("BMP parser is OpenFrameworks");
            ImGui::Text("(bad for performance)");
            ImGui::EndTooltip();
        }
    }
#endif

    bool reAllocate = false;

    static unsigned int pixelSteps[2] = {1, 10}; // Slow + Fast steps
    if(ImGui::InputScalar("Width", ImGuiDataType_U32, &width, (void*)&pixelSteps[0], (void*)&pixelSteps[1], "%u px", ImGuiInputTextFlags_EnterReturnsTrue)){
        reAllocate=true;
    }
    if(ImGui::InputScalar("Height", ImGuiDataType_U32, &height, (void*)&pixelSteps[0], (void*)&pixelSteps[1], "%u px", ImGuiInputTextFlags_EnterReturnsTrue)){
        reAllocate=true;
    }
    static const std::pair<GLint, const char*> options[] = {{GL_RGBA, "GL_RGBA"}, {GL_RGB, "GL_RGB"}, { GL_LUMINANCE, "GL_LUMINANCE" }, { 0, "Unknown" } };
    auto curOpt = std::find_if( std::begin(options), std::end(options),
        [this](const std::pair<GLint, const char*>& element){
            return element.first == this->glInternalFormatTexture;
        }
    );
    if(curOpt == std::end(options)) curOpt = &options[IM_ARRAYSIZE(options)-1];
    if (ImGui::BeginCombo("Pixel format", curOpt->second)){
        for (auto& pair : options){
            const bool is_selected = glInternalFormatTexture == pair.first;
            if(pair.first==0) ImGui::BeginDisabled();
            if(ImGui::Selectable(pair.second, is_selected)){
                allocate(width, height, pair.first);
            }
            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if(is_selected) ImGui::SetItemDefaultFocus();
            if(pair.first==0) ImGui::EndDisabled();
        }
        ImGui::EndCombo();
    }
    if(reAllocate){
        allocate(width, height);
    }

    static unsigned int numThreads[4] = { 0, 1, 0, 12 }; // cur, speed, min, max
    numThreads[0] = createInfo.threadCount;
    if(ImGui::DragScalar("Num threads", ImGuiDataType_U32, (void*)&numThreads[0], numThreads[1], &numThreads[2], &numThreads[3], "%u" )){
        createInfo.threadCount = numThreads[0];
    }
    ImGui::Checkbox("High quality rendering", &bRenderHD);

    if(getTexture().isAllocated()){
        ImGui::Text("Texture Resolution: %.0f x %.0f (%s)", getTexture().getWidth(), getTexture().getHeight(), curOpt->second);
        if(getTexture().getTextureData().glInternalFormat!=glInternalFormatTexture){
            ImGui::SameLine();
            ImGui::TextDisabled("[!]");
            if(ImGui::IsItemHovered()){
                if(ImGui::BeginTooltip()){
                    ImGui::Text("Warning! The texture pixel format differs from the allocated one !");
                    ImGui::EndTooltip();
                }
            }
        }
    }
    else {
        ImGui::Text("Texture Resolution: [Not allocated!]");
    }

#ifdef ofxBlend2D_ENABLE_OFXFPS
    static float averageFps = 0, minFps = 100, maxFps = 0;
    if(!pauseHistogram){
        averageFps = 0;
        minFps = 100;
        maxFps = 0;
        for(int i = 0; i<ofxBlend2D_FPS_HISTORY_SIZE; ++i){
            const float& f = (&getFpsHist())[i];
            averageFps += f;
            minFps = glm::min(minFps, f);
            maxFps = glm::max(maxFps, f);
        }
        averageFps /= ofxBlend2D_FPS_HISTORY_SIZE;
    }

    ImGui::Dummy({10,20});
    ImGui::SeparatorText("Performance");

    ImGui::PlotHistogram("##blend2d_fps_histogram", &getFpsHist(), ofxBlend2D_FPS_HISTORY_SIZE, 0, NULL, 0.f, 75.f, ImVec2(0,30));
    pauseHistogram = ImGui::IsKeyDown(ImGuiMod_Shift) && ImGui::IsItemHovered();

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {2,0});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {2, 0});
    ImGui::SameLine();
    ImGui::Spacing();
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::SetCursorPosY(ImGui::GetCursorPosY()-5);
    ImGui::SetWindowFontScale(1.5f);
    ImGui::Text("%5.1f", averageFps );
    ImGui::SetWindowFontScale(1.f);
    ImGui::Text(" %5.3f sec", getSyncTime() );
    ImGui::EndGroup();
    ImGui::PopStyleVar(2);

    ImGui::Text("(cur:%3.0f min:%5.1f max:%5.1f) %6u frames", getFps(), minFps, maxFps, getRenderedFrames() );
#endif // end ofxBlend2D_ENABLE_OFXFPS

    ImGui::PopID();
}
#endif // end ofxBlend2D_ENABLE_IMGUI

// ImGui Helpers
#ifdef ofxBlend2D_ENABLE_IMGUI
#include "imgui_internal.h"
namespace ImGuiEx {

    // Static labels
    // Note: they must be in chronological order
    static const std::array< std::pair<BLCompOp, const char*>, (1+BL_COMP_OP_MAX_VALUE) > compOpStrings {{
        { BL_COMP_OP_SRC_OVER,      "Source-over [default]" },
        { BL_COMP_OP_SRC_COPY,      "Source-copy" },
        { BL_COMP_OP_SRC_IN,        "Source-in" },
        { BL_COMP_OP_SRC_OUT,       "Source-out" },
        { BL_COMP_OP_SRC_ATOP,      "Source-atop" },
        { BL_COMP_OP_DST_OVER,      "Destination-over" },
        { BL_COMP_OP_DST_COPY,      "Destination-copy [nop]" },
        { BL_COMP_OP_DST_IN,        "Destination-in" },
        { BL_COMP_OP_DST_OUT,       "Destination-out" },
        { BL_COMP_OP_DST_ATOP,      "Destination-atop" },
        { BL_COMP_OP_XOR,           "Xor" },
        { BL_COMP_OP_CLEAR,         "Clear" },
        { BL_COMP_OP_PLUS,          "Plus" },
        { BL_COMP_OP_MINUS,         "Minus" },
        { BL_COMP_OP_MODULATE,      "Modulate" },
        { BL_COMP_OP_MULTIPLY,      "Multiply" },
        { BL_COMP_OP_SCREEN,        "Screen" },
        { BL_COMP_OP_OVERLAY,       "Overlay" },
        { BL_COMP_OP_DARKEN,        "Darken" },
        { BL_COMP_OP_LIGHTEN,       "Lighten" },
        { BL_COMP_OP_COLOR_DODGE,   "Color dodge" },
        { BL_COMP_OP_COLOR_BURN,    "Color burn" },
        { BL_COMP_OP_LINEAR_BURN,   "Linear burn" },
        { BL_COMP_OP_LINEAR_LIGHT,  "Linear light" },
        { BL_COMP_OP_PIN_LIGHT,     "Pin light" },
        { BL_COMP_OP_HARD_LIGHT,    "Hard-light" },
        { BL_COMP_OP_SOFT_LIGHT,    "Soft-light" },
        { BL_COMP_OP_DIFFERENCE,    "Difference" },
        { BL_COMP_OP_EXCLUSION,     "Exclusion" }
    }};

    static const char* flattenModes[] = {
        "Use default mode (decided by Blend2D)", // BL_FLATTEN_MODE_DEFAULT
        "Recursive subdivision flattening", // BL_FLATTEN_MODE_RECURSIVE
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(flattenModes)==BL_FLATTEN_MODE_MAX_VALUE+1); // Blend2d has added a new enum value !

    static const char* fillModes[] = {
        "Non Zero", // BL_FILL_RULE_NON_ZERO
        "Even Odd",  // BL_FILL_RULE_EVEN_ODD
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(fillModes)==BL_FILL_RULE_MAX_VALUE+1); // Blend2d has added a new enum value !

    static const char* renderQualities[] = {
        "Antialias",// BL_RENDERING_QUALITY_ANTIALIAS
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(renderQualities)==BL_RENDERING_QUALITY_MAX_VALUE+1); // Blend2d has added a new enum value !

    static const char* patternQualities[] = {
        "Nearest neighbor interpolation", // BL_PATTERN_QUALITY_NEAREST
        "Bilinear interpolation" // BL_PATTERN_QUALITY_BILINEAR
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(patternQualities)==BL_PATTERN_QUALITY_MAX_VALUE+1); // Blend2d has added a new enum value !

    static const char* offsetModes[] = {
        "Use default mode (decided by Blend2D)", // BL_OFFSET_MODE_DEFAULT
        "Iterative offset construction" // BL_OFFSET_MODE_ITERATIVE
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(offsetModes)==BL_OFFSET_MODE_MAX_VALUE+1); // Blend2d has added a new enum value !

    static const char* gradientQualities[] = {
        "Nearest neighbor",// BL_GRADIENT_QUALITY_NEAREST
        "Use smoothing (unavailable)",// BL_GRADIENT_QUALITY_SMOOTH
        "Dither (implementation-specific algo)" //BL_GRADIENT_QUALITY_DITHER,
    };
    IM_STATIC_ASSERT(IM_ARRAYSIZE(gradientQualities)==BL_GRADIENT_QUALITY_MAX_VALUE+1); // Blend2d has added a new enum value !

    // Static bounds
    static const double alphaMin = 0.0, alphaMax = 1.0;
    static const double toleranceMin = 0.0, toleranceMax = 1.0; // todo: check speed+min+max here

    // Like ImGui demo
    void Blend2DHelpMarker(const char* text){
        ImGui::SameLine();
        ImGui::TextDisabled("[?]");
        if(ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary)){
            if(ImGui::BeginItemTooltip()){
                ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
                ImGui::Text("%s", text);
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }
        }
    }

    // Multi-purpose enum selector
    template<typename ENUM>
    bool Blend2DCombo(const char* label, ENUM& value, const char* const* items, int count, int maxValue, const char* help = nullptr){
        bool ret = false;
        int intValue = value;
        if(ImGui::Combo(label, &intValue, items, count)){
            if(intValue<0) intValue = 0;
            else if (intValue>maxValue) intValue = maxValue;
            value = static_cast<ENUM>(intValue);
            ret = true;
        }
        if(help != nullptr)
            Blend2DHelpMarker(help);
        return ret;
    }

    // BlCompOp selector, returns true on change
    bool Blend2DCompOp(BLCompOp& op, const char* label){
        bool ret = false;
        if(ImGui::BeginCombo(label, compOpStrings[op].second, ImGuiComboFlags_None)){
            for(auto co : compOpStrings){
                if(ImGui::Selectable(co.second, op==co.first)){
                    op = co.first;
                    ret = true;
                }
                if(op==co.first){
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        else {
            // If active, allow some extra input
            if(ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary)){//ImGui::IsItemActive()){
                // Increment with mouse scroll
                const float mouseWheel = ImGui::GetIO().MouseWheel;
                if(mouseWheel != 0){//glm::abs(mouseWheel) > 0){
                    const int dir = (mouseWheel<0?1:-1);
                    // Loop to end ?
                    if(op<=0 && dir<0) op=compOpStrings.rbegin()->first;//static_cast<BLCompOp>(BL_COMP_OP_MAX_VALUE);
                    // Loop to begin ?
                    else if(dir>0 && op>=BL_COMP_OP_MAX_VALUE) op=compOpStrings.begin()->first;//static_cast<BLCompOp>(0u);
                    // Apply increment
                    else op = static_cast<BLCompOp>(op+dir);

                    // Claim mouse wheel
                    ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
                }
            }
        }
        return ret;
    }

    bool Blend2DFlattenMode(BLFlattenMode& mode, const char* label){
        return Blend2DCombo<BLFlattenMode>(label, mode, flattenModes, IM_ARRAYSIZE(flattenModes), BL_FLATTEN_MODE_MAX_VALUE, "Specifies how curves are offsetted (used by stroking).");
    }

    bool Blend2DFlattenMode(uint8_t& mode, const char* label){
        BLFlattenMode flattenMode = BLFlattenMode(mode); // Weirdly a diff type
        bool ret = false;
        if(Blend2DFlattenMode(flattenMode, label)){
            mode = flattenMode;
            ret = true;
        }
        return ret;
    }

    bool Blend2DFlattenTolerance(double& flattenTolerance, const char* label){
        // todo: check speed here...
        bool ret = ImGui::DragScalar(label, ImGuiDataType_Double, &flattenTolerance, 0.01f, &toleranceMin, &toleranceMax, "%0.2f");
        Blend2DHelpMarker("Tolerance used for curve flattening.");
        return ret;
    }

    bool Blend2DSimplifyTolerance(double& simplifyTolerance, const char* label){
        bool ret = ImGui::DragScalar(label, ImGuiDataType_Double, &simplifyTolerance, 0.01f, &toleranceMin, &toleranceMax, "%0.2f");
        Blend2DHelpMarker("Tolerance used to approximate cubic curves with quadratic curves.");
        return ret;
    }

    bool Blend2DOffsetParam(double& offset, const char* label){
        // Todo: use other bounds ?
        bool ret = ImGui::DragScalar(label, ImGuiDataType_Double, &offset, 0.5f, &toleranceMin, &toleranceMax, "%.1f");
        Blend2DHelpMarker("Curve offsetting parameter, exact meaning depends on `offsetMode`.");
        return ret;
    }

    bool Blend2DRenderingQuality(BLRenderingQuality& value, const char* label){
        return Blend2DCombo<BLRenderingQuality>(label, value, renderQualities, IM_ARRAYSIZE(renderQualities), BL_RENDERING_QUALITY_MAX_VALUE, "The quality hint used for rendering.");
    }

    bool Blend2DPatternQuality(BLPatternQuality& value, const char* label){
        return Blend2DCombo<BLPatternQuality>(label, value, patternQualities, IM_ARRAYSIZE(patternQualities), BL_PATTERN_QUALITY_MAX_VALUE, "The quality hint used for rendering patterns.");
    }

    bool Blend2DGradientQuality(BLGradientQuality& value, const char* label){
        return Blend2DCombo<BLGradientQuality>(label, value, gradientQualities, IM_ARRAYSIZE(gradientQualities), BL_GRADIENT_QUALITY_MAX_VALUE, "The quality hint used for rendering gradients.");
    }

    bool Blend2DGlobalAlpha(double& globalAlpha, const char* label){
        bool ret = false;
        if(ImGui::DragScalar(label, ImGuiDataType_Double, &globalAlpha, 0.01f, &alphaMin, &alphaMax, "%0.2f")){
            ret = true;
        }
        Blend2DHelpMarker("Global rendering alpha value.");
        return ret;
    }

    bool Blend2DFillAlpha(double& fillAlpha, const char* label){
        bool ret = false;
        if(ImGui::DragScalar(label, ImGuiDataType_Double, &fillAlpha, 0.01f, &alphaMin, &alphaMax, "%0.2f")){
            ret = true;
        }
        Blend2DHelpMarker("Global fill alpha value.");
        return ret;
    }

    bool Blend2DOffsetMode(BLOffsetMode& value, const char* label){
        return Blend2DCombo<BLOffsetMode>(label, value, offsetModes, IM_ARRAYSIZE(offsetModes), BL_OFFSET_MODE_MAX_VALUE, " Specifies how curves are offsetted (used by stroking).");
    }

    bool Blend2DOffsetMode(uint8_t& value, const char* label){
        BLOffsetMode mode = BLOffsetMode(value); // Weirdly a diff type
        if(Blend2DOffsetMode(mode, label)){
            value = mode;
            return true;
        }
        return false;
    }

    bool Blend2DApproximationOptions(BLApproximationOptions& approxOptions, const char* label){
        bool anyChanged = false;

        if(label!=nullptr)
            ImGui::SeparatorText(label);

        // Curve flatten mode
        if(ImGuiEx::Blend2DFlattenMode(approxOptions.flattenMode))
            anyChanged |= true;

        // Flatten tolerance
        if(ImGuiEx::Blend2DFlattenTolerance(approxOptions.flattenTolerance))
            anyChanged |= true;

        // Offset Mode
        if(ImGuiEx::Blend2DOffsetMode(approxOptions.offsetMode))
            anyChanged |= true;

        // Offset param
        if(ImGuiEx::Blend2DOffsetParam(approxOptions.offsetParameter))
            anyChanged |= true;

        // Simplify Tolerance
        if(ImGuiEx::Blend2DSimplifyTolerance(approxOptions.simplifyTolerance))
            anyChanged |= true;

        return anyChanged;
    }

    void Blend2DContextInfo(BLContext& ctx){

        ImGui::SeparatorText("Rendering");
        BLRenderingQuality rq = ctx.renderingQuality();
        if(ImGuiEx::Blend2DRenderingQuality(rq)){
            ctx.setRenderingQuality(rq);
        }

        BLPatternQuality pq = ctx.patternQuality();
        if(ImGuiEx::Blend2DPatternQuality(pq)){
            ctx.setPatternQuality(pq);
        }

        BLGradientQuality gq = ctx.gradientQuality();
        if(ImGuiEx::Blend2DGradientQuality(gq)){
            ctx.setGradientQuality(gq);
        }

        ImGui::SeparatorText("Geometry");

        int curFillRule = ctx.fillRule();
        if(ImGui::Combo("Fill Rule", &curFillRule, fillModes, IM_ARRAYSIZE(fillModes))){
            ctx.setFillRule((BLFillRule)curFillRule);
        }

        // composition operator
        BLCompOp blendingMode = ctx.compOp();
        if(ImGuiEx::Blend2DCompOp(blendingMode, "Blending Mode")){
            ctx.setCompOp(blendingMode);
        }

        // Curve flatten mode
        BLFlattenMode flattenMode = ctx.flattenMode();
        if(ImGuiEx::Blend2DFlattenMode(flattenMode)){
            ctx.setFlattenMode(flattenMode);
        }

        // Flatten tolerance
        double flattenTolerance = ctx.flattenTolerance();
        if(ImGuiEx::Blend2DFlattenTolerance(flattenTolerance)){
            ctx.setFlattenTolerance(flattenTolerance);
        }

        ImGui::SeparatorText("Styles");

        // global alpha value
        double globalAlpha = ctx.globalAlpha();
        if(ImGuiEx::Blend2DGlobalAlpha(globalAlpha)){
            ctx.setGlobalAlpha(globalAlpha);
        }

        // Fill alpha value
        double fillAlpha = ctx.fillAlpha();
        if(ImGuiEx::Blend2DFillAlpha(fillAlpha)){
            ctx.setFillAlpha(fillAlpha);
        }

    }
}
#endif

