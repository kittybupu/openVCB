// Code for file reading

#include "openVCB.hh"
#include "openVCB_Data.hh"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <zstd.h>

/*--------------------------------------------------------------------------------------*/


namespace openVCB {


char const *
getInkString(Ink ink)
{
      int i = (int)ink;
      if (i >= 128)
            i += (int)Ink::numTypes - 128;
      if (i < 0 || i > 2 * (int)Ink::numTypes)
            return "None";
      return inkNames[i].data();
}

inline int
col2int(int col)
{
      int r = col & 0xFF0000;
      int g = col & 0xFF00;
      int b = col & 0xFF;
      return (r >> 16) | g | (b << 16);
}


InkPixel
color2ink(int col)
{
      col          = col2int(col);
      InkPixel pix = {};
      for (int i = 0; i < 16; i++)
            if (col == traceColors[i]) {
                  pix.meta = (int16_t)i;
                  pix.ink  = Ink::Trace;
                  break;
            }

      switch (col) {
      case 0x3A4551:
            pix.ink = Ink::Annotation;
            break;

      case 0x8CABA1:
            pix.ink = Ink::Filler;
            break;

      default:
            for (int i = 0; i < 2 * (int)Ink::numTypes; i++)
                  if (colorPallet[i] == col) {
                        if (i >= (int)Ink::numTypes)
                              i += 128 - (int)Ink::numTypes;
                        pix.ink = (Ink)i;
                        break;
                  }
      }

      return pix;
}

std::string
split(std::string data, char const *t, int &start)
{
      int tlen  = strlen(t);
      int begin = start;
      int end   = data.find(t, start + 1);

      if (end < 0) {
            printf("error: token [%s] not found in .vcb\n", t);
            exit(-1);
      }

      start = end + tlen;
      return data.substr(begin, end - begin);
}

bool
processData(std::vector<unsigned char> logicData,
            int                        headerSize,
            int                       &width,
            int                       &height,
            unsigned char            *&originalImage,
            unsigned long long        &imSize)
{
      int *header = (int *)&logicData[logicData.size() - headerSize];

      int const imgDSize = header[5];
      width              = header[3];
      height             = header[1];

      if (imgDSize != width * height * 4) {
            printf("Error: header width x height does not match header length");
            return false;
      }

      unsigned char *cc     = logicData.data();
      size_t         ccSize = logicData.size() - headerSize;

      imSize = ZSTD_getFrameContentSize(cc, ccSize);

      if (imSize == ZSTD_CONTENTSIZE_ERROR) {
            printf("error: not compressed by zstd!");
            return false;
      } else if (imSize == ZSTD_CONTENTSIZE_UNKNOWN) {
            printf("error: original size unknown!");
            return false;
      } else if (imSize != imgDSize) {
            printf("error: decompressed image data size does not match header");
            return false;
      } else {
            originalImage      = new unsigned char[imSize];
            size_t const dSize = ZSTD_decompress(originalImage, imSize, cc, ccSize);
            return true;
      }
}

bool
Project::processLogicData(std::vector<unsigned char> logicData, int headerSize)
{
      unsigned long long imSize;
      if (processData(logicData, headerSize, width, height, originalImage, imSize)) {
            image = new InkPixel[imSize];
#pragma omp parallel for schedule(static, 8196)
            for (int i = 0; i < imSize / 4; i++)
                  image[i] = color2ink(((int *)originalImage)[i]);

            return true;
      }

      return false;
}

void
Project::processDecorationData(std::vector<unsigned char> decorationData, int *&decoData)
{
      unsigned long long imSize;
      int                width, height;
      if (processData(decorationData, 24, width, height, (unsigned char *&)decoData, imSize))
      {
            for (int i = 0; i < imSize / 4; i++) {
                  int color   = decoData[i];
                  decoData[i] = col2int(color);
            }
      }
}

void
Project::readFromVCB(std::string filePath)
{
      std::ifstream     stream(filePath);
      std::stringstream ss;
      ss << stream.rdbuf();
      std::string godotObj = ss.str();
      stream.close();

      if (godotObj.empty()) {
            printf(R"(Could not read file "%s")"
                   "\n",
                   filePath.c_str());
            exit(1);
      }

      // split out assembly
      int pos = 0;
      split(godotObj, R"("assembly": ")", pos);
      assembly = split(godotObj, "\",", pos);
      // printf("Loaded assembly %d chars\n", assembly.size());

      // if VMem is enabled
      split(godotObj, "\"is_vmem_enabled\": ", pos);
      auto vmemFlag = split(godotObj, ",", pos) == "true";

      std::vector<unsigned char> logicData;
      split(godotObj, "PoolByteArray(", pos);

      {
            auto              dat = split(godotObj, " ), PoolByteArray( ", pos);
            std::stringstream s(dat);
            std::string       val;
            while (std::getline(s, val, ','))
                  logicData.push_back(atoi(val.c_str() + 1));
      }

      auto decorationData = new std::vector<unsigned char>[3];
      {
            pos--;
            auto              dat = split(godotObj, " ), PoolByteArray( ", pos);
            std::stringstream s(dat);
            std::string       val;
            while (std::getline(s, val, ','))
                  decorationData[0].push_back(atoi(val.c_str() + 1));
      }

      {
            pos--;
            auto              dat = split(godotObj, " ), PoolByteArray( ", pos);
            std::stringstream s(dat);
            std::string       val;
            while (std::getline(s, val, ','))
                  decorationData[1].push_back(atoi(val.c_str() + 1));
      }

      {
            pos--;
            auto              dat = split(godotObj, " ) ]", pos);
            std::stringstream s(dat);
            std::string       val;
            while (std::getline(s, val, ','))
                  decorationData[2].push_back(atoi(val.c_str() + 1));
      }

      // led palette
      std::vector<int> vecPalette;
      split(godotObj, "\"led_palette\": [ ", pos);

      auto              dat = split(godotObj, " ]", pos);
      std::stringstream s(dat);
      std::string       val;
      while (std::getline(s, val, ',')) {
            // remove quotes
            val.erase(remove(val.begin(), val.end(), '\"'), val.end());
            vecPalette.emplace_back(static_cast<uint32_t>(std::stoull(val, nullptr, 16)));
      }

      for (int i = 0; i < std::min((int)vecPalette.size(), 16); i++) {
            ledPalette[i] = vecPalette[i];
      }

      split(godotObj, "\"vmem_settings\": [ ", pos);

      // Get VMem settings
      int vmemArr[14];
      {
            auto dat = split(godotObj, " ]", pos);
            // printf("%s dat", dat.c_str());

            std::stringstream s(dat);
            std::string       val;
            for (size_t i = 0; i < 14; i++) {
                  std::getline(s, val, ',');
                  vmemArr[i] = std::stoi(val);
            }
      }

      // Set the vmem settings
      vmAddr.numBits  = std::max(0, std::min(vmemArr[0], 32));
      vmAddr.pos.x    = vmemArr[1];
      vmAddr.pos.y    = vmemArr[2];
      vmAddr.stride.x = -vmemArr[3];
      vmAddr.stride.y = vmemArr[4];
      vmAddr.size.x   = vmemArr[5];
      vmAddr.size.y   = vmemArr[6];

      vmData.numBits  = std::max(0, std::min(vmemArr[7], 32));
      vmData.pos.x    = vmemArr[8];
      vmData.pos.y    = vmemArr[9];
      vmData.stride.x = -vmemArr[10];
      vmData.stride.y = vmemArr[11];
      vmData.size.x   = vmemArr[12];
      vmData.size.y   = vmemArr[13];

      if (vmemFlag) {
            vmemSize = 1ull << vmAddr.numBits;
            vmem     = new int[vmemSize];
            memset(vmem, 0, 4 * vmemSize);
      }

      if (processLogicData(logicData, 24)) {
            // Overwrite latch locations for vmem
            if (vmemFlag) {
                  for (int i = 0; i < vmAddr.numBits; i++) {
                        auto start = vmAddr.pos + i * vmAddr.stride;
                        auto end   = start + vmAddr.size;
                        for (auto pos = start; pos.x < end.x; pos.x++) {
                              for (pos.y = start.y; pos.y < end.y; pos.y++) {
                                    if (pos.x < 0 || pos.x >= width || pos.y < 0 || pos.y >= height)
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

            // printf("Loaded image %dx%d (%d bytes)\n", width, height, dSize);
      }

      processDecorationData(decorationData[0], decoration[0]);
      processDecorationData(decorationData[1], decoration[1]);
      processDecorationData(decorationData[2], decoration[2]);
}

} // namespace openVCB