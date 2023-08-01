// ReSharper disable CppTooWideScopeInitStatement
// ReSharper disable StringLiteralTypo
// ReSharper disable CppClangTidyCertErr33C
#include "openVCB.h"
#include "openVCB_Expr.hh"

namespace openVCB {
/****************************************************************************************/


template <size_t N>
static constexpr bool
prefix(char const *str, char const (&pre)[N])
{
      return strncmp(pre, str, N - 1) == 0;
}

static std::string
getNext(char *buf, size_t &pos)
{
      while (buf[pos] == ' ' || buf[pos] == '\t')
            pos++;
      char *ptr = strchr(buf + pos, ' ');
      char *tab = strchr(buf + pos, '\t');

      if (ptr == nullptr || (tab != nullptr && tab < ptr))
            ptr = tab;

      if (!ptr) {
            // XXX Does this really need to be a copy?
            auto res = std::string(buf + pos);
            pos      = -1;
            return res;
      }

      // XXX This too: wouldn't a string view work?
      auto res = std::string(buf + pos, ptr);
      while (*ptr == ' ' || *ptr == '\t')
            ++ptr;
      pos = ptr - buf;

      return res;
}

static std::string
getNextLine(char const *buf, uint32_t &pos, uint32_t &lineNum)
{
      while (buf[pos] && (isspace(buf[pos]) || buf[pos] == ';'))
            if (buf[pos++] == '\n')
                  ++lineNum;

      uint32_t const start = pos;
      int64_t        end   = -1;

      while (buf[pos] && buf[pos] != '\n' && buf[pos] != ';') {
            if (end < 0 && buf[pos] == '#')
                  end = static_cast<int64_t>(pos);
            ++pos;
      }

      if (buf[pos] == '\n')
            ++lineNum;
      if (end < 0)
            end = static_cast<int64_t>(pos);

      // XXX Again here, must this be a copy?
      return {buf + start, buf + end};
}


/*--------------------------------------------------------------------------------------*/


void
Project::assembleVmem(char *errp, size_t errSize)
{
      if (!vmem.i)
            return;
      lineNumbers.clear();
      char errBuf[512];

      // Scan through everything once to obtain values for labels
      assemblySymbols.clear();
      char const *asmBuffer = assembly.data();
      uint32_t    loc       = 1;
      uint32_t    lineNum   = 0;
      uint32_t    lineLoc   = 0;

      while (lineLoc < assembly.size()) {
            // Read a line in.
            std::string line = getNextLine(asmBuffer, lineLoc, lineNum);
            if (line.empty())
                  continue;
            char *buf = line.data();

            // Parse this stuff
            if (prefix(buf, "symbol") || prefix(buf, "resymb")) {
                  /* nothing */
            } else if (prefix(buf, "unsymb")) {
                  /* nothing */
            } else if (prefix(buf, "unpoint")) {
                  /* nothing */
            } else if (prefix(buf, "pointer") || prefix(buf, "repoint")) {
                  size_t      k     = 7;
                  std::string label = getNext(buf, k);
                  std::string addr  = getNext(buf, k);
                  if (addr == "inline")
                        ++loc;
            } else if (buf[0] == '@') {
                  size_t      k          = 1;
                  std::string label      = getNext(buf, k);
                  assemblySymbols[label] = loc;
            } else if (prefix(buf, "bookmark")) {
                  /* nothing */
            } else if (prefix(buf, "sub_bookmark")) {
                  /* nothing */
            } else {
                  ++loc;
            }
      }

      // Scan through everything again to assemble
      loc     = 1;
      lineLoc = 0;
      lineNum = 0;

      while (lineLoc != assembly.size()) {
            uint32_t lNum = lineNum;
            // Read a line in.
            std::string line = getNextLine(asmBuffer, lineLoc, lineNum);
            if (line.empty())
                  continue;

            char *buf = line.data();
            errBuf[0] = '\0';

            // Parse this stuff
            if (prefix(buf, "symbol") || prefix(buf, "resymb")) {
                  size_t      k          = 6;
                  std::string label      = getNext(buf, k);
                  auto        val        = evalExpr(buf + k, assemblySymbols, errBuf);
                  assemblySymbols[label] = val;
            } else if (prefix(buf, "unsymb")) {
                  size_t      k     = 6;
                  std::string label = getNext(buf, k);
                  assemblySymbols.erase(label);
            } else if (prefix(buf, "pointer") || prefix(buf, "repoint")) {
                  size_t      k     = 7;
                  std::string label = getNext(buf, k);
                  std::string addr  = getNext(buf, k);

                  auto addrVal = addr == "inline"
                                       ? loc++
                                       : evalExpr(addr.c_str(), assemblySymbols, errBuf);
                  auto val = evalExpr(buf + k, assemblySymbols, errBuf);

                  addrVal                = addrVal % vmemSize;
                  assemblySymbols[label] = addrVal;
                  vmem.i[addrVal]        = val;
                  lineNumbers[addrVal]   = lNum;
            } else if (prefix(buf, "unpoint")) {
                  size_t      k     = 7;
                  std::string label = getNext(buf, k);
                  assemblySymbols.erase(label);
            } else if (buf[0] == '@') {
                  /* TODO */
            } else if (prefix(buf, "bookmark")) {
                  /* TODO */
            } else if (prefix(buf, "sub_bookmark")) {
                  /* TODO */
            } else {
                  auto val = evalExpr(buf, assemblySymbols, errBuf);

                  // printf("%s (0x%08x=0x%08x)\n", buf, loc, val);
                  auto addrVal         = loc++ % vmemSize;
                  vmem.i[addrVal]      = val;
                  lineNumbers[addrVal] = lNum;
            }

            if (*errBuf)
                  snprintf(errp, errSize, "line %zu: %s", lineNum, errBuf);

            if (loc >= vmemSize) {
                  if (errp)
                        std::ranges::copy("VMem exceeded.", errp);
                  break;
            }
      }

      std::pair<LatchInterface &, char const *> latchWrapper[] = {
            {vmAddr, "address"},
            {vmData, "data"}
      };

      // Set VMem latch ids with mostly pointless but nifty c++ features.
      for (auto &[data, msg] : latchWrapper) {
            for (int i = 0; i < data.numBits; ++i) {
                  glm::ivec2 pos = data.pos + i * data.stride;
                  data.gids[i]   = indexImage[pos.x + pos.y * width];

                  if (data.gids[i] == -1 || SetOff(states[data.gids[i]].logic) != Logic::LatchOff) {
                        ::printf("error: No %s latch at VMem position %d %d\n",
                                 msg, pos.x, pos.y);
                        ::exit(1);
                  }
            }
      }

      // dumpVMemToText("vmemDump.bin");
}

void
Project::dumpVMemToText(std::string const &p) const
{
      FILE *file = ::fopen(p.c_str(), "w");

      if (!file) {
            auto const e = std::system_error{errno, std::generic_category()};
            std::cerr << "Error opening file \"" << p
                      << "\" for VMem dumping: " << e.what() << '\n';
            return;
      }

      for (size_t i = 0; i < vmemSize; ++i)
            ::fprintf(file, "a: 0x%08zx = 0x%08x\n", i, vmem.i[i]);
      ::fclose(file);
}


/****************************************************************************************/
} // namespace openVCB
