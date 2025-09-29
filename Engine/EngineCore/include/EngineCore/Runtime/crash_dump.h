#pragma once

#include <optional>
#include <string>

namespace Engine::Core::Runtime {

struct CrashDump
{
    // use std::string since this thing is the last behavior of the engine it's not going to cause any more performance issues
    std::string File;
    std::string ErrorDetail;
    int Line;
};

typedef std::optional<CrashDump> CallbackResult;

inline CallbackResult CallbackSuccess()
{
    return CallbackResult();
}

inline CallbackResult Crash(const std::string& file, int line, const std::string& errorDetail)
{
    return CallbackResult({file, errorDetail, line});
}

inline CallbackResult Crash(const std::string&& file, int line, const std::string&& errorDetail)
{
    return CallbackResult({file, errorDetail, line});
}

}