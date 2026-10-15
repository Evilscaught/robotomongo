// Entry point for robo_isolated_tests: tests of code that uses AppRegistry (and therefore
// SettingsManager, which reads and writes the config under ~/.3T and the key file).
//
// The config paths are computed from $HOME when the program starts, so this executable must be
// launched with HOME pointing at a throwaway directory. Use the run_isolated_tests target, which
// recreates <build>/robo-test-home and sets HOME and ROBO_TEST_ISOLATED_HOME to it.

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <QApplication>
#include <QDir>

#include <gtest/gtest.h>

int main(int argc, char **argv)
{
    const char *home = std::getenv("HOME");
    const char *isolatedHome = std::getenv("ROBO_TEST_ISOLATED_HOME");

    bool const isolated = home && isolatedHome && std::strcmp(home, isolatedHome) == 0 &&
                          std::strstr(home, "robo-test-home") != nullptr &&
                          QDir::homePath() == QDir(isolatedHome).absolutePath();

    if (!isolated) {
        std::fprintf(stderr,
            "robo_isolated_tests must run with HOME set to a scratch directory so it cannot touch\n"
            "your real Robo 3T config. Run it with:\n"
            "    cmake --build <build dir> --target run_isolated_tests\n");
        return 2;
    }

    // Owned by main() so it is destroyed before Qt's plugins are unloaded at exit (a static
    // QApplication crashes during exit once widgets have created an offscreen GL context)
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
