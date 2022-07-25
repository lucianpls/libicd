
#include "icd_codecs.h"
#include <QB3.h>

#if !defined(NEED_SWAP)
#error QB3 only works in little endian
#endif

NS_ICD_START

static const char ERR_QB3[] = "Corrupt or invalid QB3";
static const char ERR_SMALL[] = "Input buffer too small";
static const char ERR_DIFFERENT[] = "Unexpected type of QB3";

// Skims over a QB3 file, checks that raster expectations are met
const char* peek_qb3(const storage_manager& src, Raster& raster)
{
    if (src.size < 4)
        return ERR_SMALL;
    size_t sizes[3]; // X, Y, C
    decsp p = 
        qb3_read_start(src.buffer, src.size, sizes);
    if (!p)
        return ERR_QB3;
    if (raster.size.x != sizes[0] && raster.size.y != sizes[1] && raster.size.c != sizes[2]) {
        qb3_destroy_decoder(p);
        return ERR_DIFFERENT;
    }
    // Could check a few more things, data type ...
    if (qb3_decoded_size(p) != getTypeSize(raster.dt, sizes[0] * sizes[1] * sizes[2])) {
        qb3_destroy_decoder(p);
        return ERR_DIFFERENT;
    }
        //// Looks reasonable, try the read_info to see if we can actually decode it
    //if (!qb3_read_info(p)) {
    //	qb3_destroy_decoder(p);
    //	return ERR_QB3;
    //}

    qb3_destroy_decoder(p);
    return nullptr;
}

const char* stride_decode_qb3(codec_params& params, storage_manager& src, void* buffer)
{
    return "NOT IMPLEMENTED";
}

const char* encode_qb3(lerc_params& params, storage_manager& src, storage_manager& dst)
{
    return "NOT IMPLEMENTED";
}

NS_END