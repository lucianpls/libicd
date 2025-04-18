/*
* JPEG8.cpp
*
* libahtse JPEG 8bit codec implementation
*
* (C) Lucian Plesea 2018-2025
*/

#include "JPEG_codec.h"

extern "C" {
#include <jpeglib.h>
#include <jerror.h>
}

NS_ICD_START

static void emitMessage(j_common_ptr cinfo, int msgLevel);
static void errorExit(j_common_ptr cinfo);

struct JPGHandle {
    jmp_buf setjmpBuffer;
    // A place to hold a message
    char *message;
    // Pointer to Zen chunk
    storage_manager zenChunk;
};

static void emitMessage(j_common_ptr cinfo, int msgLevel)
{
    JPGHandle* jh = static_cast<JPGHandle *>(cinfo->client_data);
    if (msgLevel > 0) return; // No trace msgs

    // There can be many warnings, just store the first one
    if (cinfo->err->num_warnings++ > 1)
        return;
    cinfo->err->format_message(cinfo, jh->message);
}

// No return to caller
static void errorExit(j_common_ptr cinfo)
{
    JPGHandle* jh = static_cast<JPGHandle *>(cinfo->client_data);
    cinfo->err->format_message(cinfo, jh->message);
    longjmp(jh->setjmpBuffer, 1);
}

/**
*\Brief Do nothing stub function for JPEG library, called
*/
static void stub_source_dec(j_decompress_ptr /* cinfo */) {}

/**
*\Brief: Called when libjpeg gets an unknwon chunk
* It should skip l bytes of input, otherwise jpeg will throw an error
*
*/
static void skip_input_data_dec(j_decompress_ptr cinfo, long l) {
    struct jpeg_source_mgr *src = cinfo->src;
    if (static_cast<size_t>(l) > src->bytes_in_buffer)
        l = static_cast<long>(src->bytes_in_buffer);
    src->bytes_in_buffer -= l;
    src->next_input_byte += l;
}

// Destination should be already set up
static void init_or_terminate_destination(j_compress_ptr /* cinfo */) {}

/**
*\Brief: Do nothing stub function for JPEG library, called?
*/
static boolean fill_input_buffer_dec(j_decompress_ptr /* cinfo */) { return TRUE; }

// Called if the buffer provided is too small
// Can't keep returning false, it will get called forever
static boolean empty_output_buffer(j_compress_ptr cinfo) { 
    // Use EMS write message as a flag
    ERREXIT(cinfo, JERR_EMS_WRITE);
    // Not reached
    return FALSE;
}

//
// JPEG marker processor, for the Zen app3 marker
// Can't return error, only works if the Zen chunk is fully in buffer
// Since this decoder has the whole JPEG in memory, we can just store a pointer
//
#define CHUNK_NAME "Zen"
#define CHUNK_NAME_SIZE 4

// This behaves like a skip_input_data_dec
static boolean zenChunkHandler(j_decompress_ptr cinfo) {

    struct jpeg_source_mgr *src = cinfo->src;
    if (src->bytes_in_buffer < 2)
        ERREXIT(cinfo, JERR_CANT_SUSPEND);

    // Big endian length, read two bytes
    int len = (*src->next_input_byte++) << 8;
    len += *src->next_input_byte++;
    // The length includes the two bytes we just read
    src->bytes_in_buffer -= 2;
    len -= 2;
    // Check that it is safe to read the rest
    if (src->bytes_in_buffer < static_cast<size_t>(len))
        ERREXIT(cinfo, JERR_CANT_SUSPEND);

    // filter out chunks that have the wrong signature, just skip them
    if (strcmp(reinterpret_cast<const char *>(src->next_input_byte), "Zen")) {
        src->bytes_in_buffer -= len;
        src->next_input_byte += len;
        return true;
    }

    // Skip the signature and keep a direct chunk pointer
    src->bytes_in_buffer -= CHUNK_NAME_SIZE;
    src->next_input_byte += CHUNK_NAME_SIZE;
    len -= static_cast<int>(CHUNK_NAME_SIZE);

    JPGHandle *jh = reinterpret_cast<JPGHandle *>(cinfo->client_data);
    // Store a pointer to the Zen chunk in the handler
    jh->zenChunk.buffer = (char *)(src->next_input_byte);
    jh->zenChunk.size = len;

    src->bytes_in_buffer -= len;
    src->next_input_byte += len;
    return true;
}

//
// IMPROVE: could reuse the cinfo, to save some memory allocation
// IMPROVE: Use a jpeg memory manager to link JPEG memory into apache's pool mechanism
//
// Byte data decompressor
// Returns an error message or nullptr if the was no error
// A non-zero params.modified on return means that there was a Zen chunk that had an effect
//

