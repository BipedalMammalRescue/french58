#pragma once

#include <Pipeline/file_access_service.h>

namespace Engine::Core::DependencyInjection {

class BuildtimeServies
{
  private:
    Pipeline::FileAccessService m_FileAccessService;

  public:
    inline Pipeline::FileAccessService *GetFileAccessService()
    {
        return &m_FileAccessService;
    }
};

} // namespace Engine::Core::DependencyInjection