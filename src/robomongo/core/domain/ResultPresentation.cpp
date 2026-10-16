#include "robomongo/core/domain/ResultPresentation.h"

#include "robomongo/core/domain/MongoShellResult.h"

namespace Robomongo
{
    namespace ResultPresentation
    {
        bool usesDocumentView(const MongoShellResult &result)
        {
            return !result.documents().empty() || result.queryInfo()._info.isValid();
        }
    }
}
