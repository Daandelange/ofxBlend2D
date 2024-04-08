#include "ofxBlend2D.h"
#include "ofAppRunner.h"
#include "ofUtils.h"
#include "ofLog.h"
#include <iostream>
#include "ofImage.h"

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
    if(isDirty){
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
    std::cout << ofGetFrameNum() << "f__ " << "Begin() : Building new ctx ! :D" << " dirty=" << isDirty << std::endl;
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

bool ofxBlend2DThreadedRenderer::end(){
    assert(isSubmittingDrawCmds); // begin() / end() call order mismatch !
    isSubmittingDrawCmds = false;
    isDirty = true;

#ifdef ofxBlend2D_DEBUG
    std::cout << ofGetFrameNum() << "f__ "  << "GLThread generated new frame data !" << " dirty=" << isDirty << std::endl;
#endif
    flushFrameSignal.send(true);

    return true;
}

void ofxBlend2DThreadedRenderer::update(){
    assert(!isSubmittingDrawCmds);

    if(isDirty){

        bool newFrame = false;
        static BLArray<uint8_t> bufFromThread;
        while(pixelDataFromThread.tryReceive(bufFromThread, 1)){ // Empty queue until most recent image to grab
            newFrame = true;
        }
        if(newFrame){
#ifdef ofxBlend2D_ENABLE_OFXFPS
            // Start timer
            fpsCounter.begin();
#endif

            // Blocks ! Wait for render queue to finish
            // Note: should be flushed already; just calling in case...
            ctx.flush(BLContextFlushFlags::BL_CONTEXT_FLUSH_SYNC);

            // Grab the result
            if(!loadBmpStreamIntoTexture((bufFromThread.data()), bufFromThread.size())){
                ofLogWarning("ofxBlend2D") << "Could not load data from pixels !" << std::endl;
            }

            // Close & release context
            ctx.end();

            isDirty = false;
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
            std::cout << ofGetFrameNum() << "f__ "  << "Update() -> Received a new frame !" << " dirty=" << isDirty << std::endl;
#endif
        }
#ifdef ofxBlend2D_DEBUG
        else std::cout << ofGetFrameNum() << "f__ "  << "Update() : Skipping, no new frame received yet." << " dirty=" << isDirty << std::endl;
#endif
    }
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
    BLArray<uint8_t> pixelDataInThread;

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
#if __cplusplus>=201103
			pixelDataFromThread.send(std::move(resultData));
#else
			pixelDataFromThread.send(resultData);
#endif
        }
	}
}

#ifdef ofxBlend2D_BMP_PARSER_INTERNAL
#pragma pack(push, 1)
struct BMPFileHeader {
    //char signature[2]; // Should be "BM"
    uint16_t signature{ 0x4D42 };          // File type always "BM" which is 0x4D42 (stored as hex uint16_t in little endian)
    uint32_t fileSize{ 0 };                // Size of the file (in bytes)
    uint16_t reserved1{ 0 };               // Reserved, always 0
    uint16_t reserved2{ 0 };               // Reserved, always 0
    uint32_t dataOffset{ 0 };              // Start position of pixel data (bytes from the beginning of the file)
};

struct BMPInfoHeader {
    uint32_t headerSize{ 0 };               // Size of this header (in bytes)
    int32_t width{ 0 };                     // width of bitmap in pixels
    int32_t height{ 0 };                    // width of bitmap in pixels
                                            //       (if positive, bottom-up, with origin in lower left corner)
                                            //       (if negative, top-down, with origin in upper left corner)
    uint16_t planes{ 1 };                   // # of planes (always 1)
    uint16_t bitCount{ 0 };                 // # of bits per pixel (8 B - 16 BA - 24 RGB - 32 RGBA)
    uint32_t compression{ 0 };              // 0 (compressed) or 3 (uncompressed)
    uint32_t imageSize{ 0 };                // 0 - for uncompressed images
    int32_t xPixelsPerMeter{ 0 };
    int32_t yPixelsPerMeter{ 0 };
    uint32_t colorsUsed{ 0 };               // # color indexes in the color table. Use 0 for the max number of colors allowed by bit_count (full color)
    uint32_t importantColors{ 0 };          // # of colors used for displaying the bitmap. If 0 all colors are important
};

