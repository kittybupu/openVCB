// Code for file reading

#include "openVCB.h"

#include <zstd.h>
#include <vector>
#include <stdio.h>
#include <fstream>
#include <istream>
#include <sstream>
#include <iterator>

namespace openVCB {
	const int colorPallet[] = {
		// Off
		0x000000,

		0x5a5845,
		0x2e475d,
		0x4d383e,
		0x66788e,

		0x3e4d3e,
		0x384b56,
		0x4d392f,

		0x4d3744,
		0x1a3c56,
		0x4d453e,

		0x433d56,
		0x3b2854,

		0x4d243c,
		0x384d47,
		0x323841,

		// On
		0x000000,

		0xa19856,
		0x63b1ff,
		0xff5e5e,
		0x66788e,

		0x92ff63,
		0x63f2ff,
		0xffa200,

		0xff628a,
		0x30d9ff,
		0xffc663,

		0xae74ff,
		0xa600ff,

		0xff0041,
		0x63ff9f,
		0xffffff
	};

	const char* inkNames[] = {
		"None",

		"Trace (Off)",
		"Read (Off)",
		"Write (Off)",
		"Cross",

		"Buffer (Off)",
		"OR (Off)",
		"NAND (Off)",

		"NOT (Off)",
		"NOR (Off)",
		"AND (Off)",

		"XOR (Off)",
		"NXOR (Off)",

		"Clock (Off)",
		"Latch (Off)",
		"LED (Off)",

		"UNDEFINED",

		"Trace (On)",
		"Read (On)",
		"Write (On)",
		"UNDEFINED",

		"Buffer (On)",
		"OR (On)",
		"NAND (On)",

		"NOT (On)",
		"NOR (On)",
		"AND (On)",

		"XOR (On)",
		"NXOR (On)",

		"Clock (On)",
		"Latch (On)",
		"LED (On)",
	};

	const char* getInkString(Ink ink) {
		int i = (int)ink;
		if (i >= 128) i += (int)Ink::numTypes - 128;
		if (i < 0 || i > 2 * (int)Ink::numTypes) return "None";
		return inkNames[i];
	}

	inline int col2int(int col) {
		int r = col & 0xff0000;
		int g = col & 0xff00;
		int b = col & 0xff;
		return (r >> 16) | g | (b << 16);
	}

	Ink color2ink(int col) {
		col = col2int(col);
		Ink ink = Ink::None;

		switch (col) {
		case 0x2a3541:
		case 0x9fa8ae:
		case 0xa1555e:
		case 0xa16c56:
		case 0xa18556:
		case 0xa19856:
		case 0x99a156:
		case 0x88a156:
		case 0x6ca156:
		case 0x56a18d:
		case 0x5693a1:
		case 0x567ba1:
		case 0x5662a1:
		case 0x6656a1:
		case 0x8756a1:
		case 0xa15597:
			ink = Ink::TraceOff;
			break;

		default:
			for (int i = 0; i < 2 * (int)Ink::numTypes; i++)
				if (colorPallet[i] == col) {
					if (i >= (int)Ink::numTypes)
						i += 128 - (int)Ink::numTypes;
					ink = (Ink)i;
					break;
				}
		}

		switch (ink) {
		case Ink::Trace:
		case Ink::Read:
		case Ink::Write:
		case Ink::Buffer:
		case Ink::Or:
		case Ink::And:
		case Ink::Xor:
		case Ink::Not:
		case Ink::Nor:
		case Ink::Nand:
		case Ink::Xnor:
		case Ink::Clock:
		case Ink::Led:
			ink = setOff(ink);
		}

		return ink;
	}

	std::string split(std::string data, const char* t, int& start) {
		int tlen = strlen(t);
		int begin = start;
		int end = data.find(t, start + 1);

		if (end < 0) {
			printf("error: token [%s] not found in .vcb\n", t);
			exit(-1);
		}

		start = end + tlen;
		return data.substr(begin, end - begin);
	}

