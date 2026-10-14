#include "robomongo/core/settings/ResultsRefreshSettings.h"

namespace Robomongo
{
    namespace ResultsRefreshSettings
    {
        const char *const AutoRefreshKey = "autoRefreshResults";

        bool loadAutoRefresh(QVariantMap const& map)
        {
            if (!map.contains(AutoRefreshKey))
                return true;

            return map.value(AutoRefreshKey).toBool();
        }

        void saveAutoRefresh(QVariantMap &map, bool autoRefresh)
        {
            map.insert(AutoRefreshKey, autoRefresh);
        }
    }
}
