#pragma once

#include <QApplication>

namespace Robomongo
{
    // Widgets need a QApplication; use the headless platform so no window is shown.
    inline void ensureQtTestApplication()
    {
        if (QApplication::instance())
            return;

        qputenv("QT_QPA_PLATFORM", "offscreen");
        static int argc = 1;
        static char arg0[] = "robo_unit_tests";
        static char *argv[] = { arg0, nullptr };
        static QApplication app(argc, argv);
    }
}
