#pragma once

#include <stdexcept>

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define AT __FILE__ ":" TOSTRING(__LINE__)

// now if you think this is not exactly a good way to report error, I agree, I just don't want to spend too much time up
// front, as long as it crashes the system it's fine
#define SE_THROW_GRAPHICS_EXCEPTION throw std::runtime_error(AT " Graphics error: see previous logs for details!")

#define SE_THROW_ASSET_NOT_LOADED throw std::runtime_error(AT " Request asset isn't loaded!")