#include "robomongo/core/settings/ExplorerSettings.h"

namespace Robomongo
{
    namespace ExplorerSettings
    {
        const char *const AutoRevealKey = "explorerAutoReveal";

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
    }
}
