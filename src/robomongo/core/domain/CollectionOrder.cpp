#include "robomongo/core/domain/CollectionOrder.h"

#include <algorithm>

namespace Robomongo
{
    namespace CollectionOrder
    {
        void sortNames(std::vector<std::string> &names)
        {
            std::sort(names.begin(), names.end());
        }
    }
}
