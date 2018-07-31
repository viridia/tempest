#include "tempest/error/diagnostics.hpp"
#include <iostream>

using namespace tempest::error;

class MockReporter : public Reporter {
public:
  MockReporter() {
    for (int i = 0; i < SEVERITY_LEVELS; ++i) {
      _messageCountArray[i] = 0;
    }
  }

  /** Return what was reported. */
  const std::stringstream& content() const {
    return _reportStrm;
  }
  std::stringstream& content() {
    return _reportStrm;
  }

  /** Reset to initial state. */
  void reset() {
    _reportStrm.str("");
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

  void report(Severity sev, Location loc, StringRef msg) {
    _messageCountArray[(int)sev] += 1;
    _reportStrm << msg << '\n';
  }

  int indentLevel() const { return 0; }
  int setIndentLevel(int level) { return 0; }

  static MockReporter INSTANCE;

private:
  int _messageCountArray[SEVERITY_LEVELS];
  std::stringstream _reportStrm;
};

struct UseMockReporter {
  UseMockReporter() {
    prevReporter = diag.reporter;
    MockReporter::INSTANCE.reset();
    diag.reporter = &MockReporter::INSTANCE;
  }

  ~UseMockReporter() {
    diag.reporter = prevReporter;
  }

  Reporter* prevReporter;
};
