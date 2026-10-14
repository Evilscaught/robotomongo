#pragma once

#include <string>

namespace Robomongo
{
    class MongoNamespace;

    /**
     * @brief Decides when query results shown in a result pane no longer match the database
     *        after a write, because auto-refresh did not re-run the query.
     */
    namespace ResultsStaleness
    {
        // Results of a find on `shown` after a successful write to `written`
        bool findResultsOutdated(bool autoRefresh, const MongoNamespace &shown,
                                 const MongoNamespace &written);

        // Results of an aggregate on `shownCollection` after a successful write to `written`.
        // Aggregate results only record the collection name, not the database.
        bool aggregateResultsOutdated(bool autoRefresh, const std::string &shownCollection,
                                      const MongoNamespace &written);
    }
}