const char *jpeg8_stride_decode(codec_params &params, storage_manager &src, void *buffer)
{
    static_assert(sizeof(params.error_message) >= JMSG_LENGTH_MAX,
        "Message buffer too small");
    params.error_message[0] = 0; // Clear errors

    if (getTypeSize(params.raster.dt) != ICDT_Byte) {
        sprintf(params.error_message, "JPEG8 decode called with wrong datatype");
        return params.error_message;
    }

    jpeg_decompress_struct cinfo;
    JPGHandle jh;
    memset(&jh, 0, sizeof(jh));
    jpeg_error_mgr err;
    memset(&err, 0, sizeof(err));
    // JPEG error message goes directly in the parameter error message space
    jh.message = params.error_message;
    struct jpeg_source_mgr s = { (JOCTET *)src.buffer, static_cast<size_t>(src.size) };

    cinfo.err = jpeg_std_error(&err);
    // Set these after hooking up the standard error methods
    err.error_exit = errorExit;
    err.emit_message = emitMessage;

    // And set our functions
    s.term_source = s.init_source = stub_source_dec;
    s.skip_input_data = skip_input_data_dec;
    s.fill_input_buffer = fill_input_buffer_dec;
    s.resync_to_restart = jpeg_resync_to_restart;
    cinfo.client_data = &jh;

    if (setjmp(jh.setjmpBuffer)) {
        // errorExit comes here
        jpeg_destroy_decompress(&cinfo);
        return params.error_message;
    }

    jpeg_create_decompress(&cinfo);
    cinfo.src = &s;
    // Set the zen chunk reader before reading the header
    jpeg_set_marker_processor(&cinfo, JPEG_APP0 + 3, zenChunkHandler);
    jpeg_read_header(&cinfo, TRUE);
    cinfo.dct_method = JDCT_FLOAT;

    auto const& rsize = params.raster.size;
    if (!(rsize.c == 1 || rsize.c == 3))
        sprintf(params.error_message, "JPEG with wrong number of components");

    if (jpeg_has_multiple_scans(&cinfo) || cinfo.arith_code)
        sprintf(params.error_message, "Unsupported JPEG type");

    if (cinfo.data_precision != 8)
        sprintf(params.error_message, "JPEG with more than 8 bits of data");

    if (cinfo.image_width != rsize.x || cinfo.image_height != rsize.y)
        sprintf(params.error_message, "Wrong JPEG size on input");

    // In bytes
    auto line_stride = params.line_stride;
    if (0 == line_stride)
        line_stride = rsize.c * rsize.x;

    // Only if the error message hasn't been set already
    if (params.error_message[0] == 0) {
        // Force output to desired number of channels
        cinfo.out_color_space = (rsize.c == 3) ? JCS_RGB : JCS_GRAYSCALE;
        jpeg_start_decompress(&cinfo);
        while (cinfo.output_scanline < cinfo.image_height) {
            JSAMPLE* rp[2]; // Two lines at a time
            // Do the math in bytes, because line_stride is in bytes
            rp[0] = (JSAMPROW)((char *)buffer + line_stride * cinfo.output_scanline);
            rp[1] = rp[0] + line_stride;
            jpeg_read_scanlines(&cinfo, JSAMPARRAY(rp), 2);
        }

        jpeg_finish_decompress(&cinfo);
    }

    jpeg_destroy_decompress(&cinfo);

    // If we have an error, return now
    if (params.error_message[0] != 0)
        return params.error_message;

    params.modified = 0; // By default, report no mask or no corrections

    // If a Zen chunk was encountered, apply it
    if (nullptr != jh.zenChunk.buffer) {
        // Mask defaults to full
        BitMap2D<> bm(static_cast<unsigned int>(rsize.x), static_cast<unsigned int>(rsize.y));

        // A zero size zen chunk means all pixels are not black, matching the full mask
        if (jh.zenChunk.size != 0) { // Read the mask from the chunk only for partial masks
            RLEC3Packer packer;
            bm.set_packer(&packer);
            if (!bm.load(&jh.zenChunk)) {
                sprintf(params.error_message, "Error decoding Zen mask");
                return params.error_message;
            }
        }

        params.modified = apply_mask(&bm,
            reinterpret_cast<JSAMPROW>(buffer),
            static_cast<int>(rsize.c),
            static_cast<int>(line_stride));
    }

    return nullptr; // nullptr on success
}


