#include "src/icd_codecs.h"
#include <iostream>
#include <vector>

using namespace ICD;
using namespace std;

// write and read a PNG RGB image
int testPNG() {
    Raster r = {};
    // x, y, z, c, l
    r.size = { 100, 100, 0, 3, 0 };
    r.dt = ICDT_Byte;
    // Build a PNG codec
    png_params p(r);
    if (string(p.error_message) != "") {
        std::cerr << "Error creating png parameters " << 
            p.error_message << std::endl;
        return 1;
    }
    // Create an input buffer
    vector<uint8_t> vsrc(p.get_buffer_size());
    storage_manager src(vsrc.data(), vsrc.size());
    // Fill in with bytes, some pattern that we can tell
    for (size_t i = 0; i < src.size; i++) {
        ((uint8_t*)src.buffer)[i] = i % 256;
    }
    // Create an output buffer
    std::vector<uint8_t> vdst(p.get_buffer_size() * 2);
    storage_manager dst(vdst.data(), vdst.size());

    // Compress it
    auto message = png_encode(p, src, dst);
    if (message != nullptr) {
        std::cerr << "Error compressing PNG " << 
            message << std::endl;
        return 1;
    }

    // Announce the size
    std::cout << "Compressed size: " << dst.size << std::endl;

    // Create a new raster
    Raster in_raster = {};
    image_peek(dst, in_raster);
    // Should be the same size
    if (in_raster.size != r.size) {
        std::cerr << "Size mismatch on unpack, " << 
            in_raster.size.x << " " << in_raster.size.y << " " <<
            in_raster.size.z << " " << in_raster.size.c << " " <<
            in_raster.size.l << std::endl;
        // And the original
        std::cerr << "Expected " <<
            r.size.x << " " << r.size.y << " " <<
            r.size.z << " " << r.size.c << " " <<
            r.size.l << std::endl;
        return 1;
    }

    // decompress it
    // Create parameters for the decoder
    codec_params p2(in_raster);
    // Create an output buffer
    vector<uint8_t> vdst2(p2.get_buffer_size());
    storage_manager dst2(vdst2.data(), vdst2.size());
    message = stride_decode(p2, dst, dst2.buffer);
    if (message != nullptr) {
        std::cerr << "Error decompressing PNG " <<
            message << std::endl;
        return 1;
    }
    // Compare contents of src and dst2
    for (size_t i = 0; i < src.size; i++) {
        if (((uint8_t*)src.buffer)[i] != ((uint8_t*)dst2.buffer)[i]) {
            std::cerr << "Mismatch at " << i << std::endl;
            return 1;
        }
    }

    return 0;
}

