
#include "openVCB.h"
#include "openVCBExpr.h"
#include <fstream>

namespace openVCB {
	using namespace std;
	using namespace glm;

	bool prefix(const char* str, const char* pre) {
		return strncmp(pre, str, strlen(pre)) == 0;
	}

	string getNext(char* buff, int& pos) {
		while (buff[pos] == ' ') pos++;
		char* ptr = strchr(buff + pos, ' ');
		if (!ptr) {
			string res = string(buff + pos);
			pos = -1;
			return res;
		}
		else {
			string res = string(buff + pos, ptr);
			while (*ptr == ' ') ptr++;
			pos = ptr - buff;
			return res;
		}
	}

	string getNextLine(char* buff, int& pos) {
		while (buff[pos] && (buff[pos] == ' ' || buff[pos] == ';' || buff[pos] == '\n')) pos++;
		int start = pos;
		int end = -1;
		while (buff[pos] && buff[pos] != '\n' && buff[pos] != ';' && buff[pos]) {
			if (end < 0 && buff[pos] == '#') end = pos;
			pos++;
		}
		if (end < 0) end = pos;
		return string(buff + start, buff + end);
	}

	void Project::assembleVmem(char* err) {
		if (!vmem) return;
		// printf("%s\n", assembly.c_str());

		// Scan through everything once to obtain values for labels
		assemblySymbols.clear();
		int loc = 1;
		int lineLoc = 0;
		while (lineLoc != assembly.size()) {
			// Read a line in.
			string line = getNextLine((char*)assembly.c_str(), lineLoc);
			if (line.size() == 0) continue;
			char* buff = (char*)line.c_str();
			// printf("Line: %s\n", buff);

			// Parse this stuff
			if (prefix(buff, "symbol") || prefix(buff, "resymb")) {}
			else if (prefix(buff, "unsymb")) {}
			else if (prefix(buff, "unpoint")) {}
			else if (prefix(buff, "pointer") || prefix(buff, "repoint")) {
				int k = 7;
				string label = getNext(buff, k);
				string addr = getNext(buff, k);
				if (addr == "inline") loc++;
			}
			else if (buff[0] == '@') {
				int k = 1;
				string label = getNext(buff, k);
				assemblySymbols[label] = loc;
			}
			else if (prefix(buff, "bookmark")) {}
			else if (prefix(buff, "sub_bookmark")) {}
			else loc++;
		}

		// Scan through everything again to assemble
		loc = 1;
		lineLoc = 0;
		while (lineLoc != assembly.size()) {
			// Read a line in.
			string line = getNextLine((char*)assembly.c_str(), lineLoc);
			if (line.size() == 0) continue;
			char* buff = (char*)line.c_str();
			// printf("Line: %s\n", buff);

			// Parse this stuff
			if (prefix(buff, "symbol") || prefix(buff, "resymb")) {
				int k = 6;
				string label = getNext(buff, k);
				auto val = evalExpr(buff + k, assemblySymbols, err);
				assemblySymbols[label] = val;
			}
			else if (prefix(buff, "unsymb")) {
				int k = 6;
				string label = getNext(buff, k);
				assemblySymbols.erase(label);
			}
			else if (prefix(buff, "pointer") || prefix(buff, "repoint")) {
				int k = 7;
				string label = getNext(buff, k);
				string addr = getNext(buff, k);
				auto addrVal = addr == "inline" ? loc++ : evalExpr(addr.c_str(), assemblySymbols, err);
				auto val = evalExpr(buff + k, assemblySymbols, err);

				assemblySymbols[label] = addrVal;
				vmem[addrVal % vmemSize] = val;
			}
			else if (prefix(buff, "unpoint")) {
				int k = 7;
				string label = getNext(buff, k);
				assemblySymbols.erase(label);
			}
			else if (buff[0] == '@') {}
			else if (prefix(buff, "bookmark")) {}
			else if (prefix(buff, "sub_bookmark")) {}
			else {
				auto val = evalExpr(buff, assemblySymbols, err);

				// printf("%s (0x%08x=0x%08x)\n", buff, loc, val);

				vmem[(loc++) % vmemSize] = val;
			}
			if (loc >= vmemSize) {
				if (err) strcpy_s(err, 256, "VMem exceeded.");
				break;
			}
		}

		// Set VMem latch ids
		for (int i = 0; i < vmAddr.numBits; i++) {
			ivec2 pos = vmAddr.pos + i * vmAddr.stride;
			vmAddr.gids[i] = indexImage[pos.x + pos.y * width];
			if (vmAddr.gids[i] == -1 ||
				setOff((Ink)states[vmAddr.gids[i]].ink) != Ink::LatchOff) {
				printf("error: No address latch at VMem position %d %d\n", pos.x, pos.y);
				exit(-1);
			}
		}
		for (int i = 0; i < vmData.numBits; i++) {
			ivec2 pos = vmData.pos + i * vmData.stride;
			vmData.gids[i] = indexImage[pos.x + pos.y * width];
			if (vmAddr.gids[i] == -1 ||
				setOff((Ink)states[vmData.gids[i]].ink) != Ink::LatchOff) {
				printf("error: No data latch at VMem position %d %d\n", pos.x, pos.y);
				exit(-1);
			}
		}

		// dumpVMemToText("vmemDump.bin");
	}

	void Project::dumpVMemToText(string p) {
		FILE* file;
		fopen_s(&file, p.c_str(), "w");
		for (size_t i = 0; i < vmemSize; i++)
			fprintf(file, "a: 0x%08x = 0x%08x\n", i, vmem[i]);
		fclose(file);
	}
}