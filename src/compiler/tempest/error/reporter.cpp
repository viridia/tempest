#include "tempest/error/reporter.h"
#include "tempest/source/programsource.h"
#include <cassert>
#include <iostream>
#include <stdio.h>

#if TEMPEST_HAVE_CSIGNAL
  #include <csignal>
#endif

#if TEMPEST_HAVE_UNISTD_H
  #include <unistd.h>
#endif

#if TEMPEST_HAVE_EXECINFO_H
  #include <execinfo.h>         // For backtrace().
#endif

#if TEMPEST_HAVE_CXXABI_H
  #include <cxxabi.h>
#endif

#if TEMPEST_HAVE_DLFCN_H && __GNUG__
  #include <dlfcn.h>
#endif

namespace tempest::error {
  using tempest::source::Location;

  #define BLACK       ";30"
  #define RED         ";31"
  #define GREEN       ";32"
  #define YELLOW      ";33"
  #define BLUE        ";34"
  #define MAGENTA     ";35"
  #define CYAN        ";36"
  #define WHITE       ";37"

  #define UNCHANGED   ""

  ConsoleReporter ConsoleReporter::INSTANCE;

  static const char * severityNames[SEVERITY_LEVELS] = {
    "debug",
    "status",
    "info",
    "warning",
    "error",
    "fatal",
  };

  //   SAVEDCOLOR

  MessageStream::~MessageStream() {
    flush();
    _reporter->report(_severity, _location, str());
  }

  void ConsoleReporter::writeSpaces(unsigned numSpaces) {
    static const char spaces[] = "                                                                ";
    while (numSpaces > sizeof(spaces) - 1) {
      std::cerr.write(spaces, sizeof(spaces) - 1);
      numSpaces -= sizeof(spaces) - 1;
    }

    std::cerr.write(spaces, numSpaces);
  }

  void ConsoleReporter::changeColor(StringRef color, bool bold) {
    std::cerr << "\033[" << (bold ? "1" : "0") << color << "m";
  }

  void ConsoleReporter::resetColor() {
    std::cerr << "\033[0m";
  }

  void ConsoleReporter::report(Severity sev, Location loc, StringRef msg) {
    assert(msg.size() > 0 && "Zero-length diagnostic message");

    _messageCountArray[(int)sev] += 1;

    bool colorChanged = false;
    #if TEMPEST_HAVE_UNISTD_H
      if (::isatty(STDERR_FILENO)) {
        if (sev >= ERROR) {
          changeColor(RED, true);
          colorChanged = true;
        } else if (sev == WARNING) {
          changeColor(YELLOW, true);
          colorChanged = true;
        } else if (sev == INFO) {
          changeColor(CYAN, true);
          colorChanged = true;
        } else if (sev == STATUS) {
          changeColor(CYAN, false);
          colorChanged = true;
        }
      }
    #endif

    bool showErrorLine = false;
    if (loc.source != nullptr && !loc.source->path().empty()) {
      std::cerr << loc.source->path() << ":" << loc.startLine << ":" << loc.startCol << ": ";
      showErrorLine = true;
    }

    if (sev != STATUS) {
      std::cerr << severityNames[(int)sev] << ": ";
    }
    writeSpaces(_indentLevel * 2);
    std::cerr << msg << "\n";

    if (colorChanged) {
      resetColor();
    }

    if (showErrorLine) {
      #if TEMPEST_HAVE_UNISTD_H
        if (colorChanged) {
          changeColor(UNCHANGED, true);
        }
      #endif

      std::string line;
      if (loc.source->getLine(loc.startLine - 1, line)) {
        uint32_t beginCol = loc.startCol - 1;
        uint32_t endCol = loc.endCol - 1;
        if (loc.endLine > loc.startLine) {
          endCol = line.size();
        }
        std::cerr << line << "\n";
        #if TEMPEST_HAVE_UNISTD_H
          if (colorChanged) {
            resetColor();
          }
        #endif
        writeSpaces(beginCol);
        while (beginCol < endCol) {
          std::cerr << "^";
          ++beginCol;
        }
        std::cerr << "\n";
      } else if (colorChanged) {
        #if TEMPEST_HAVE_UNISTD_H
          resetColor();
        #endif
      }
    }


    std::cerr.flush();
    if (sev == FATAL) {
  #if TEMPEST_HAVE_CSIGNAL
      printStackTrace(5);
      std::raise(SIGINT);
  #endif
    }
  }

  void ConsoleReporter::printStackTrace(int skipFrames) {
  #if TEMPEST_HAVE_BACKTRACE
    static void* stackTrace[256];

    // Use backtrace() to output a backtrace on Linux systems with glibc.
    int depth = backtrace(stackTrace,
        static_cast<int>(sizeof(stackTrace) / sizeof(stackTrace[0])));

  #if false && TEMPEST_HAVE_DLFCN_H && __GNUG__
    for (int i = skipFrames; i < depth; ++i) {
      Dl_info dlinfo;
      dladdr(stackTrace[i], &dlinfo);
      if (dlinfo.dli_sname != nullptr) {
        ::fputs("   ", stderr);
  #if TEMPEST_HAVE_CXXABI_H
        int status;
        char* d = abi::__cxa_demangle(dlinfo.dli_sname, nullptr, nullptr, &status);
        if (d == nullptr) ::fputs(dlinfo.dli_sname, stderr);
        else           ::fputs(d, stderr);
        ::free(d);
  #else
        ::fputs(dlinfo.dli_sname, stderr);
  #endif

        ::fprintf(stderr, " + %tu",(char*)stackTrace[i]-(char*)dlinfo.dli_saddr);
      }
      ::fputc('\n', stderr);
    }
  #elif TEMPEST_HAVE_CXXABI_H
    if (char** symbols = backtrace_symbols(stackTrace, depth)) {

      // Name buffer used to contain demangling result.
      size_t sz = 256;
      char* buffer = (char *)malloc(sz);

      for (int i = 0; i < depth; ++i) {
        if (i >= skipFrames) {
          char* symbol = symbols[i];
          // TODO: This is a very cheesy way to extract the symbol name,
          // need to come up with something that will work on various platforms.
          // fprintf(outstream, "%s\n", symbol);
          char* begin = strchr(symbol, '_');
          char* demangled_name = nullptr;
          if (begin) {
            char* end = ::strchr(begin, ' ');
            if (end) {
              *end = 0;
              int status;
              demangled_name = abi::__cxa_demangle(begin, buffer, &sz, &status);
            }
          }

          if (demangled_name != nullptr) {
            ::fprintf(stderr, "    %s\n", demangled_name);

            // Result may be a realloc of input buffer.
            buffer = demangled_name;
          } else if (begin != nullptr){
            ::fprintf(stderr, "    %s\n", begin);
          }
        }
      }

      ::free(symbols);
      ::free(buffer);
    }
  #else
    backtrace_symbols_fd(stackTrace, depth, STDERR_FILENO);
  #endif
  #endif
}

}