// The buffer is contiguous, so we can just scan it, count the zero pixels 
static size_t nzeros(void* src, jpeg_params& params)
{
    auto s = reinterpret_cast<JSAMPLE*>(src);
    size_t nzeros = 0;
    int w = (int)params.raster.size.x;
    int h = (int)params.raster.size.y;
    int bands = (int)params.raster.size.c;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bool zero = true;
            for (int c = 0; c < bands; c++)
                zero &= (s[c + x * bands] == 0);
            if (zero)
                nzeros++;
        }
        s += params.line_stride;
    }
    return nzeros;
}

static size_t update_mask(BitMask& mask, void* src, jpeg_params& params)
{
    auto s = reinterpret_cast<JSAMPLE*>(src);

    size_t nzeros = 0;
    int w = mask.getWidth();
    int h = mask.getHeight();
    int bands = (int)params.raster.size.c;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            bool zero = true;
            for (int c = 0; c < bands; c++)
                zero &= (s[c + x * bands] == 0);
            if (zero) {
                mask.clear(x, y);
                nzeros++;
            }
        }
        s += params.line_stride;
    }
    return nzeros;
}

const char *jpeg8_encode(jpeg_params &params, storage_manager &src, storage_manager &dst)
{
    struct jpeg_compress_struct cinfo;
    jpeg_error_mgr err;
    JPGHandle jh;
    jpeg_destination_mgr mgr;
    size_t linesize;
    memset(&jh, 0, sizeof(jh));

    mgr.next_output_byte = (JOCTET *)dst.buffer;
    mgr.free_in_buffer = dst.size;
    mgr.init_destination = init_or_terminate_destination;
    mgr.empty_output_buffer = empty_output_buffer;
    mgr.term_destination = init_or_terminate_destination;
    memset(&err, 0, sizeof(err));
    cinfo.err = jpeg_std_error(&err);
    err.error_exit = errorExit;
    err.emit_message = emitMessage;
    auto const& rsize = params.raster.size;

    // This will manage the mask buffer storage
    std::vector<unsigned char> maskbuff(CHUNK_NAME_SIZE);
    // If we don't need one, it's just the empty signature
    memcpy(maskbuff.data(), CHUNK_NAME, CHUNK_NAME_SIZE);
    jh.zenChunk.buffer = maskbuff.data();
    jh.zenChunk.size = 0;

    // might need a mask
    if (nzeros(src.buffer, params) > 0) {
        BitMask mask((unsigned int)rsize.x, (unsigned int)rsize.y);
        update_mask(mask, src.buffer, params);
        jh.zenChunk.size = 2 * mask.size();
        maskbuff.resize(jh.zenChunk.size + CHUNK_NAME_SIZE);
        jh.zenChunk.buffer = &maskbuff[CHUNK_NAME_SIZE]; // Skip the name
        mask.set_packer(new RLEC3Packer);
        mask.store(&jh.zenChunk);
        // Adjust the zenChunk
        jh.zenChunk.size += CHUNK_NAME_SIZE;
        jh.zenChunk.buffer = maskbuff.data();
    }

    jh.message = params.error_message;
    cinfo.client_data = &jh;
    params.error_message[0] = 0; // Clear error messages

    if (setjmp(jh.setjmpBuffer)) {
        jpeg_destroy_compress(&cinfo);
        return params.error_message;
    }

    jpeg_create_compress(&cinfo);
    cinfo.dest = &mgr;
    cinfo.image_width = static_cast<JDIMENSION>(rsize.x);
    cinfo.image_height = static_cast<JDIMENSION>(rsize.y);
    cinfo.input_components = static_cast<int>(rsize.c);
    cinfo.in_color_space = (rsize.c == 3) ? JCS_RGB : JCS_GRAYSCALE;

    jpeg_set_defaults(&cinfo);

    jpeg_set_quality(&cinfo, params.quality, TRUE);
    cinfo.dct_method = JDCT_FLOAT;
    linesize = static_cast<size_t>(cinfo.image_width) * cinfo.num_components;

    jpeg_start_compress(&cinfo, TRUE);
    // Always write the Zen app chunk
    jpeg_write_marker(&cinfo, JPEG_APP0 + 3, 
        (JOCTET *)jh.zenChunk.buffer, (unsigned int)jh.zenChunk.size);

    const JSAMPROW rowbuffer = reinterpret_cast<JSAMPROW>(src.buffer);
    while (cinfo.next_scanline != cinfo.image_height) {
        JSAMPLE* rp[2];
        rp[0] = rowbuffer + linesize * cinfo.next_scanline;
        rp[1] = rp[0] + linesize;
        jpeg_write_scanlines(&cinfo, JSAMPARRAY(rp), 2);
    }
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    dst.size -= static_cast<int>(mgr.free_in_buffer);

    return params.error_message[0] != 0 ?
        params.error_message : nullptr;
}

NS_END // ICD
