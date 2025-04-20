#pragma once

#include "Pipeline/unified_id_service.h"

namespace Engine {
namespace Core {
namespace DependencyInjection {

class BuildtimeServies
{
  private:
    Pipeline::UnifiedIdService m_UnifiedIdService;

  public:
    inline Pipeline::UnifiedIdService *GetUnifiedIdService()
    {
        return &m_UnifiedIdService;
    }
};

} // namespace DependencyInjection
} // namespace Core
} // namespace Engine
