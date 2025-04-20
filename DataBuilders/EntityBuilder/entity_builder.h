#pragma once

#include "DependencyInjection/buildtime_services.h"
#include <komihash.h>

namespace DataBuilders {

namespace EntityBuilder {

class EntityBuilder
{
  private:
    Engine::Core::DependencyInjection::BuildtimeServies m_BuildTimeServices;

  public:
    // the constructor needs to initialize cached info: the engine assembly doesn't automatically construct complex data
    // structures from the metadata it has
    EntityBuilder();
};

} // namespace EntityBuilder
} // namespace DataBuilders
