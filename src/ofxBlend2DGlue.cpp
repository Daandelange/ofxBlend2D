#include "ofxBlend2DGlue.h"

// OF Types
#include "ofPixels.h"


// OF Glue
// - - - -
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

// Utility for making error codes human-readable
// Todo: implement toString(BLResult with this for seamless logging compatibility)
std::string blResultToString(BLResult r){
    switch(r){
        case BL_SUCCESS:
            return "BL_SUCCESS";
            break;
        case BL_ERROR_OUT_OF_MEMORY:
           return "BL_ERROR_OUT_OF_MEMORY"; // Out of memory                 [ENOMEM].
           break;
        case BL_ERROR_INVALID_VALUE:
           return "BL_ERROR_INVALID_VALUE"; // Invalid value/argument        [EINVAL].
           break;
        case BL_ERROR_INVALID_STATE:
           return "BL_ERROR_INVALID_STATE"; // Invalid state                 [EFAULT].
           break;
        case BL_ERROR_INVALID_HANDLE:
           return "BL_ERROR_INVALID_HANDLE"; // Invalid handle or file.       [EBADF].
           break;
        case BL_ERROR_INVALID_CONVERSION:
           return "BL_ERROR_INVALID_CONVERSION"; // Invalid conversion.
           break;
        case BL_ERROR_OVERFLOW:
           return "BL_ERROR_OVERFLOW"; // Overflow or value too large   [EOVERFLOW].
           break;
        case BL_ERROR_NOT_INITIALIZED:
           return "BL_ERROR_NOT_INITIALIZED"; // Object not initialized.
           break;
        case BL_ERROR_NOT_IMPLEMENTED:
           return "BL_ERROR_NOT_IMPLEMENTED"; // Not implemented               [ENOSYS].
           break;
        case BL_ERROR_NOT_PERMITTED:
           return "BL_ERROR_NOT_PERMITTED"; // Operation not permitted       [EPERM].
           break;
        case BL_ERROR_IO:
           return "BL_ERROR_IO"; // IO error                      [EIO].
           break;
        case BL_ERROR_BUSY:
           return "BL_ERROR_BUSY"; // Device or resource busy       [EBUSY].
           break;
        case BL_ERROR_INTERRUPTED:
           return "BL_ERROR_INTERRUPTED"; // Operation interrupted         [EINTR].
           break;
        case BL_ERROR_TRY_AGAIN:
           return "BL_ERROR_TRY_AGAIN"; // Try again                     [EAGAIN].
           break;
        case BL_ERROR_TIMED_OUT:
           return "BL_ERROR_TIMED_OUT"; // Timed out                     [ETIMEDOUT].
           break;
        case BL_ERROR_BROKEN_PIPE:
           return "BL_ERROR_BROKEN_PIPE"; // Broken pipe                   [EPIPE].
           break;
        case BL_ERROR_INVALID_SEEK:
           return "BL_ERROR_INVALID_SEEK"; // File is not seekable          [ESPIPE].
           break;
        case BL_ERROR_SYMLINK_LOOP:
           return "BL_ERROR_SYMLINK_LOOP"; // Too many levels of symlinks   [ELOOP].
           break;
        case BL_ERROR_FILE_TOO_LARGE:
           return "BL_ERROR_FILE_TOO_LARGE"; // File is too large             [EFBIG].
           break;
        case BL_ERROR_ALREADY_EXISTS:
           return "BL_ERROR_ALREADY_EXISTS"; // File/directory already exists [EEXIST].
           break;
        case BL_ERROR_ACCESS_DENIED:
           return "BL_ERROR_ACCESS_DENIED"; // Access denied                 [EACCES].
           break;
        case BL_ERROR_MEDIA_CHANGED:
           return "BL_ERROR_MEDIA_CHANGED"; // Media changed                 [Windows::ERROR_MEDIA_CHANGED].
           break;
        case BL_ERROR_READ_ONLY_FS:
           return "BL_ERROR_READ_ONLY_FS"; // The file/FS is read-only      [EROFS].
           break;
        case BL_ERROR_NO_DEVICE:
           return "BL_ERROR_NO_DEVICE"; // Device doesn't exist          [ENXIO].
           break;
        case BL_ERROR_NO_ENTRY:
           return "BL_ERROR_NO_ENTRY"; // Not found, no entry (fs)      [ENOENT].
           break;
        case BL_ERROR_NO_MEDIA:
           return "BL_ERROR_NO_MEDIA"; // No media in drive/device      [ENOMEDIUM].
           break;
        case BL_ERROR_NO_MORE_DATA:
           return "BL_ERROR_NO_MORE_DATA"; // No more data / end of file    [ENODATA].
           break;
        case BL_ERROR_NO_MORE_FILES:
           return "BL_ERROR_NO_MORE_FILES"; // No more files                 [ENMFILE].
           break;
        case BL_ERROR_NO_SPACE_LEFT:
           return "BL_ERROR_NO_SPACE_LEFT"; // No space left on device       [ENOSPC].
           break;
        case BL_ERROR_NOT_EMPTY:
           return "BL_ERROR_NOT_EMPTY"; // Directory is not empty        [ENOTEMPTY].
           break;
        case BL_ERROR_NOT_FILE:
           return "BL_ERROR_NOT_FILE"; // Not a file                    [EISDIR].
           break;
        case BL_ERROR_NOT_DIRECTORY:
           return "BL_ERROR_NOT_DIRECTORY"; // Not a directory               [ENOTDIR].
           break;
        case BL_ERROR_NOT_SAME_DEVICE:
           return "BL_ERROR_NOT_SAME_DEVICE"; // Not same device               [EXDEV].
           break;
        case BL_ERROR_NOT_BLOCK_DEVICE:
           return "BL_ERROR_NOT_BLOCK_DEVICE"; // Not a block device            [ENOTBLK].
           break;
        case BL_ERROR_INVALID_FILE_NAME:
           return "BL_ERROR_INVALID_FILE_NAME"; // File/path name is invalid     [n/a].
           break;
        case BL_ERROR_FILE_NAME_TOO_LONG:
           return "BL_ERROR_FILE_NAME_TOO_LONG"; // File/path name is too long    [ENAMETOOLONG].
           break;
        case BL_ERROR_TOO_MANY_OPEN_FILES:
           return "BL_ERROR_TOO_MANY_OPEN_FILES"; // Too many open files           [EMFILE].
           break;
        case BL_ERROR_TOO_MANY_OPEN_FILES_BY_OS:
           return "BL_ERROR_TOO_MANY_OPEN_FILES_BY_OS"; // Too many open files by OS     [ENFILE].
           break;
        case BL_ERROR_TOO_MANY_LINKS:
           return "BL_ERROR_TOO_MANY_LINKS"; // Too many symbolic links on FS [EMLINK].
           break;
        case BL_ERROR_TOO_MANY_THREADS:
           return "BL_ERROR_TOO_MANY_THREADS"; // Too many threads              [EAGAIN].
           break;
        case BL_ERROR_THREAD_POOL_EXHAUSTED:
           return "BL_ERROR_THREAD_POOL_EXHAUSTED"; // Thread pool is exhausted and couldn't acquire the requested thread count.
           break;
        case BL_ERROR_FILE_EMPTY:
           return "BL_ERROR_FILE_EMPTY"; // File is empty (not specific to any OS error).
           break;
        case BL_ERROR_OPEN_FAILED:
           return "BL_ERROR_OPEN_FAILED"; // File open failed              [Windows::ERROR_OPEN_FAILED].
           break;
        case BL_ERROR_NOT_ROOT_DEVICE:
           return "BL_ERROR_NOT_ROOT_DEVICE"; // Not a root device/directory   [Windows::ERROR_DIR_NOT_ROOT].
           break;
        case BL_ERROR_UNKNOWN_SYSTEM_ERROR:
           return "BL_ERROR_UNKNOWN_SYSTEM_ERROR"; // Unknown system error that failed to translate to Blend2D result code.
           break;
        case BL_ERROR_INVALID_ALIGNMENT:
           return "BL_ERROR_INVALID_ALIGNMENT"; // Invalid data alignment.
           break;
        case BL_ERROR_INVALID_SIGNATURE:
           return "BL_ERROR_INVALID_SIGNATURE"; // Invalid data signature or header.
           break;
        case BL_ERROR_INVALID_DATA:
           return "BL_ERROR_INVALID_DATA"; // Invalid or corrupted data.
           break;
        case BL_ERROR_INVALID_STRING:
           return "BL_ERROR_INVALID_STRING"; // Invalid string (invalid data of either UTF8, UTF16, or UTF32).
           break;
        case BL_ERROR_INVALID_KEY:
           return "BL_ERROR_INVALID_KEY"; // Invalid key or property.
           break;
        case BL_ERROR_DATA_TRUNCATED:
           return "BL_ERROR_DATA_TRUNCATED"; // Truncated data (more data required than memory/stream provides).
           break;
        case BL_ERROR_DATA_TOO_LARGE:
           return "BL_ERROR_DATA_TOO_LARGE"; // Input data too large to be processed.
           break;
        case BL_ERROR_DECOMPRESSION_FAILED:
           return "BL_ERROR_DECOMPRESSION_FAILED"; // Decompression failed due to invalid data (RLE, Huffman, etc).
           break;
        case BL_ERROR_INVALID_GEOMETRY:
           return "BL_ERROR_INVALID_GEOMETRY"; // Invalid geometry (invalid path data or shape).
           break;
        case BL_ERROR_NO_MATCHING_VERTEX:
           return "BL_ERROR_NO_MATCHING_VERTEX"; // Returned when there is no matching vertex in path data.
           break;
        case BL_ERROR_INVALID_CREATE_FLAGS:
           return "BL_ERROR_INVALID_CREATE_FLAGS"; // Invalid create flags (BLContext).
           break;
        case BL_ERROR_NO_MATCHING_COOKIE:
           return "BL_ERROR_NO_MATCHING_COOKIE"; // No matching cookie (BLContext).
           break;
        case BL_ERROR_NO_STATES_TO_RESTORE:
           return "BL_ERROR_NO_STATES_TO_RESTORE"; // No states to restore (BLContext).
           break;
        case BL_ERROR_TOO_MANY_SAVED_STATES:
           return "BL_ERROR_TOO_MANY_SAVED_STATES"; // Cannot save state as the number of saved states reached the limit (BLContext).
           break;
        case BL_ERROR_IMAGE_TOO_LARGE:
           return "BL_ERROR_IMAGE_TOO_LARGE"; // The size of the image is too large.
           break;
        case BL_ERROR_IMAGE_NO_MATCHING_CODEC:
           return "BL_ERROR_IMAGE_NO_MATCHING_CODEC"; // Image codec for a required format doesn't exist.
           break;
        case BL_ERROR_IMAGE_UNKNOWN_FILE_FORMAT:
           return "BL_ERROR_IMAGE_UNKNOWN_FILE_FORMAT"; // Unknown or invalid file format that cannot be read.
           break;
        case BL_ERROR_IMAGE_DECODER_NOT_PROVIDED:
           return "BL_ERROR_IMAGE_DECODER_NOT_PROVIDED"; // Image codec doesn't support reading the file format.
           break;
        case BL_ERROR_IMAGE_ENCODER_NOT_PROVIDED:
           return "BL_ERROR_IMAGE_ENCODER_NOT_PROVIDED"; // Image codec doesn't support writing the file format.
           break;
        case BL_ERROR_PNG_MULTIPLE_IHDR:
           return "BL_ERROR_PNG_MULTIPLE_IHDR"; // Multiple IHDR chunks are not allowed (PNG).
           break;
        case BL_ERROR_PNG_INVALID_IDAT:
           return "BL_ERROR_PNG_INVALID_IDAT"; // Invalid IDAT chunk (PNG).
           break;
        case BL_ERROR_PNG_INVALID_IEND:
           return "BL_ERROR_PNG_INVALID_IEND"; // Invalid IEND chunk (PNG).
           break;
        case BL_ERROR_PNG_INVALID_PLTE:
           return "BL_ERROR_PNG_INVALID_PLTE"; // Invalid PLTE chunk (PNG).
           break;
        case BL_ERROR_PNG_INVALID_TRNS:
           return "BL_ERROR_PNG_INVALID_TRNS"; // Invalid tRNS chunk (PNG).
           break;
        case BL_ERROR_PNG_INVALID_FILTER:
           return "BL_ERROR_PNG_INVALID_FILTER"; // Invalid filter type (PNG).
           break;
        case BL_ERROR_JPEG_UNSUPPORTED_FEATURE:
           return "BL_ERROR_JPEG_UNSUPPORTED_FEATURE"; // Unsupported feature (JPEG).
           break;
        case BL_ERROR_JPEG_INVALID_SOS:
           return "BL_ERROR_JPEG_INVALID_SOS"; // Invalid SOS marker or header (JPEG).
           break;
        case BL_ERROR_JPEG_INVALID_SOF:
           return "BL_ERROR_JPEG_INVALID_SOF"; // Invalid SOF marker (JPEG).
           break;
        case BL_ERROR_JPEG_MULTIPLE_SOF:
           return "BL_ERROR_JPEG_MULTIPLE_SOF"; // Multiple SOF markers (JPEG).
           break;
        case BL_ERROR_JPEG_UNSUPPORTED_SOF:
           return "BL_ERROR_JPEG_UNSUPPORTED_SOF"; // Unsupported SOF marker (JPEG).
           break;
        case BL_ERROR_FONT_NOT_INITIALIZED:
           return "BL_ERROR_FONT_NOT_INITIALIZED"; // Font doesn't have any data as it's not initialized.
           break;
        case BL_ERROR_FONT_NO_MATCH:
           return "BL_ERROR_FONT_NO_MATCH"; // Font or font face was not matched (BLFontManager).
           break;
        case BL_ERROR_FONT_NO_CHARACTER_MAPPING:
           return "BL_ERROR_FONT_NO_CHARACTER_MAPPING"; // Font has no character to glyph mapping data.
           break;
        case BL_ERROR_FONT_MISSING_IMPORTANT_TABLE:
           return "BL_ERROR_FONT_MISSING_IMPORTANT_TABLE"; // Font has missing an important table.
           break;
        case BL_ERROR_FONT_FEATURE_NOT_AVAILABLE:
           return "BL_ERROR_FONT_FEATURE_NOT_AVAILABLE"; // Font feature is not available.
           break;
        case BL_ERROR_FONT_CFF_INVALID_DATA:
           return "BL_ERROR_FONT_CFF_INVALID_DATA"; // Font has an invalid CFF data.
           break;
        case BL_ERROR_FONT_PROGRAM_TERMINATED:
            return "BL_ERROR_FONT_PROGRAM_TERMINATED"; // Font program terminated because the execution reached the limit.
           break;
        case BL_ERROR_GLYPH_SUBSTITUTION_TOO_LARGE:
            return "BL_ERROR_GLYPH_SUBSTITUTION_TOO_LARGE"; // Glyph substitution requires too much space and was terminated.
            break;
        case BL_ERROR_INVALID_GLYPH:
            return "BL_ERROR_INVALID_GLYPH"; //  Invalid glyph identifier.
            break;
        default:
            return "Unknown";
            break;
    }
}

// GL Utils
GLint blFormatToGlFormat(uint16_t blFormat){
    switch(blFormat){
        case BL_FORMAT_PRGB32:
            return GL_RGBA;
        break;
        case BL_FORMAT_XRGB32:
            return GL_RGB;
        break;
        case BL_FORMAT_A8:
            return GL_LUMINANCE;
        break;
        default:
            return GL_RGBA;
        break;
    }
}

// Missing ofGLUtils
ofPixelFormat ofxBlend2DGetOfPixelFormatFromGLFormat(const GLint glFormat){
    switch(glFormat){
        case GL_BGRA:
        //case GL_BGRA_EXT:
            return OF_PIXELS_BGRA;
        case GL_RGB:
            return OF_PIXELS_RGB;
#ifdef TARGET_OPENGLES
        case GL_RGB:
#else
        case GL_BGR:
#endif
            return OF_PIXELS_BGR;
        case GL_RGBA:
            return OF_PIXELS_RGBA;
        case GL_LUMINANCE:
            return OF_PIXELS_GRAY;
        case GL_RG:
        case GL_LUMINANCE_ALPHA:
            return OF_PIXELS_GRAY_ALPHA;
        default:
            return OF_PIXELS_GRAY;
    }
}
