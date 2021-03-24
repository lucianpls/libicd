#include "icd_codecs.h"
#include <unordered_map>

NS_ICD_START

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

NS_END