#pragma once

#include <ostream>

#include <QString>

// Lets Google Test print QString values readably in failure messages
inline void PrintTo(const QString &value, std::ostream *os)
{
    *os << '"' << value.toStdString() << '"';
}
