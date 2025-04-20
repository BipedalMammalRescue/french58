#include "string_hashing.h"

#include <cstring>
#include <komihash.h>
using namespace Engine::Core::Utils;

uint64_t Engine::Core::Utils::StringHash64(const char *string, size_t length)
{
    return komihash(string, length, 0);
}

uint64_t Engine::Core::Utils::StringHash64(const char *string)
{
    return StringHash64(string, strlen(string));
}
