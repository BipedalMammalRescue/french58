#pragma once

#include <cstdint>
#include <string>
#include <istream>
#include <unordered_map>

namespace Engine::Core::Pipeline {

// provides *build-time* file access (mostly here to allow some file organization tricks later)
class FileAccessService
{
  private:
    std::unordered_map<uint64_t, std::string> m_IndexCache;

  public:
    void Index(const char *rootPath);

    // returns null if the identifier is not found
    const char* GetPath(uint64_t identifier) const;
};

} // namespace Engine::Core::Pipeline