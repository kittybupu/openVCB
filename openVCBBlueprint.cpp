#include "openVCB.h"

namespace openVCB {
    static const int B64index[256] = { 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
    0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 62, 63, 62, 62, 63, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,  4,  5,  6,
    7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,  0,
    0,  0,  0, 63,  0, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51 };

    std::vector<unsigned char> b64decode(std::string data) {
        const size_t len = data.length();
        int pad = len > 0 && (len % 4 || data[len - 1] == '=');
        
        const size_t size = ((len + 3) / 4 - pad) * 4;
        std::vector<unsigned char> result(size / 4 * 3 + pad);

        for (size_t i = 0, j = 0; i < size; i += 4) {
            int n = B64index[data[i]] << 18 | B64index[data[i + 1]] << 12 | B64index[data[i + 2]] << 6 | B64index[data[i + 3]];
            result[j++] = n >> 16;
            result[j++] = n >> 8 & 0xFF;
            result[j++] = n & 0xFF;
        }

        if (pad) {
            int n = B64index[data[size]] << 18 | B64index[data[size + 1]] << 12;
            result[result.size() - 1] = n >> 16;

            if (len > size + 2 && data[size + 2] != '=') {
                n |= B64index[data[size + 2]] << 6;
                result.push_back(n >> 8 & 0xFF);
            }
        }
        return result;
    }

    void Project::readFromBlueprint(std::string clipboardData) {
        std::vector<unsigned char> logicData = b64decode(clipboardData);
        Project::processLogicData(logicData, 32);
    }
}
