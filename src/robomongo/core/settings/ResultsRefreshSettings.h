#pragma once

#include <QVariantMap>

namespace Robomongo
{
    /**
     * @brief Persistence of the "auto-refresh results after writes" preference.
     *        Auto-refresh is enabled unless the config explicitly disables it.
     */
    namespace ResultsRefreshSettings
    {
        extern const char *const AutoRefreshKey;

        bool loadAutoRefresh(QVariantMap const& map);
        void saveAutoRefresh(QVariantMap &map, bool autoRefresh);
    }
}