	bool Project::processLogicData(std::vector<unsigned char> logicData, int headerSize) {
		int* header = (int*)&logicData[logicData.size() - headerSize];

		const int imgDSize = header[5];
		width = header[3];
		height = header[1];
		
		if (imgDSize != width * height * 4) {
			printf("error: header width x height does not match header length");
			return 0;
		}
		
		unsigned char* cc = &logicData[0];
		size_t ccSize = logicData.size() - headerSize;

		unsigned long long const imSize = ZSTD_getFrameContentSize(cc, ccSize);
		
		if (imSize == ZSTD_CONTENTSIZE_ERROR) {
			printf("error: not compressed by zstd!");
			return 0;
		}
		else if (imSize == ZSTD_CONTENTSIZE_UNKNOWN) {
			printf("error: original size unknown!");
			return 0;
		}
		else if (imSize != imgDSize) {
			printf("error: decompressed image data size does not match header");
			return 0;
		}
		else {
			originalImage = new unsigned char[imSize];
			image = new Ink[imSize];
			size_t const dSize = ZSTD_decompress(originalImage, imSize, cc, ccSize);

#pragma omp parallel for schedule(static, 8196)
			for (int i = 0; i < imSize / 4; i++)
				image[i] = color2ink(((int*)originalImage)[i]);
			return 1;
		}
	}

	void Project::readFromVCB(std::string filePath) {
		std::ifstream stream(filePath);
		std::stringstream ss;
		ss << stream.rdbuf();
		std::string godotObj = ss.str();
		stream.close();

		if (godotObj.size() == 0) {
			printf("Could not read file \"%s\"\n", filePath.c_str());
			exit(-1);
		}

		// split out assembly
		int pos = 0;
		split(godotObj, "\"assembly\": \"", pos);
		assembly = split(godotObj, "\",", pos);
		// printf("Loaded assembly %d chars\n", assembly.size());

		// if VMem is enabled
		split(godotObj, "\"is_vmem_enabled\": ", pos);
		auto vmemFlag = split(godotObj, ",", pos) == "true";

		std::vector<unsigned char> logicData;
		split(godotObj, "PoolByteArray(", pos);

		{
			auto dat = split(godotObj, " ), PoolByteArray( ", pos);
			std::stringstream s(dat);
			std::string val;
			while (std::getline(s, val, ','))
				logicData.push_back(atoi(val.c_str() + 1));
		}

		split(godotObj, "\"vmem_settings\": [ ", pos);

		// Get VMem settings
		int vmemArr[14];
		{
			auto dat = split(godotObj, " ]", pos);
			// printf("%s dat", dat.c_str());

			std::stringstream s(dat);
			std::string val;
			for (size_t i = 0; i < 14; i++) {
				std::getline(s, val, ',');
				vmemArr[i] = atoi(val.c_str());
			}
		}

		// Set the vmem settings
		vmAddr.numBits = std::max(0, std::min(vmemArr[0], 32));
		vmAddr.pos.x = vmemArr[1];
		vmAddr.pos.y = vmemArr[2];
		vmAddr.stride.x = -vmemArr[3];
		vmAddr.stride.y = vmemArr[4];
		vmAddr.size.x = vmemArr[5];
		vmAddr.size.y = vmemArr[6];

		vmData.numBits = std::max(0, std::min(vmemArr[7], 32));
		vmData.pos.x = vmemArr[8];
		vmData.pos.y = vmemArr[9];
		vmData.stride.x = -vmemArr[10];
		vmData.stride.y = vmemArr[11];
		vmData.size.x = vmemArr[12];
		vmData.size.y = vmemArr[13];

		if (vmemFlag) {
			vmemSize = 1ull << vmAddr.numBits;
			vmem = new int[vmemSize];
			memset(vmem, 0, 4 * vmemSize);
		}

		if (Project::processLogicData(logicData, 24)) {
			// Overwrite latch locations for vmem
			if (vmemFlag) {
				for (int i = 0; i < vmAddr.numBits; i++) {
					auto start = vmAddr.pos + i * vmAddr.stride;
					auto end = start + vmAddr.size;
					for (auto pos = start; pos.x < end.x; pos.x++) {
						for (pos.y = start.y; pos.y < end.y; pos.y++) {
							if (pos.x < 0 || pos.x >= width ||
								pos.y < 0 || pos.y >= height)
								continue;
							image[pos.x + pos.y * width] = Ink::LatchOff;
						}
					}
				}

				for (int i = 0; i < vmData.numBits; i++) {
					auto start = vmData.pos + i * vmData.stride;
					auto end = start + vmData.size;
					for (auto pos = start; pos.x < end.x; pos.x++) {
						for (pos.y = start.y; pos.y < end.y; pos.y++) {
							if (pos.x < 0 || pos.x >= width ||
								pos.y < 0 || pos.y >= height)
								continue;
							image[pos.x + pos.y * width] = Ink::LatchOff;
						}
					}
				}
			}

			// printf("Loaded image %dx%d (%d bytes)\n", width, height, dSize);
		}
	}
}