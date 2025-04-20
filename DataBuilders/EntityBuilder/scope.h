#pragma once

#include <cstdint>

namespace DataBuilders {
namespace EntityBuilder {

class IScope
{
  public:
    virtual void ProcessLine() = 0;
    virtual uint64_t GetID() = 0;
};

} // namespace EntityBuilder
} // namespace DataBuilders
