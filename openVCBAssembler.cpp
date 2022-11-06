
#include "openVCB.hh"
#include "openVCBExpr.hh"

#include "utils.hh"

#ifdef _WIN64
typedef signed __int64 ssize_t;
#else
typedef signed int ssize_t;
#endif


namespace openVCB {


template <size_t N>
static constexpr bool
prefix(char const *str, char const (&pre)[N])
{
      return strncmp(pre, str, N - 1) == 0;
}

static std::string
getNext(char *buff, size_t &pos)
{
      while (buff[pos] == ' ' || buff[pos] == '\t')
            pos++;
      char *ptr = strchr(buff + pos, ' ');
      char *tab = strchr(buff + pos, '\t');

      if (ptr == nullptr || (tab != nullptr && tab < ptr))
            ptr = tab;

      if (!ptr) {
            // XXX Does this really need to be a copy?
            auto res = std::string(buff + pos);
            pos      = -1;
            return res;
      }

      // XXX This too: wouldn't a string view work?
      auto res = std::string(buff + pos, ptr);
      while (*ptr == ' ' || *ptr == '\t')
            ++ptr;
      pos = ptr - buff;

      return res;
}

static std::string
getNextLine(char *buff, size_t &pos, size_t &lineNum)
{
      while (buff[pos] && (::isspace(buff[pos]) || buff[pos] == ';'))
            if (buff[pos++] == '\n')
                  ++lineNum;

      size_t const start = pos;
      ssize_t      end   = -1;

      while (buff[pos] && buff[pos] != '\n' && buff[pos] != ';') {
            if (end < 0 && buff[pos] == '#')
                  end = static_cast<ssize_t>(pos);
            ++pos;
      }

      if (buff[pos] == '\n')
            ++lineNum;
      if (end < 0)
            end = static_cast<ssize_t>(pos);

      // XXX Again here, must this be a copy?
      return {buff + start, buff + end};
}

void
Project::assembleVmem(char *err, size_t errSize)
{
      if (!vmem)
            return;
      lineNumbers.clear();
      // printf("%s\n", assembly.c_str());

      // Scan through everything once to obtain values for labels
      assemblySymbols.clear();
      size_t loc       = 1;
      size_t lineLoc   = 0;
      size_t lineNum   = 0;
      char  *asmBuffer = assembly.data();

      while (lineLoc < assembly.size())
      {
            // Read a line in.
            std::string line = getNextLine(asmBuffer, lineLoc, lineNum);
            if (line.empty())
                  continue;
            char *buff = line.data();
            // printf("Line: %s\n", buff);

            // Parse this stuff
            if (prefix(buff, "symbol") || prefix(buff, "resymb")) {
            } else if (prefix(buff, "unsymb")) {
            } else if (prefix(buff, "unpoint")) {
            } else if (prefix(buff, "pointer") || prefix(buff, "repoint")) {
                  size_t k = 7;
                  std::string label = getNext(buff, k);
                  std::string addr  = getNext(buff, k);
                  if (addr == "inline")
                        ++loc;
            }
            else if (buff[0] == '@') {
                  size_t k = 1;
                  std::string label      = getNext(buff, k);
                  assemblySymbols[label] = loc;
            }
            else if (prefix(buff, "bookmark")) {
            } else if (prefix(buff, "sub_bookmark")) {
            } else {
                  ++loc;
            }
      }

      // Scan through everything again to assemble
      loc     = 1;
      lineLoc = 0;
      lineNum = 0;

      while (lineLoc != assembly.size()) {
            size_t lNum = lineNum;
            // Read a line in.
            std::string line = getNextLine(asmBuffer, lineLoc, lineNum);
            if (line.empty())
                  continue;

            char *buff = line.data();

            // Parse this stuff
            if (prefix(buff, "symbol") || prefix(buff, "resymb")) {
                  size_t k = 6;
                  std::string label = getNext(buff, k);
                  auto val = evalExpr(buff + k, assemblySymbols, err);
                  assemblySymbols[label] = val;
            }
            else if (prefix(buff, "unsymb")) {
                  size_t k = 6;
                  std::string label = getNext(buff, k);
                  assemblySymbols.erase(label);
            }
            else if (prefix(buff, "pointer") || prefix(buff, "repoint")) {
                  size_t k = 7;
                  std::string label   = getNext(buff, k);
                  std::string addr    = getNext(buff, k);
                  auto        addrVal = addr == "inline"
                                            ? loc++
                                            : evalExpr(addr.c_str(), assemblySymbols, err);
                  auto val = evalExpr(buff + k, assemblySymbols, err);

                  addrVal                = addrVal % vmemSize;
                  assemblySymbols[label] = addrVal;
                  vmem[addrVal]          = val;
                  lineNumbers[addrVal]   = lNum;
            }
            else if (prefix(buff, "unpoint")) {
                  size_t k = 7;
                  std::string label = getNext(buff, k);
                  assemblySymbols.erase(label);
            } else if (buff[0] == '@') {
                  /* TODO */
            } else if (prefix(buff, "bookmark")) {
                  /* TODO */
            } else if (prefix(buff, "sub_bookmark")) {
                  /* TODO */
            } else {
                  auto val = evalExpr(buff, assemblySymbols, err);

                  // printf("%s (0x%08x=0x%08x)\n", buff, loc, val);
                  auto addrVal         = (loc++) % vmemSize;
                  vmem[addrVal]        = val;
                  lineNumbers[addrVal] = lNum;
            }

            if (loc >= vmemSize) {
                  if (err)
                        strcpy_s(err, errSize, "VMem exceeded.");
                  break;
            }
      }

      // Set VMem latch ids
      for (int i = 0; i < vmAddr.numBits; ++i) {
            glm::ivec2 pos      = vmAddr.pos + i * vmAddr.stride;
            vmAddr.gids[i] = indexImage[pos.x + pos.y * width];
            if (vmAddr.gids[i] == -1 ||
                setOff((Logic)states[vmAddr.gids[i]].logic) != Logic::LatchOff) {
                  ::printf("error: No address latch at VMem position %d %d\n", pos.x,
                           pos.y);
                  ::exit(1); // GAHH NEVER EVER RETURN NEGATIVE VALUES FROM A PROGRAM.
                             // Values MUST be between 0 and 127 inclusive.                                                         
            }
      }

      for (int i = 0; i < vmData.numBits; ++i) {
            glm::ivec2 pos      = vmData.pos + i * vmData.stride;
            vmData.gids[i] = indexImage[pos.x + pos.y * width];
            if (vmAddr.gids[i] == -1 ||
                setOff((Logic)states[vmData.gids[i]].logic) != Logic::LatchOff) {
                  ::printf("error: No data latch at VMem position %d %d\n", pos.x, pos.y);
                  ::exit(1);
            }
      }

      // dumpVMemToText("vmemDump.bin");
}

void
Project::dumpVMemToText(std::string const &p) const
{
      FILE *file;
      fopen_s(&file, p.c_str(), "w");
      for (size_t i = 0; i < vmemSize; i++)
            fprintf(file, "a: 0x%08zx = 0x%08x\n", i, vmem[i]);
      fclose(file);
}

} // namespace openVCB