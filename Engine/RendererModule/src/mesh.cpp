#include "RendererModule/Assets/mesh.h"

#include <EngineUtils/ErrorHandling/exceptions.h>
#include <EngineUtils/Memory/in_place_read_stream.h>
#include <memory>
#include <tiny_obj_loader.h>


using namespace Engine;
using namespace Extension::RendererModule;

static const char s_ChannelName[] = "RendererModule::Mesh";

static bool LoadObj(const char* inPath, std::vector<Assets::Vertex>& outputVertices, std::vector<int>& outputIndices)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials; // TODO: add in material later
	std::string warn, err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inPath);
	
	if (warn.size() > 0)
	{
		Core::Logging::GetLogger()->Warning(s_ChannelName, "Tiny Obj Loader warning: %s", warn.c_str());
	}
	
	if (err.size() > 0)
	{
		Core::Logging::GetLogger()->Error(s_ChannelName, "Tiny Obj Loader error: %s", err.c_str());
	}

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inPath))
	{
		Core::Logging::GetLogger()->Error(s_ChannelName, "Obj file failed to load!");
		return false;
	}

	// deduplication
	std::string rollingKeyBuffer;
	rollingKeyBuffer.reserve(64);
	std::unordered_map<std::string, unsigned int> vertexMap;

	for (const tinyobj::shape_t& shape : shapes)
	{
		// load each index of this face into the buffer
		for (const tinyobj::index_t& index : shape.mesh.indices)
		{
			// attempt deduplication
			{
				rollingKeyBuffer.clear();
				rollingKeyBuffer.append(std::to_string(index.vertex_index));
				rollingKeyBuffer.append(std::to_string(index.normal_index));
				rollingKeyBuffer.append(std::to_string(index.texcoord_index));

				auto foundIndex = vertexMap.find(rollingKeyBuffer);
				if (foundIndex != vertexMap.end())
				{
					outputIndices.push_back(foundIndex->second);
					continue;
				}
				else
				{
					vertexMap[rollingKeyBuffer] = (unsigned int)outputVertices.size();
				}
			}

		    Assets::Vertex vertex;

			if (index.vertex_index >= 0)
			{
				vertex.position = {
					attrib.vertices[3 * index.vertex_index + 0],
					attrib.vertices[3 * index.vertex_index + 1],
					attrib.vertices[3 * index.vertex_index + 2]
				};
			}

			if (index.normal_index >= 0)
			{
				vertex.normal = {
					attrib.normals[3 * index.normal_index +0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};
			}

			if (index.texcoord_index >= 0)
			{
				vertex.uv = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					attrib.texcoords[2 * index.texcoord_index + 1],
				};
			}

			outputVertices.push_back(vertex);
			outputIndices.push_back((unsigned int)outputVertices.size() - 1);
		}
	}

	return true;
}

Core::Pipeline::AssetDefinition Assets::Mesh::GetDefinition() 
{
    static const char name[] = "Engine::Extension::RendererModule::Assets::Mesh";
    static const char propNamePath[] = "Path";
    
    static const Core::Pipeline::Scripting::NamedProperty properties[] = 
    {
        {propNamePath, Core::Pipeline::Scripting::DataType::PATH}
    };
    
    static const Core::Pipeline::AssetDefinition def = 
    {
        name,
        properties,
        sizeof(properties) / sizeof(Core::Pipeline::Scripting::NamedProperty)
    };
    
    return def;
}

bool Assets::Mesh::Build(const Core::Pipeline::Scripting::Variant *fieldv, size_t fieldc, 
                         Core::DependencyInjection::BuildtimeServies *services, std::ostream &output) 
{
    const char* filePath = services->GetFileAccessService()->GetPath(fieldv[0].Data.Path);
    
    std::vector<Assets::Vertex> outputVertices;
    std::vector<int> outputIndices;
    bool success = LoadObj(filePath, outputVertices, outputIndices);
    
    if (!success)
        return false;
    
    unsigned int vertexCount = (unsigned int)outputVertices.size();
	unsigned int indexCount = (unsigned int)outputIndices.size();
    output
		.write((const char*)&vertexCount, sizeof(vertexCount))
		.write((const char*)(outputVertices.data()), outputVertices.size() * sizeof(RendererModule::Assets::Vertex))
		.write((const char*)&indexCount, sizeof(indexCount))
		.write((const char*)(outputIndices.data()), outputIndices.size() * sizeof(int));
		
	return true;
}

size_t Assets::Mesh::MaxLoadSize(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                 Core::DependencyInjection::RuntimeServices *services) 
{
    return sizeof(Core::Rendering::RendererMesh);
}

Core::AssetManagement::LoadedAsset Assets::Mesh::Load(const unsigned char *inputDataV, const size_t inputDataC, const uint64_t id,
                                                      Core::DependencyInjection::RuntimeServices *services)
{
    Utils::Memory::InPlaceReadStream stream(inputDataV, inputDataC);

    // load all vertices
    unsigned int vertexCount = stream.ReadCopy<unsigned int>();
    Vertex* allVertices = stream.ReadInPlace<Vertex>(vertexCount);

    // load all indices
    unsigned int indexCount = stream.ReadCopy<unsigned int>();
    unsigned int* allIndices = stream.ReadInPlace<unsigned int>(indexCount);

    // submit everything to server
    Core::Rendering::RendererMesh newMesh;
    if (!services->GetRenderer()->RegisterMesh({allVertices, (unsigned int)sizeof(Vertex) * vertexCount},
                                               {allIndices, indexCount}, newMesh))
    {
        SE_THROW_GRAPHICS_EXCEPTION;
    }
    
    // allocate and return
    Core::AssetManagement::LoadedAsset newAsset = services->GetAssetManager()->CreateAsset(sizeof(newMesh), id);
    *((Core::Rendering::RendererMesh*)newAsset.Buffer) = newMesh;

    return newAsset;
}

void Assets::Mesh::Dispose(Core::AssetManagement::LoadedAsset asset, const uint64_t id,
                        Core::DependencyInjection::RuntimeServices *services)
{
    Core::Rendering::RendererMesh* mesh = (Core::Rendering::RendererMesh*)asset.Buffer;
    services->GetRenderer()->DeleteMesh(*mesh);
}