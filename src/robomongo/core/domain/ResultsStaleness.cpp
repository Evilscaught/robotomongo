#include "robomongo/core/domain/ResultsStaleness.h"

#include "robomongo/core/domain/MongoNamespace.h"

namespace Robomongo
{
    namespace ResultsStaleness
    {
        bool findResultsOutdated(bool autoRefresh, const MongoNamespace &shown,
                                 const MongoNamespace &written)
        {
            if (autoRefresh || !shown.isValid() || !written.isValid())
                return false;

            return shown.toString() == written.toString();
        }

        bool aggregateResultsOutdated(bool autoRefresh, const std::string &shownCollection,
                                      const MongoNamespace &written)
        {
            if (autoRefresh || shownCollection.empty() || !written.isValid())
                return false;

            return shownCollection == written.collectionName();
        }
    }
}