struct BMPColorHeader {
    uint32_t redMask{ 0x00ff0000 };         // Bit mask for the red channel
    uint32_t greenMask{ 0x0000ff00 };       // Bit mask for the green channel
    uint32_t blueMask{ 0x000000ff };        // Bit mask for the blue channel
    uint32_t alphaMask{ 0xff000000 };       // Bit mask for the alpha channel
    uint32_t colorSpace{ 0x73524742 };      // Default 0x73524742 (=sRGB)
    //uint32_t colorSpaceData[16]{ 0 };       // Colorspace data (unused for sRGB color space), to fill the structure up to 64 bytes
};
#pragma pack(pop)

GLenum getOpenGLFormat(const BMPInfoHeader* infoHeader) {
    // Get GL pixel type from bitCount
    GLenum glFormat = 0;
    switch(infoHeader->bitCount) {
        case 1:  // Monochrome
            glFormat = GL_DEPTH_STENCIL;
            break;
        case 8: // Grayscale or indexed color
            glFormat = GL_LUMINANCE_ALPHA;
            break;
        case 24: // RGB
            glFormat = GL_RGB;
            break;
        case 32: // RGBA
            glFormat = GL_RGBA;
            break;

        case 4: // Indexed color
        case 16: // RGB565
        default:
            ofLogWarning("ofxBlend2D::getOpenGLFormat") << "Unsupported pixel type / bitCount = " << infoHeader->bitCount;
            return 0;
    }
    return 0;
}

GLenum getOpenGLFormat(const BMPColorHeader& colorHeader) {
    // Getting mask R-g-b-a : 00ff0000-0000ff00-000000ff-ff000000 (indicates position of data)
    if (colorHeader.redMask == 0x0000FF && colorHeader.greenMask == 0x00FF00 &&
        colorHeader.blueMask == 0xFF0000 && colorHeader.alphaMask == 0x000000) {
        return GL_RGBA;  // Standard RGBA ordering
    } else if (colorHeader.redMask == 0x00FF0000 && colorHeader.greenMask == 0x0000FF00 &&
               colorHeader.blueMask == 0x000000FF && colorHeader.alphaMask == 0xFF000000) {
        return GL_BGRA;  // Standard BGRA ordering
    } else if (colorHeader.redMask == 0x00FF0000 && colorHeader.greenMask == 0x0000FF00 &&
               colorHeader.blueMask == 0x000000FF && colorHeader.alphaMask == 0x00000000) {
        return GL_RGB;   // Standard RGB ordering
    } else if (colorHeader.redMask == 0x000000FF && colorHeader.greenMask == 0x00000000 &&
               colorHeader.blueMask == 0x00000000 && colorHeader.alphaMask == 0x00000000) {
        return GL_LUMINANCE;  // Single channel (luminance/grayscale)
    } else {
        ofLogWarning("ofxBlend2D::getOpenGLFormat") << "Unsupported color mask : " << std::hex << colorHeader.redMask << "-" << colorHeader.greenMask << "-" << colorHeader.blueMask << "-" << colorHeader.alphaMask;
        return GL_BGRA;
    }
}
#endif

