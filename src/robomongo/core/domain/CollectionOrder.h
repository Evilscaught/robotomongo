#pragma once

#include <string>
#include <vector>

namespace Robomongo
{
    namespace CollectionOrder
    {
        /**
         * @brief Order in which collections are listed in the explorer: byte-wise ascending,
         *        so uppercase names come before lowercase ones (Robo 3T's original order).
         *        Shared by the legacy and modern driver paths so both list collections alike.
         */
        void sortNames(std::vector<std::string> &names);
    }
}
