#pragma once

// One-time initialisation of the embedded MongoDB 4.2 client, mirroring app/main.cpp, so tests can
// open mongo::DBClientConnection connections (the legacy driver path).

#include <mongo/platform/basic.h>

#include <mongo/base/initializer.h>
#include <mongo/db/service_context.h>
#include <mongo/transport/transport_layer_asio.h>
#include <mongo/util/assert_util.h>

namespace Robomongo
{
    inline void ensureLegacyMongoRuntime()
    {
        static bool const initialised = []() {
            static char arg0[] = "robo_unit_tests";
            static char *argv[] = { arg0, nullptr };
            static char *envp[] = { nullptr };
            mongo::runGlobalInitializersOrDie(1, argv, envp);
            mongo::setGlobalServiceContext(mongo::ServiceContext::make());

            mongo::transport::TransportLayerASIO::Options options;
            options.enableIPv6 = false;
            options.mode = mongo::transport::TransportLayerASIO::Options::kEgress;
            auto serviceContext = mongo::getGlobalServiceContext();
            serviceContext->setTransportLayer(
                std::make_unique<mongo::transport::TransportLayerASIO>(options, nullptr));
            uassertStatusOK(serviceContext->getTransportLayer()->setup());
            uassertStatusOK(serviceContext->getTransportLayer()->start());
            return true;
        }();
        (void)initialised;
    }
}
