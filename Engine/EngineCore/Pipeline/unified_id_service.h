#pragma once

#include <cstdint>
#include <komihash.h>
#include <string>

namespace Engine {
namespace Core {
namespace Pipeline {

//
class UnifiedIdService
{
  public:
    uint64_t IdString(const char *strInput)
    {
        return komihash(strInput, strlen(strInput), 0);
    }

    uint64_t IdString(const std::string &strInput)
    {
        return IdString(strInput.c_str());
    }
};

} // namespace Pipeline
} // namespace Core
} // namespace Engine