// Write and read an RGB JPEG image
static int testJPEG8() {
    Raster r = {};
    // x, y, z, c, l
    r.size = { 100, 100, 0, 3, 0 };
    r.dt = ICDT_Byte;
    // Build a PNG codec
    jpeg_params p(r);
    p.quality = 85; // Defaults to 75
    if (string(p.error_message) != "") {
        std::cerr << "Error creating JPEG parameters " <<
            p.error_message << std::endl;
        return 1;
    }
    // Create an input buffer
    vector<uint8_t> vsrc(p.get_buffer_size());
    storage_manager src(vsrc.data(), vsrc.size());
    // Fill in with bytes, some pattern that we can tell
    for (size_t i = 0; i < src.size; i++) {
        ((uint8_t*)src.buffer)[i] = i % 256;
    }

    // Make the first pixel black
    ((uint8_t*)src.buffer)[0] = 0;
    ((uint8_t*)src.buffer)[1] = 0;
    ((uint8_t*)src.buffer)[2] = 0;

    // Create an output buffer
    std::vector<uint8_t> vdst(p.get_buffer_size() * 2);
    storage_manager dst(vdst.data(), vdst.size());

    // Compress it
    auto message = jpeg_encode(p, src, dst);
    if (message != nullptr) {
        std::cerr << "Error compressing JPEG " <<
            message << std::endl;
        return 1;
    }

    // Announce the size
    std::cout << "Compressed size: " << dst.size << std::endl;

    // Create a new raster
    Raster in_raster = {};
    image_peek(dst, in_raster);
    // Should be the same size
    if (in_raster.size != r.size) {
        std::cerr << "Size mismatch on unpack, " <<
            in_raster.size.x << " " << in_raster.size.y << " " <<
            in_raster.size.z << " " << in_raster.size.c << " " <<
            in_raster.size.l << std::endl;
        // And the original
        std::cerr << "Expected " <<
            r.size.x << " " << r.size.y << " " <<
            r.size.z << " " << r.size.c << " " <<
            r.size.l << std::endl;
        return 1;
    }

    // decompress it
    // Create parameters for the decoder
    codec_params p2(in_raster);
    // Create an output buffer
    vector<uint8_t> vdst2(p2.get_buffer_size());
    storage_manager dst2(vdst2.data(), vdst2.size());
    message = stride_decode(p2, dst, dst2.buffer);
    if (message != nullptr) {
        std::cerr << "Error decompressing JPEG " <<
            message << std::endl;
        return 1;
    }

    // Compare contents of src and dst2, with some tolerance
    uint8_t* srcb = (uint8_t*)src.buffer;
    uint8_t* dst2b = (uint8_t*)dst2.buffer;
    size_t hist[256] = { 0 };
    for (size_t i = 0; i < src.size; i++)
        hist[abs(srcb[i] - dst2b[i])]++;
    //// Print historgram, comma separated
    //for (size_t i = 0; i < 256; i++) {
    //    std::cout << hist[i] << ",";
    //}

    // Normalized error
    float error = 0;
    for (size_t i = 0; i < 256; i++) {
        error += hist[i] * i;
    }
    error /= src.size;

    std::cout << "Quality " << p.quality << ": average error " << error << std::endl;
    // Fail if error is above 4 (should be 3.8076 for Q = 85)
    if (p.quality == 85 && error > 4) {
        std::cerr << "Error too high" << std::endl;
        return 1;
    }

    return 0;
}

int testJPEG() {
    return testJPEG8();
}

// Write and read a byte LERC raster
int testLERC() {
    Raster r = {};
    // x, y, z, c, l
    r.size = { 100, 100, 0, 1, 0 };
    r.dt = ICDT_Byte;
    // Build a PNG codec
    lerc_params p(r);
    if (string(p.error_message) != "") {
        std::cerr << "Error creating LERC parameters " <<
            p.error_message << std::endl;
        return 1;
    }
    // Create an input buffer
    vector<uint8_t> vsrc(p.get_buffer_size());
    storage_manager src(vsrc.data(), vsrc.size());
    // Fill in with bytes, some pattern that we can tell
    for (size_t i = 0; i < src.size; i++) {
        ((uint8_t*)src.buffer)[i] = i % 256;
    }
    // Create an output buffer
    std::vector<uint8_t> vdst(p.get_buffer_size() * 2);
    storage_manager dst(vdst.data(), vdst.size());

    // Compress it
    auto message = lerc_encode(p, src, dst);
    if (message != nullptr) {
        std::cerr << "Error compressing: " <<
            message << std::endl;
        return 1;
    }

    // Announce the size
    std::cout << "Compressed size: " << dst.size << std::endl;

    // Create a new raster
    Raster in_raster = {};
    //image_peek(dst, in_raster);
    in_raster.init(dst);
    // Should be the same size
    if (in_raster.size != r.size) {
        std::cerr << "Size mismatch on unpack, " <<
            in_raster.size.x << " " << in_raster.size.y << " " <<
            in_raster.size.z << " " << in_raster.size.c << " " <<
            in_raster.size.l << std::endl;
        // And the original
        std::cerr << "Expected " <<
            r.size.x << " " << r.size.y << " " <<
            r.size.z << " " << r.size.c << " " <<
            r.size.l << std::endl;
        return 1;
    }

    // Check that data type is returned as float
    if (in_raster.dt != ICDT_Float32) {
        std::cerr << "Data type invalid for LERC, it should be float, got " <<
            in_raster.dt << std::endl;
        return 1;
    }

    // decompress it as byte
    in_raster.dt = ICDT_Byte;
    // Create parameters for the decoder
    codec_params p2(in_raster);
    // Create an output buffer
    vector<uint8_t> vdst2(p2.get_buffer_size());
    storage_manager dst2(vdst2.data(), vdst2.size());

    message = stride_decode(p2, dst, dst2.buffer);
    if (message != nullptr) {
        std::cerr << "Error decompressing " <<
            message << std::endl;
        return 1;
    }
    // Compare contents of src and dst2
    for (size_t i = 0; i < src.size; i++) {
        if (((uint8_t*)src.buffer)[i] != ((uint8_t*)dst2.buffer)[i]) {
            std::cerr << "Mismatch at " << i << std::endl;
            return 1;
        }
    }

    return 0;
}

