#include "robomongo/core/domain/TabCollection.h"

#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    namespace TabCollection
    {
        MongoNamespace fromResults(const std::vector<MongoShellResult> &results,
                                   const std::string &currentDatabase)
        {
            for (auto const& result : results) {
                if (result.queryInfo()._info.isValid())
                    return result.queryInfo()._info._ns;
            }

            if (currentDatabase.empty())
                return MongoNamespace();

            for (auto const& result : results) {
                AggrInfo const& aggrInfo = result.aggrInfo();
                if (aggrInfo.isValid && !aggrInfo.collectionName.empty())
                    return MongoNamespace(currentDatabase, aggrInfo.collectionName);
            }

            return MongoNamespace();
        }
    }
}
