#include "icd_codecs.h"
#include <unordered_map>
#include <string>
#include <cctype>
#include <cstring>

NS_ICD_START

// Given a data type name, returns a data type
ICDDataType getDT(const char* name)
{
    if (name == nullptr)
        return ICDT_Byte;

    std::string s(name);
    for (auto& c : s)
        c = std::tolower(c);

    if (s == "uint16")
        return ICDT_UInt16;
    if (s == "int16" || s == "short")
        return ICDT_Int16;
    if (s == "uint32")
        return ICDT_UInt32;
    if (s == "int" || s == "int32" || s == "long")
        return ICDT_Int32;
    if (s == "float" || s == "float32")
        return ICDT_Float32;
    if (s == "double" || s == "float64")
        return ICDT_Float64;
    else
        return ICDT_Byte;
}

size_t getTypeSize(ICDDataType dt, size_t n) {
    static const std::unordered_map<ICDDataType, int> sizes = {
        {ICDT_Unknown, ~0},
        {ICDT_Byte, 1},
        {ICDT_UInt16, 2},
        {ICDT_Int16, 2},
        {ICDT_UInt32, 4},
        {ICDT_Int32, 4},
        {ICDT_Float32, 4},
        {ICDT_Double, 8}
    };
    return n * ((sizes.find(dt) == sizes.end()) ? ~0 : sizes.at(dt));
}

IMG_T getFMT(const char *name) {
    std::string s(name);
    if (s == "image/jpeg")
        return IMG_JPEG;
    if (s == "image/png")
        return IMG_PNG;
    if (s == "raster/lerc")
        return IMG_LERC;
    return IMG_INVALID;
}

const char* stride_decode(codec_params& params, storage_manager& src, void* buffer)
{
    const char* error_message = nullptr;
    uint32_t sig = 0;
    memcpy(&sig, src.buffer, sizeof(sig));
    params.raster.format = IMG_INVALID;
    switch (sig)
    {
    case JPEG_SIG:
        params.raster.format = IMG_JPEG;
        error_message = jpeg_stride_decode(params, src, buffer);
        break;
    case PNG_SIG:
        params.raster.format = IMG_PNG;
        error_message = png_stride_decode(params, src, buffer);
        break;
    case LERC_SIG:
        params.raster.format = IMG_LERC;
        error_message = lerc_stride_decode(params, src, buffer);
        break;
    default:
        error_message = "Decode requested for unknown format";
    }
    return error_message;
}

NS_END