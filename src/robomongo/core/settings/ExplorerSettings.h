#pragma once

#include <QVariantMap>

namespace Robomongo
{
    /**
     * @brief Persistence of explorer preferences.
     *        "explorerAutoReveal" mirrors the VS Code setting explorer.autoReveal: when enabled,
     *        activating a query tab selects its collection in the explorer. Enabled by default.
     *        "explorerShowCollectionSizes" shows storage sizes next to databases, collections and
     *        indexes. Disabled by default: fetching sizes runs dbStats plus collStats per collection.
     */
    namespace ExplorerSettings
    {
        extern const char *const AutoRevealKey;
        extern const char *const ShowCollectionSizesKey;

        bool loadAutoReveal(QVariantMap const& map);
        void saveAutoReveal(QVariantMap &map, bool autoReveal);

        bool loadShowCollectionSizes(QVariantMap const& map);
        void saveShowCollectionSizes(QVariantMap &map, bool showSizes);
    }
}