// Parsing BMP buffer into the texture
bool ofxBlend2DThreadedRenderer::loadBmpStreamIntoTexture(const uint8_t* data, std::size_t size){
#ifdef ofxBlend2D_BMP_PARSER_INTERNAL
    // Load with INTERNAL loader

    // Ensure data is large enough to contain BMP file and image headers
    if(size < sizeof(BMPFileHeader) + sizeof(BMPInfoHeader)) {
        ofLogWarning("ofxBlend2D::loadBmpStreamIntoTexture") << "The header is missing!";
        return false;
    }

    // Parse BMP headers
    const BMPFileHeader* fileHeader = reinterpret_cast<const BMPFileHeader*>(data);

    // Check BMP signature
    if(fileHeader->signature != 0x4D42) {
        ofLogWarning("ofxBlend2D::loadBmpStreamIntoTexture") << "Invalid BMP buffer !";
        return false;
    }

    // Parse 2nd header
    const BMPInfoHeader* infoHeader = reinterpret_cast<const BMPInfoHeader*>(data + sizeof(BMPFileHeader));

    // Check compression
    if(infoHeader->compression!=0){
        //const BMPColorHeader* colorHeader = reinterpret_cast<const BMPColorHeader*>(data + dataOffset);
        ofLogWarning("ofxBlend2D::loadBmpStreamIntoTexture") << "The buffer is compressed BMP, I don't talk compression !";
        return false;
    }

    // Guess pixel format from info header
    GLenum glFormat = getOpenGLFormat(infoHeader);

    // Grab Color header if present (only used in RGBA formats)
    if(infoHeader->bitCount == 32 && infoHeader->headerSize > sizeof(BMPInfoHeader)){
        if(
            // Does the buffer go far enough ?
            (size >= (sizeof(BMPFileHeader) + sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))) &&
            // Header size is extended after color header !
            (infoHeader->headerSize >= ((sizeof(BMPInfoHeader) + sizeof(BMPColorHeader))))
        ) {
            const BMPColorHeader* colorHeader = reinterpret_cast<const BMPColorHeader*>(data + sizeof(BMPFileHeader) + sizeof(BMPInfoHeader));
            // Is it sRGB ? Can't verify, blend2D sets it to 0x00000001 !
            //if(colorHeader->colorSpace != 0x73524742){
            //    ofLogWarning("ofxBlend2D::loadBmpStreamIntoTexture") << "Unknown color space ! = "<< std::hex << colorHeader->colorSpace;
            //    // return here ? is this an error ?
            //}
            //else {
            GLenum nf = getOpenGLFormat(*colorHeader);
            if(nf!=0) glFormat = nf;
            //}
        }
        else {
            // Maybe this is no error ?
            ofLogWarning("ofxBlend2D::loadBmpStreamIntoTexture") << "Corrupted color header ! size=" << size << " -  encoded="<<infoHeader->headerSize << " - Theory=" << sizeof(BMPInfoHeader) << "/" << (sizeof(BMPInfoHeader) + sizeof(BMPColorHeader));
        }
    }

    // Extract information from BMP Info Header
    unsigned int dataOffset = fileHeader->dataOffset;
    int width = infoHeader->width;
    bool isFlipped = infoHeader->height > 0;
    int height = glm::abs(infoHeader->height);
    unsigned short bitCount = infoHeader->bitCount;

    // Calculate image size and data length
    unsigned int imageSize = (size - dataOffset);
    unsigned int dataLength = imageSize; // Adjust as needed based on image format

//    std::cout << "Image Dimensions: " << width << " x " << height << std::endl;
//    std::cout << "Pixel Format: " << bitCount << " bits per pixel /" << sizeof(uint8_t) << "=" << infoHeader->bitCount/sizeof(uint8_t) << std::endl;
//    std::cout << "Data Offset: " << dataOffset << std::endl;
//    std::cout << "Data Length: " << dataLength << std::endl;

    // Set the data pointer to the beginning of the pixel data
    const uint8_t* pixelData = data + dataOffset;

    // Parse 3rd header
    const BMPColorHeader* colorHeader = reinterpret_cast<const BMPColorHeader*>(data + sizeof(BMPFileHeader) + sizeof(BMPInfoHeader));


    // Ensure alocated size is the same
    if(
        !tex.isAllocated() || // not allocated ?
        tex.getTextureData().glInternalFormat != glInternalFormatTexture || // Different pixel format ?
        tex.getTextureData().width != width || // different width ?
        tex.getTextureData().height != height // different width ?
    ){
        tex.allocate(width, height, glInternalFormatTexture);
    }

    // Load the pixel data into the texture
    tex.loadData((const void*)pixelData, width, height, glFormat, GL_UNSIGNED_BYTE);//GL_UNSIGNED_BYTE);
    tex.getTextureData().bFlipTexture = isFlipped; // Buffer sends image flipped...

    return true;
#elif defined(ofxBlend2D_BMP_PARSER_OF)
    // Load with Openframeworks loader (which uses FreeImage)
    ofBuffer buf(reinterpret_cast<const char*>(data), size);
    return ofLoadImage(tex, buf);
#else
    return false;
#endif
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
    static const std::pair<GLenum, const char*> options[] = {{GL_RGBA, "GL_RGBA"}, {GL_RGB, "GL_RGB"}, { GL_LUMINANCE, "GL_LUMINANCE" }, { 0, "Unknown" } };
    auto curOpt = std::find_if( std::begin(options), std::end(options),
        [this](const std::pair<GLenum, const char*>& element){
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
    pauseHistogram = ImGui::IsKeyDown(ImGuiKey_ModShift) && ImGui::IsItemHovered();

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
