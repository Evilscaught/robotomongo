#pragma once

#include <QVariantMap>

namespace Robomongo
{
    /**
     * @brief Persistence of explorer preferences.
     *        "explorerAutoReveal" mirrors the VS Code setting explorer.autoReveal: when enabled,
     *        activating a query tab selects its collection in the explorer. Enabled by default.
     */
    namespace ExplorerSettings
    {
        extern const char *const AutoRevealKey;

        bool loadAutoReveal(QVariantMap const& map);
        void saveAutoReveal(QVariantMap &map, bool autoReveal);
    }
}
