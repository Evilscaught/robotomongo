#include "robomongo/core/settings/ExplorerSettings.h"

namespace Robomongo
{
    namespace ExplorerSettings
    {
        const char *const AutoRevealKey = "explorerAutoReveal";
        const char *const ShowCollectionSizesKey = "explorerShowCollectionSizes";

        bool loadAutoReveal(QVariantMap const& map)
        {
            if (!map.contains(AutoRevealKey))
                return true;

            return map.value(AutoRevealKey).toBool();
        }

        void saveAutoReveal(QVariantMap &map, bool autoReveal)
        {
            map.insert(AutoRevealKey, autoReveal);
        }

        bool loadShowCollectionSizes(QVariantMap const& map)
        {
            return map.value(ShowCollectionSizesKey, false).toBool();
        }

        void saveShowCollectionSizes(QVariantMap &map, bool showSizes)
        {
            map.insert(ShowCollectionSizesKey, showSizes);
        }
    }
}
