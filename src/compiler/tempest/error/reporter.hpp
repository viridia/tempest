#ifndef TEMPEST_ERROR_REPORTER_HPP
#define TEMPEST_ERROR_REPORTER_HPP 1

#ifndef TEMPEST_SOURCE_LOCATION_HPP
  #include "tempest/source/location.hpp"
#endif

#ifndef LLVM_ADT_STRINGREF_H
  #include <llvm/ADT/StringRef.h>
#endif

#include <sstream>

namespace tempest::error {
  using tempest::source::Location;
  using llvm::StringRef;

  class Reporter;

  enum Severity {
    DEBUG = 0,
    STATUS,
    INFO,
    WARNING,
    ERROR,
    FATAL,
    OFF,

    SEVERITY_LEVELS,
  };

  /** Stream class which prints "Assertion failed" and aborts. */
  class MessageStream : public std::stringstream {
  public:
    MessageStream(Reporter* reporter, Severity severity, Location location)
      : _reporter(reporter)
      , _severity(severity)
      , _location(location)
    {}

    MessageStream(const MessageStream& src)
      : _reporter(src._reporter)
      , _severity(src._severity)
      , _location(src._location)
    {}

    /** Destructor will flush the string to the reporter. */
    ~MessageStream();

    MessageStream &operator=(const MessageStream& src) {
      _reporter = src._reporter;
      _severity = src._severity;
      _location = src._location;
      return *this;
    }

  private:
    Reporter* _reporter;
    Severity _severity;
    Location _location;
  };

  /** Reporter interface. */
  class Reporter {
  public:
    /** Write a message to the output stream. */
    virtual void report(Severity sev, Location loc, StringRef msg) = 0;

    /** Number of errors encountered so far. */
    virtual int errorCount() const { return 0; }

    /** Increase the indentation level. */
    void indent() {
      setIndentLevel(indentLevel() + 1);
    }

    /** Decrease the indentation level. */
    void unindent() {
      setIndentLevel(indentLevel() - 1);
    }

    /** Get the current indent level. */
    virtual int indentLevel() const = 0;

    /** Set the current indentation level. Returns the previous indent level. */
    virtual int setIndentLevel(int level) = 0;
  };

  /** Implements the indentation functionality. */
  class IndentingReporter : public Reporter {
  public:
    IndentingReporter() : _indentLevel(0) {}

    int indentLevel() const { return _indentLevel; }
    int setIndentLevel(int level) {
      int prevLevel = _indentLevel;
      _indentLevel = level;
      return prevLevel;
    }

  protected:
    int _indentLevel;
  };

  /** Reporter that prints to stdout / stderr. */
  class ConsoleReporter : public IndentingReporter {
  public:
    ConsoleReporter() {
      for (int i = 0; i < SEVERITY_LEVELS; ++i) {
        _messageCountArray[i] = 0;
      }
    }

    /** Get message count by severity. */
    int messageCount(Severity sev) const {
      return _messageCountArray[sev];
    }

    /** Total number of error messages. */
    int errorCount() const {
      return messageCount(ERROR) + messageCount(FATAL);
    }

    void report(Severity sev, Location loc, StringRef msg);

    /** Print a stack backtrace if possible. */
    static void printStackTrace(int skipFrames);

    static ConsoleReporter INSTANCE;

  private:
    int _messageCountArray[SEVERITY_LEVELS];
  //   RecoveryState _recovery;

    void writeSpaces(unsigned numSpaces);
    void changeColor(StringRef color, bool bold);
    void resetColor();
  };

  // How to print severity.
  inline ::std::ostream& operator<<(::std::ostream& os, Severity sev) {
    switch (sev) {
      case DEBUG: os << "DEBUG"; break;
      case STATUS: os << "STATUS"; break;
      case INFO: os << "INFO"; break;
      case WARNING: os << "WARNING"; break;
      case ERROR: os << "ERROR"; break;
      case FATAL: os << "FATAL"; break;
      case OFF: os << "OFF"; break;
      default:
        assert(false && "Invalid severity.");
    }
    return os;
  }
}

#endif
