// Code for file reading

#include "openVCB.h"
#include "openVCB_Data.hh"

#include <zstd.h>

/*--------------------------------------------------------------------------------------*/


namespace openVCB
{
static constexpr int num_types       = static_cast<unsigned>(Ink::numTypes);
static constexpr int num_enumerators = num_types * 2;


char const *
getInkString(Ink const ink)
{
      unsigned i = static_cast<int>(ink);
      if (i >= 128)
            i += num_types - 128;
      if (i < 0 || i > num_enumerators)
            return "None";
      assert(i < std::size(inkNames));
      return inkNames[i].data();
}

inline uint32_t
col2int(uint32_t const col)
{
      uint const r = col & UINT32_C(0xFF0000);
      uint const g = col & UINT32_C(0x00FF00);
      uint const b = col & UINT32_C(0x0000FF);
      return (r >> 16) | g | (b << 16);
}


InkPixel
color2ink(uint32_t col)
{
      InkPixel pix{};
      col = col2int(col);

      for (int i = 0; i < 16; ++i) {
            if (col == traceColors[i]) {
                  pix.meta = i;
                  pix.ink  = Ink::Trace;
                  break;
            }
      }

      switch (col) {
      case 0x3A4551:
            pix.ink = Ink::Annotation;
            break;

      case 0x8CABA1:
            pix.ink = Ink::Filler;
            break;

      default:
            for (int i = 0; i < num_enumerators; ++i) {
                  if (colorPallet[i] == col) {
                        if (i >= num_types)
                              i += 128 - num_types;
                        pix.ink = static_cast<Ink>(i);
                        break;
                  }
            }
      }

      return pix;
}

std::string
split(std::string const &data, char const *t, int &start)
{
      size_t const tlen  = strlen(t);
      int const    begin = start;
      auto const   end   = data.find(t, start + 1);

      if (end == std::string::npos) {
            ::printf("error: token [%s] not found in .vcb\n", t);
            ::exit(1);
      }

      start = static_cast<int>(end + tlen);
      return data.substr(begin, end - begin);
}

bool
processData(std::vector<uint8_t> const &logicData,
            int const                   headerSize,
            int                        &width,
            int                        &height,
            uint8_t                   *&originalImage,
            uint64_t                   &imSize)
{
      auto const *header =
          reinterpret_cast<int const *>(&logicData[logicData.size() - headerSize]);

      int const imgDSize = header[5];
      width              = header[3];
      height             = header[1];

      if (imgDSize != width * height * 4) {
            fputs("Error: header width x height does not match header length\n", stderr);
            return false;
      }

      uint8_t const *cc     = logicData.data();
      size_t const   ccSize = logicData.size() - headerSize;

      imSize = ZSTD_getFrameContentSize(cc, ccSize);

      if (imSize == ZSTD_CONTENTSIZE_ERROR) {
            (void)fputs("error: not compressed by zstd!\n", stderr);
            return false;
      }
      if (imSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            (void)fputs("error: original size unknown!\n", stderr);
            return false;
      }
      if (static_cast<int>(imSize) != imgDSize) {
            (void)fputs("error: decompressed image data size does not match header\n",
                        stderr);
            return false;
      }

      originalImage = new uint8_t[imSize];

      size_t const dSize = ZSTD_decompress(originalImage, imSize, cc, ccSize);

      return dSize == ccSize;
}

bool
Project::processLogicData(std::vector<uint8_t> const &logicData, int32_t const headerSize)
{
      __debugbreak();
      uint64_t imSize;
      if (processData(logicData, headerSize, width, height, originalImage, imSize)) {
            image = new InkPixel[imSize];
#pragma omp parallel for schedule(static, 8196)
            for (int64_t i = 0; i < (int64_t)imSize / 4; ++i)
                  image[i] = color2ink(originalImage[i]);

            return true;
      }

      return false;
}

void
Project::processDecorationData(std::vector<uint8_t> const &decorationData, int *&decoData)
{
      uint64_t imSize = 0;
      int32_t  width;
      int32_t  height;

      union lazy_u {
            int           *i;
            unsigned char *uc;
      };
      lazy_u lazy = {.i = decoData};

      auto const ret = processData(decorationData, 24, width, height, lazy.uc, imSize);
      decoData       = lazy.i;

      assert(decoData != nullptr);
      if (ret) {
            for (uint64_t i = 0; i < imSize / 4; i++) {
                  int const color = decoData[i];
                  decoData[i]     = static_cast<int>(col2int(color));
            }
      }
}

void
Project::readFromVCB(std::string const &filePath)
{
      std::ifstream stream(filePath);
      std::string   godotObj;

      while (!stream.eof())
            stream >> godotObj;

      stream.close();

      if (godotObj.empty()) {
            std::ignore = ::fprintf(stderr, "Could not read file \"%s\"\n",
                                    filePath.c_str());
            ::exit(1); // NOLINT(concurrency-mt-unsafe)
      }

      // split out assembly
      int pos = 0;
      split(godotObj, R"("assembly": ")", pos);
      assembly = split(godotObj, "\",", pos);
      // printf("Loaded assembly %i chars\n", assembly.size());

      // if VMem is enabled
      split(godotObj, "\"is_vmem_enabled\": ", pos);
      auto vmemFlag = split(godotObj, ",", pos) == "true";

      std::vector<uint8_t> logicData;
      split(godotObj, "PoolByteArray(", pos);

      {
            std::string val;
            auto const  dat = split(godotObj, " ), PoolByteArray( ", pos);
            auto        s   = std::stringstream(dat);

            while (std::getline(s, val, ','))
                  logicData.push_back(util::xatoi(val.c_str() + 1));
      }

      auto decorationData = new std::vector<uint8_t>[3];
      {
            std::string val;
            auto const  dat = split(godotObj, " ), PoolByteArray( ", --pos);
            auto        s   = std::stringstream(dat);

            while (std::getline(s, val, ','))
                  decorationData[0].push_back(
                      static_cast<uint8_t>(util::xatoi(val.c_str() + 1)));
      }

      {
            std::string val;
            auto const  dat = split(godotObj, " ), PoolByteArray( ", --pos);
            auto        s   = std::stringstream(dat);

            while (std::getline(s, val, ','))
                  decorationData[1].push_back(
                      static_cast<uint8_t>(util::xatoi(val.c_str() + 1)));
      }

      {
            std::string val;
            auto const  dat = split(godotObj, " ) ]", --pos);
            auto        s   = std::stringstream(dat);

            while (std::getline(s, val, ','))
                  decorationData[2].push_back(
                      static_cast<uint8_t>(util::xatoi(val.c_str() + 1)));
      }

      // led palette
      std::vector<int> vecPalette;
      split(godotObj, "\"led_palette\": [ ", pos);

      {
            std::string val;
            auto const  dat = split(godotObj, " ]", pos);
            auto        s   = std::stringstream(dat);

            while (std::getline(s, val, ',')) {
                  // remove quotes
                  val.erase(std::ranges::remove(val, '\"').begin(), val.end());
                  vecPalette.emplace_back(
                      static_cast<uint32_t>(util::xatou(val.c_str(), 16)));
            }
      }

      for (int i = 0; i < std::min((int)vecPalette.size(), 16); ++i)
            ledPalette[i] = vecPalette[i];

      split(godotObj, "\"vmem_settings\": [ ", pos);

      // Get VMem settings
      int vmemArr[14];
      {
            auto dat = split(godotObj, " ]", pos);
            // printf("%s dat", dat.c_str());

            std::stringstream s(dat);
            std::string       val;
            for (int &i : vmemArr) {
                  std::getline(s, val, ',');
                  i = std::stoi(val);
            }
      }

      // Set the vmem settings
      vmAddr.numBits  = std::clamp(vmemArr[0], 0, 32);
      vmAddr.pos.x    = vmemArr[1];
      vmAddr.pos.y    = vmemArr[2];
      vmAddr.stride.x = -vmemArr[3];
      vmAddr.stride.y = vmemArr[4];
      vmAddr.size.x   = vmemArr[5];
      vmAddr.size.y   = vmemArr[6];

      vmData.numBits  = std::clamp(vmemArr[7], 0, 32);
      vmData.pos.x    = vmemArr[8];
      vmData.pos.y    = vmemArr[9];
      vmData.stride.x = -vmemArr[10];
      vmData.stride.y = vmemArr[11];
      vmData.size.x   = vmemArr[12];
      vmData.size.y   = vmemArr[13];

      __debugbreak();
      if (vmemFlag) {
            vmemSize = UINT64_C(1) << vmAddr.numBits;
#ifdef OVCB_BYTE_ORIENTED_VMEM
            vmem.b = new uint8_t[vmemSize]{};
#else
            vmem.i = new uint32_t[vmemSize]{};
#endif
      }

      if (processLogicData(logicData, 24))
      {
            // Overwrite latch locations for vmem
            if (vmemFlag) {
                  for (int i = 0; i < vmAddr.numBits; i++) {
                        auto start = vmAddr.pos + i * vmAddr.stride;
                        auto end   = start + vmAddr.size;
                        for (auto pos = start; pos.x < end.x; pos.x++) {
                              for (pos.y = start.y; pos.y < end.y; pos.y++) {
                                    if (pos.x < 0 || pos.x >= width || pos.y < 0 ||
                                        pos.y >= height)
                                          continue;
                                    image[pos.x + pos.y * width].ink = Ink::LatchOff;
                              }
                        }
                  }

                  for (int i = 0; i < vmData.numBits; i++) {
                        auto start = vmData.pos + i * vmData.stride;
                        auto end   = start + vmData.size;
                        for (auto pos = start; pos.x < end.x; pos.x++) {
                              for (pos.y = start.y; pos.y < end.y; pos.y++) {
                                    if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
                                          continue;
                                    image[pos.x + pos.y * width].ink = Ink::LatchOff;
                              }
                        }
                  }
            }

            // printf("Loaded image %dx%i (%i bytes)\n", width, height, dSize);
      }

      processDecorationData(decorationData[0], decoration[0]);
      processDecorationData(decorationData[1], decoration[1]);
      processDecorationData(decorationData[2], decoration[2]);
}
} // namespace openVCB
