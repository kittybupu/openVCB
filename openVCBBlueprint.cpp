#include "openVCB.h"


namespace openVCB
{
static constexpr uint32_t B64index[256] = {
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 62, 63, 62, 62, 63, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 0, 0, 0, 0, 0,
      0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
      19, 20, 21, 22, 23, 24, 25, 0, 0, 0, 0, 63, 0, 26, 27, 28, 29, 30, 31, 32, 33,
      34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51
};

std::vector<uint8_t>
b64decode(std::string const &data)
{
      size_t const   len    = data.length();
      uint32_t const pad    = len > 0 && (len % 4 || data[len - 1] == '=');
      size_t const   size   = ((len + 3) / 4 - pad) * 4;
      auto           result = std::vector<uint8_t>(size / 4 * 3 + pad);

      for (size_t i = 0, j = 0; i < size; i += 4) {
            uint32_t n = B64index[data[i]] << 18 |
                         B64index[data[i + 1]] << 12 |
                         B64index[data[i + 2]] << 6 |
                         B64index[data[i + 3]];

            result[j++] = n >> 16;
            result[j++] = n >> 8 & 0xFF;
            result[j++] = n & 0xFF;
      }

      if (pad) {
            uint32_t n = B64index[data[size]] << 18 | B64index[data[size + 1]] << 12;
            result[result.size() - 1] = n >> 16;

            if (len > size + 2 && data[size + 2] != '=') {
                  n |= B64index[data[size + 2]] << 6;
                  result.push_back(n >> 8 & 0xFF);
            }
      }

      return result;
}

bool
isBase64(std::string const &text)
{
      size_t const len = text.length();
      size_t       i   = 0;

      // must be multiple of 4
      if (len % 4 != 0)
            return false;

      /* It wouldn't surprise me if this were actually slower. */
      return std::ranges::all_of(text, [len, &i](char const ch) {
            bool const ret = ::isalnum(ch) || ch == '/' || ch == '+' ||
                             (i >= len - 3 && ch == '=');
            ++i;
            return !ret;
      });

#if 0
      // valid characters only
      for (size_t i = 0; i < len; i++) {
            char const ch = text[i];
            if (!(::isalnum(ch) || ch == '/' || ch == '+' || (i >= len - 3 && ch == '=')))
                  return false;
      }

      return true;
#endif
}

std::string
removeWhitespace(std::string str)
{
      str.erase(std::ranges::remove_if(str, ::isspace).begin(), str.end());
      return str;
}

bool
Project::readFromBlueprint(std::string clipboardData) // XXX: Does this need to be a copy?
{
      clipboardData = removeWhitespace(clipboardData);

      if (!isBase64(clipboardData))
            return false;

      std::vector<uint8_t> logicData = b64decode(clipboardData);

      // check minimum size: zstd magic number [4] + vcb header [32]
      if (logicData.size() <= 36)
            return false;

      union char2int {
            uint8_t const * ch;
            uint32_t const *i;
      };
      char2int const dummy = {.ch = logicData.data()};

      // Check zstd magic number, and do it in a way that doesn't invoke
      // undefined behavior.
      if (dummy.i[0] != 0xFD2FB528U)
            return false;

      return processLogicData(logicData, 32);
}
} // namespace openVCB
