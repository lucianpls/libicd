
#include <QB3.h>
#include "icd_codecs.h"

#if !defined(NEED_SWAP)
#error QB3 works only in little endian
#endif

NS_ICD_START

static const char ERR_QB3[] = "Corrupt or unsupported QB3";
static const char ERR_SMALL[] = "Input buffer too small";
static const char ERR_OUT_SMALL[] = "Output buffer too small";
static const char ERR_DIFFERENT[] = "Unexpected type of QB3";

bool has_qb3() { return true; };

// Convert from qb3_dtype to ICDDataType
static ICDDataType qb3type_to_icdtype(qb3_dtype dt)
{
    switch (dt) {
    case QB3_U8: return ICDT_Byte;
    case QB3_I8: return ICDT_Char; // Signed missmatch between ICD and QB3
    case QB3_U16: return ICDT_UInt16;
    case QB3_I16: return ICDT_Int16;
    case QB3_U32: return ICDT_UInt32;
    case QB3_I32: return ICDT_Int32;
    //case QB3_U64: return ICDT_UInt64;
    //case QB3_I64: return ICDT_Int64;
    default: return ICDT_Unknown;
    }
}
// Reverse, from ICDDataType to qb3_dtype
static qb3_dtype icdtype_to_qb3type(ICDDataType dt)
{
    switch (dt) {
    case ICDT_UInt16: return QB3_U16;
    case ICDT_Int16: return QB3_I16;
    case ICDT_UInt32: return QB3_U32;
    case ICDT_Int32: return QB3_I32;
        //    case ICDT_Byte: return QB3_U8;
        //    case ICDT_Char: return QB3_I8; // Signed missmatch between ICD and QB3
                //case ICDT_UInt64: return QB3_U64;
        //case ICDT_Int64: return QB3_I64;
    default: return QB3_U8;
    }
}

// Skims over a QB3 file, checks that raster expectations are met
const char* peek_qb3(const storage_manager& src, Raster& raster)
{
    // What is the smallest valid QB3 file?
    if (src.size < 100)
        return ERR_SMALL;
    size_t size[3]; // X, Y, C
    decsp p = 
        qb3_read_start(src.buffer, src.size, size);
    if (!p)
        return ERR_QB3;
    raster.size.x = size[0];
    raster.size.y = size[1];
    raster.size.c = size[2];

    // Call qb3_read_info to get the data type
    if (!qb3_read_info(p)) {
        qb3_destroy_decoder(p);
        return ERR_QB3;
    }
    // Get the data type
    raster.dt = qb3type_to_icdtype(qb3_get_type(p));

    qb3_destroy_decoder(p);
    return nullptr;
}

const char* stride_decode_qb3(codec_params& params, storage_manager& src, void* buffer)
{
    if (src.size < 100)
        return ERR_SMALL;
    // Start reading the QB3 file
    size_t size[3]; // X, Y, C
    decsp p = qb3_read_start(src.buffer, src.size, size);
    if (!p)
        return ERR_QB3;
    // Check that the size is as expected
    if (size[0] != params.raster.size.x || 
        size[1] != params.raster.size.y || 
        size[2] != params.raster.size.c)
    {
        qb3_destroy_decoder(p);
        return ERR_DIFFERENT;
    }
    // Read the full headers
    auto status = qb3_read_info(p);
    if (!status) {
        qb3_destroy_decoder(p);
        return ERR_QB3;
    }
    // Check that we have the expected type
    if (qb3_get_type(p) != icdtype_to_qb3type(params.raster.dt)) {
        qb3_destroy_decoder(p);
        return ERR_DIFFERENT;
    }
    // Check that the output buffer is large enough
    if (params.get_buffer_size() < qb3_decoded_size(p)) {
        qb3_destroy_decoder(p);
        return ERR_OUT_SMALL;
    }
    // Set stride if needed
    if (params.line_stride != 0)
        qb3_set_decoder_stride(p, params.line_stride);
    // OK, go ahead and decode
    auto read_size = qb3_read_data(p, buffer);
    if (read_size != qb3_decoded_size(p)) {
        qb3_destroy_decoder(p);
        return ERR_QB3;
    }
    qb3_destroy_decoder(p);
    return nullptr;
}

const char* encode_qb3(qb3_params& params, storage_manager& src, storage_manager& dst)
{
    // Only integer types
    if (params.raster.dt >= ICDT_Float)
        return "Only integer types supported for QB3";
    if (getTypeSize(params.raster.dt, params.raster.size.x * params.raster.size.y * params.raster.size.c) > src.size)
        return ERR_SMALL;
    // At least 4x4
    if (params.raster.size.x < 4 || params.raster.size.y < 4)
        return "QB3 requires at least 4x4 input image";
    // Build the encoder
    auto encoder = qb3_create_encoder(params.raster.size.x, params.raster.size.y, params.raster.size.c, 
        icdtype_to_qb3type(params.raster.dt));
    if (!encoder)
        return "Can't create QB3 encoder";
    
    // Check that the output buffer is large enough
    size_t max_size = qb3_max_encoded_size(encoder);
    if (dst.size < max_size) {
        qb3_destroy_encoder(encoder);
        return ERR_OUT_SMALL;
    }

    dst.size = qb3_encode(encoder, src.buffer, dst.buffer);
    // Check if the encoding failed
    if (qb3_get_encoder_state(encoder)) {
        qb3_destroy_encoder(encoder);
        return ERR_QB3;
    }

    // It worked, we're done
    qb3_destroy_encoder(encoder);
    return nullptr;
}

NS_END