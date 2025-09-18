#define TINYOBJLOADER_IMPLEMENTATION

#include "RendererModule/Assets/mesh.h"

#include <tiny_obj_loader.h>
#include <iostream>
#include <vector>

using namespace Engine::Extension::RendererModule;

static bool LoadObj(const char* inputPath, std::vector<Assets::Vertex>& outputVertices, std::vector<unsigned int>& outputIndices)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials; // TODO: how to load material info?
	std::string warn, err;

	bool success = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputPath);
	
	if (warn.size() > 0)
	{
        std::cerr << "Tiny Obj Loader warning: " << warn << std::endl;
	}
	
	if (err.size() > 0)
	{
        std::cerr << "Tiny Obj Loader warning: " << err << std::endl;
	}

	if (!success)
	{
        std::cerr << "Obj file failed to load!" << std::endl;
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

enum class ErrorCodes
{
	FailLoadObj = 1,
	EmptyOutput,
	BadCommandLine,

};

int main(int argc, char *argv[])
{
	std::vector<Assets::Vertex> outputVertices;
	std::vector<unsigned int> outputIndices;

	if (argc != 2)
	{
		std::cerr << "Error: bad argument." << std::endl;
		return (int)ErrorCodes::BadCommandLine;
	}
	
	// read in vertices and indices
	if (!LoadObj(argv[1], outputVertices, outputIndices))
	{
		std::cerr << "Error: failed to load obj data." << std::endl;
		return (int)ErrorCodes::FailLoadObj;
	}

	// write them out	
	unsigned int vertexCount = (unsigned int)outputVertices.size();
	unsigned int indexCount = (unsigned int)outputIndices.size();

	// TODO: temp solution, sanity check output by checking the output file size
	if (vertexCount == 0 || indexCount == 0) 
	{
		std::cerr << "Error: obj data resutls in empty output." << std::endl;
		return (int)ErrorCodes::EmptyOutput;
	}

	std::cout
		.write((const char*)&vertexCount, sizeof(vertexCount))
		.write((const char*)(outputVertices.data()), outputVertices.size() * sizeof(Assets::Vertex))
		.write((const char*)&indexCount, sizeof(indexCount))
		.write((const char*)(outputIndices.data()), outputIndices.size() * sizeof(int));
}