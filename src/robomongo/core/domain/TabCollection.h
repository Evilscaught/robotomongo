#pragma once

#include <string>
#include <vector>

#include "robomongo/core/domain/MongoNamespace.h"

namespace Robomongo
{
    class MongoShellResult;

    namespace TabCollection
    {
        /**
         * @brief Collection a query tab is showing: the namespace of its first find result,
         *        otherwise the collection of its first aggregate result in `currentDatabase`.
         *        Returns an invalid namespace when no result belongs to a collection.
         */
        MongoNamespace fromResults(const std::vector<MongoShellResult> &results,
                                   const std::string &currentDatabase);
    }
}
