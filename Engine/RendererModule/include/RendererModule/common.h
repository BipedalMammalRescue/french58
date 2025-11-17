#pragma once

namespace Engine::Extension::RendererModule {

template <typename THeader>
static void* SkipHeader(THeader* headerAddress)
{
    auto rawAddress = (char*)headerAddress;
    return rawAddress + sizeof(THeader);
}
    
}