// Write and read a QB3 raster
int testQB3() {
    
    if (!has_qb3()) {
        std::cerr << "QB3 codec not available" << std::endl;
        return 1;
    }

    Raster r = {};
    // x, y, z, c, l
    r.size = { 100, 100, 0, 3, 0 };
    r.dt = ICDT_Byte;
    // Build a QB3 codec
    qb3_params p(r);
    if (string(p.error_message) != "") {
        std::cerr << "Error creating QB3 parameters " <<
            p.error_message << std::endl;
        return 1;
    }

    // Create an input buffer
    vector<uint8_t> vsrc(p.get_buffer_size());
    storage_manager src(vsrc.data(), vsrc.size());
    // Fill in with bytes, some pattern that we can tell
    for (size_t i = 0; i < src.size; i++) {
        ((uint8_t*)src.buffer)[i] = i % 256;
    }
    // Create an output buffer
    std::vector<uint8_t> vdst(p.get_buffer_size() * 2);
    storage_manager dst(vdst.data(), vdst.size());

    // Compress it
    auto message = encode_qb3(p, src, dst);
    if (message != nullptr) {
        std::cerr << "Error compressing: " <<
            message << std::endl;
        return 1;
    }

    // Announce the size
    std::cout << "Compressed size: " << dst.size << std::endl;

    // Create a new raster
    Raster in_raster = {};
    in_raster.init(dst);
    // Should be the same size
    if (in_raster.size != r.size) {
        std::cerr << "Size mismatch on unpack, " <<
            in_raster.size.x << " " << in_raster.size.y << " " <<
            in_raster.size.z << " " << in_raster.size.c << " " <<
            in_raster.size.l << std::endl;
        // And the original
        std::cerr << "Expected " <<
            r.size.x << " " << r.size.y << " " <<
            r.size.z << " " << r.size.c << " " <<
            r.size.l << std::endl;
        return 1;
    }

    // Decompress it
    // Create parameters for the decoder
    codec_params p2(in_raster);
    // Create an output buffer
    vector<uint8_t> vdst2(p2.get_buffer_size());
    storage_manager dst2(vdst2.data(), vdst2.size());
    message = stride_decode(p2, dst, dst2.buffer);
    if (message != nullptr) {
        std::cerr << "Error decompressing " <<
            message << std::endl;
        return 1;
    }

    return 0;
}

int main(int argc, char** argv) {
    // Takes one argument, the image format mime type
    if (argc == 2) {
        IMG_T fmt = getFMT(argv[1]);
        if (fmt == IMG_PNG) {
            return testPNG();
        }
        else if (fmt == IMG_JPEG) {
            return testJPEG();
        }
        else if (fmt == IMG_LERC) {
            return testLERC();
        }
#if defined(LIBQB3_FOUND)
        else if (fmt == IMG_QB3) {
            return testQB3();
        }
#endif
        else {
            std::cerr << "Unsupported format " << argv[1] << std::endl;
            std::cerr << "image/jpeg, image/png, raster/lerc" << std::endl; 
            return 1;
        }
    }

    std::cerr << "Usage: testicd <format>" << std::endl;
    return 1;
}