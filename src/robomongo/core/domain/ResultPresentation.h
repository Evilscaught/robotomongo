#pragma once

namespace Robomongo
{
    class MongoShellResult;

    namespace ResultPresentation
    {
        /**
         * @brief Whether a shell result is shown in the document view (results header with
         *        paging, export and refresh controls) rather than as plain output.
         *        A find on a collection uses the document view even when it returned no
         *        documents, so an empty collection still offers "Insert Document".
         */
        bool usesDocumentView(const MongoShellResult &result);
    }
}
