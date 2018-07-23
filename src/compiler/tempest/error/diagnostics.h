#ifndef TEMPEST_ERROR_DIAGNOSTICS_H
#define TEMPEST_ERROR_DIAGNOSTICS_H 1

#ifndef TEMPEST_ERROR_REPORTER_H
  #include "tempest/error/reporter.h"
#endif

namespace tempest::error {
  using tempest::source::Locatable;

  struct Diagnostics {
    Reporter* reporter = &ConsoleReporter::INSTANCE;

    /** Fatal error. */
    inline MessageStream fatal(Location loc = Location()) {
      return MessageStream(reporter, FATAL, loc);
    }

    /** Error. */
    inline MessageStream error(Location loc = Location()) {
      return MessageStream(reporter, ERROR, loc);
    }

    inline MessageStream error(const Locatable *loc) {
      return MessageStream(reporter, ERROR, loc->getLocation());
    }

    /** Warning message. */
    inline MessageStream warn(Location loc = Location()) {
      return MessageStream(reporter, WARNING, loc);
    }

    inline MessageStream warn(const Locatable *loc) {
      return MessageStream(reporter, WARNING, loc->getLocation());
    }

    /** Info message. */
    inline MessageStream info(Location loc = Location()) {
      return MessageStream(reporter, INFO, loc);
    }

    /** Status message. */
    inline MessageStream status(Location loc = Location()) {
      return MessageStream(reporter, STATUS, loc);
    }

    /** Debugging message. */
    inline MessageStream debug(Location loc = Location()) {
      return MessageStream(reporter, DEBUG, loc);
    }

    /** Message with variable severity. */
    inline MessageStream operator()(Severity sev, Location loc = Location()) {
      return MessageStream(reporter, sev, loc);
    }

    /** Number of errors encountered so far. */
    int errorCount() const { return reporter->errorCount(); }

    /** Increase the indentation level. */
    void indent() {
      reporter->indent();
    }

    /** Decrease the indentation level. */
    void unindent() {
      reporter->unindent();
    }

    /** Get the current indent level. */
    int indentLevel() const {
      return reporter->indentLevel();
    }

    /** Set the current indentation level. Returns the previous indent level. */
    int setIndentLevel(int level) {
      return reporter->setIndentLevel(level);
    }
  };

  // Static diagnostics instance.
  static Diagnostics diag;

  /** Convenience class that increases indentation level within a scope. */
  class AutoIndent {
  public:
    AutoIndent(Diagnostics& diag, bool enabled = true)
      : _diag(diag)
      , _enabled(enabled)
    {
      if (_enabled) _diag.indent();
    }

    ~AutoIndent() {
      if (_enabled) _diag.unindent();
    }

  private:
    Diagnostics& _diag;
    bool _enabled;
  };
}

#endif