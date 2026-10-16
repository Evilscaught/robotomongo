# Unit tests

## Building and running
- Main tests:  cmake --build <build> --target robo_unit_tests && <build>/src/robomongo-unit-tests/robo_unit_tests
- Isolated tests (anything touching AppRegistry/SettingsManager):
  cmake --build <build> --target run_isolated_tests
- Run the FULL suite before committing; some failures only show up depending on test order.

## Where tests go
- Next to the code: Foo.cpp -> Foo_test.cpp, registered in SOURCES_TEST in CMakeLists.txt.
- Code that uses AppRegistry, SettingsManager, GuiRegistry::font() or PagingWidget:
  Foo_isolated_test.cpp in SOURCES_ISOLATED_TEST.
- Tests that need a server: Foo_integration_test.cpp using TestMongoServer.h
  (starts its own mongod; skips itself if mongod isn't installed).

## Rules
- Never touch the real ~/.3T. Config paths are fixed from $HOME at program start,
  so only robo_isolated_tests may construct SettingsManager/AppRegistry.
- Don't call accept()/reject()/close() on dialogs: they write window sizes to the
  real macOS preferences (QSettings ignores $HOME).
- Avoid inputs that open modal message boxes (e.g. invalid JSON in editors); they hang the run.
- Tests must not depend on the process locale; QApplication changes it.
- Widgets need ensureQtTestApplication() (QtTestApplication.h).
- Only one SettingsManager may be alive at a time (its connection list is static).
- Never kill processes by name; integration servers clean themselves up.

## Bugs
- Tests that expose a bug go in Foo<Bug>_bug_test.cpp, are confirmed failing,
  and are stashed (git stash push -u -m "BUG: ...") rather than committed.
  The user reviews and fixes them one by one.

## Coverage
- Instrumented build in build/coverage (-fprofile-instr-generate -fcoverage-mapping);
  merge with xcrun llvm-profdata, report with xcrun llvm-cov report.

# Where things stand on chore/test-coverage:
- Tests: about 500 in total (446 in the main executable, 50 in the isolated one), up from 8.
- Commits: about 40 test suites, plus infrastructure commits for:
  - the isolated executable that runs with HOME pointed at a scratch folder,
  - a throwaway mongod fixture with a crash-proof watchdog,
  - the legacy MongoDB runtime setup.
- Integration tests against a real MongoDB 7.0: both the new driver and the legacy client paths.
- Measured coverage: 24% of lines before the integration suites, up from about 1%. I'll re-measure at the end.

# 26 bugs stashed, each with failing tests. The most serious:
1. Rejected writes are reported as success on the legacy path (duplicate _id, validator violations). That path handles SSH, SSL and replica-set connections, on MongoDB 5.1+.
2. Two crashes on MongoDB 6.0+: any find with a projection, and creating a text index with weights. Both read freed memory.
3. Editing a document changes data: double precision is lost, and the regex s flag is dropped.
4. MongoDB 6.0+ changes field types: whole doubles become int32, and small int64 values are read as int32.
5. Create Database does nothing on WiredTiger, and an index TTL of 0 is silently dropped.
6. An unparseable config wipes all saved connections, and the config and key files are readable by other users.
