#include "mesh.h"
#include "pch.h"

#include <memory>

using namespace Engine;
using namespace Extension::RendererModule;

void *Assets::Mesh::LoadMesh(Core::DependencyInjection::RuntimeServices *services,
                             Core::AssetManagement::ByteStream *source)
{
    // load all vertices
    unsigned int vertexCount = 0;
    if (!source->FillStruct(&vertexCount))
        return nullptr;
    std::unique_ptr<Vertex[]> allVertices(new Vertex[vertexCount]);
    if (!source->FillArray(allVertices.get(), vertexCount))
        return nullptr;

    // load all indices
    unsigned int indexCount = 0;
    if (!source->FillStruct(&indexCount))
        return nullptr;
    std::unique_ptr<unsigned int[]> allIndices(new unsigned int[indexCount]);
    if (!source->FillArray(allIndices.get(), indexCount))
        return nullptr;

    Mesh *newMesh = services->GetGlobalAllocator()->New<Mesh>();

    // submit everything to server
    if (!services->GetRenderer()->RegisterMesh({allVertices.get(), (unsigned int)sizeof(Vertex) * vertexCount},
                                               {allIndices.get(), indexCount}, newMesh->m_RendererCopy))
    {
        services->GetGlobalAllocator()->Free(newMesh);
        return nullptr;
    }

    return newMesh;
}

void Assets::Mesh::DisposeMesh(Core::DependencyInjection::RuntimeServices *services, void *data)
{
    Mesh *mesh = (Mesh *)data;
    services->GetRenderer()->DeleteMesh(mesh->m_RendererCopy);
    services->GetGlobalAllocator()->Free(mesh);
}

SE_REFLECTION_BEGIN(Extension::RendererModule::Assets::Mesh)
    .SE_REFLECTION_OVERRIDE_DESERIALIZER(Extension::RendererModule::Assets::Mesh::LoadMesh)
    .SE_REFLECTION_OVERRIDE_DISPOSER(Extension::RendererModule::Assets::Mesh::DisposeMesh)
    .SE_REFLECTION_DELETE_SERIALIZER()
    .SE_REFLECTION_END
