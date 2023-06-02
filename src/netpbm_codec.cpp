/*
* PNG_codec.cpp
* C++ codec for Netpbm family of formats
*
* (C)Lucian Plesea 2023
*/

#include "icd_codecs.h"
#include <vector>
#include <string>
#include <cstring>
#include <cassert>

NS_ICD_START

// Detect netpbm, return 0 if not recognized
int is_netpbm(const storage_manager& src) {
	unsigned char* buffer = static_cast<unsigned char *>(src.buffer);
	// 'P0' to 'P6'
	return (src.size < 2 || buffer[0] != 'P' || buffer[1] < '1' || buffer[1] > '6');
}

// Get an ascii line, up to a CR
static std::string get_line(unsigned char* s, size_t maxsize) {
	size_t sz = 0;
	while (sz < maxsize && s[sz] != '\n')
		sz++;
	return std::string(s, s + sz);
}

// zero on success
static int get_size(storage_manager &src, Raster & raster) {
	// Keep reading until we get the raster size or an error occurs
	std::string header;
	unsigned char *buffer = static_cast<unsigned char*>(src.buffer);
	unsigned int x, y, maxval;
	while (src.size) {
		auto line = get_line(buffer, src.size);
		// Empty or comment line, skip it
		if (line.empty() or line[0] == '#') {
			buffer += line.size() + 1;
			src.size -= line.size() + 1;
			continue;
		}
		// Got a non-empty line, append it to the header, without the CR
		header.append(line);
		buffer += line.size() + 1;
		src.size -= line.size() + 1;
		// Try reading the three numbers
		auto found = sscanf(header.c_str(), "%u %u %u", &x, &y, &maxval);
		// Done if we found all three values
		if (found == 3) {
			raster.size.x = x;
			raster.size.y = y;
			raster.max = maxval;
			raster.has_max = true;
			raster.min = 0;
			raster.has_min = true;
			raster.has_ndv = false;
			raster.dt = (maxval > 0xffff) ? ICDT_UInt32 :
				(maxval > 0xff) ? ICDT_UInt16 :
				ICDT_Byte;
			// Need to adjust the src, point after the current line
			src.buffer = buffer + 1;
			return 0;
		}
	}
	return -1;
}

const char* netpbm_peek(const storage_manager& src, Raster& raster) {
	if (!is_netpbm(src))
		return "Not a recognized netpbm";
	const unsigned char* buffer = reinterpret_cast<unsigned char*>(src.buffer);
	const unsigned char* sentinel = buffer + src.size;
	raster.format = IMG_NETPBM;
	// Type is the number
	unsigned char* buffer = static_cast<unsigned char*>(src.buffer);
	// Only handler gray and RGB binary
	if (buffer[1] != '5' && buffer[1] != '6')
		return "Only binary grayscale and color netpbm are supported";
	raster.size.c = buffer[1] == '5' ? 1 : 3; // Gray or RGB
}

const char* netpbm_stride_decode(codec_params& params, storage_manager& src, void* buffer) {
}

const char* netpbm_encode(lerc_params& params, storage_manager& src, storage_manager& dst) {
}

NS_END
