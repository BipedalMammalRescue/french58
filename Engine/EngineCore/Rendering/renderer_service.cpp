#include "renderer_service.h"
#include "ErrorHandling/exceptions.h"

#include <SDL3/SDL.h>
#include <SDL3/SDL_opengl.h>
#include <stdio.h>
#include <string>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat2x2.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <SDL3/SDL_gpu.h>


using namespace Engine;
using namespace Engine::Core;


// TODO: how the fuck do I handle errors from SDL?


static const char s_ChannelName[] = "RendererService";


bool Engine::Core::Rendering::RendererService::CompileShader(const std::string &code, ShaderType type, unsigned int numSamplers, unsigned int numUniformBuffers, unsigned int numStorageBuffers, unsigned int numStorageTextures, RendererShader &outID)
{
	SDL_GPUShaderStage shaderStage;
	switch (type) 
	{
	case ShaderType::FRAGMENT_SHADER:
		shaderStage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_FRAGMENT;
		break;
	case ShaderType::VERTEX_SHADER:
		shaderStage = SDL_GPUShaderStage::SDL_GPU_SHADERSTAGE_VERTEX;
		break;
	}

	SDL_GPUShaderCreateInfo shaderInfo = {
		code.size(),
		(unsigned char*)code.data(),
		"main",
		SDL_GPU_SHADERFORMAT_SPIRV,
		shaderStage,
		numSamplers,
		numStorageTextures,
		numStorageBuffers,
		numUniformBuffers
	};

	SDL_GPUShader* newShader = SDL_CreateGPUShader(m_Platform->m_GpuDevice, &shaderInfo);
	if (newShader == nullptr) 
	{
		return false;
	}

	outID.m_ShaderID = newShader;
    return true;
}


bool Engine::Core::Rendering::RendererService::DeleteShader(RendererShader &shader)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::CreateMaterial(const RendererShader& vertexShader, const RendererShader& fragmentShader, RendererMaterial& outID)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::DeleteMaterial(RendererMaterial& material)
{
	return false;
}


bool Engine::Core::Rendering::RendererService::RegisterMesh(
	const VertexCollection& vertices, 
	const VertexLayout& vertexLayout, 
	const IndexCollection& indices, 
	RendererMesh& outID)
{
	// create buffers for the upload operation
	SDL_GPUBufferCreateInfo vertBufferCreateInfo 
	{
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vertices.Size
	};

	SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(
		m_Platform->m_GpuDevice,
		&vertBufferCreateInfo
	);

	SDL_GPUBufferCreateInfo indexBufferCreateInfo
	{
		.usage = SDL_GPU_BUFFERUSAGE_INDEX,
		.size = indices.GetSize()
	};

	SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(
		m_Platform->m_GpuDevice,
		&indexBufferCreateInfo
	);

	SDL_GPUTransferBufferCreateInfo transBufferCreateInfo 
	{
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vertBufferCreateInfo.size + indexBufferCreateInfo.size
	};

	SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
		m_Platform->m_GpuDevice,
		&transBufferCreateInfo
	);

	// upload the data
	void* mapping = SDL_MapGPUTransferBuffer(
		m_Platform->m_GpuDevice,
		transferBuffer,
		false
	);
	memcpy(mapping, vertices.Vertices, vertices.Size);
	memcpy((char*)mapping + vertices.Size, indices.Indices, indices.GetSize());
	SDL_UnmapGPUTransferBuffer(m_Platform->m_GpuDevice, transferBuffer);

	SDL_GPUCommandBuffer* uploadCmdBuffer = SDL_AcquireGPUCommandBuffer(m_Platform->m_GpuDevice);
	SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCmdBuffer);
	
	SDL_GPUTransferBufferLocation transBufLocation 
	{
		.transfer_buffer = transferBuffer,
		.offset = 0
	};

	// upload vertex buffer
	SDL_GPUBufferRegion vertexBufferRegion
	{
		.buffer = vertexBuffer,
		.offset = 0,
		.size = vertices.Size
	};

	SDL_UploadToGPUBuffer(
		copyPass,
		&transBufLocation,
		&vertexBufferRegion,
		false
	);

	transBufLocation.offset += vertexBufferRegion.size;

	// upload index buffer
	SDL_GPUBufferRegion indexBufferRegion 
	{
		.buffer = indexBuffer,
		.offset = 0,
		.size = indices.GetSize()
	};

	SDL_UploadToGPUBuffer(
		copyPass,
		&transBufLocation,
		&indexBufferRegion,
		false
	);

	// clean up
	SDL_EndGPUCopyPass(copyPass);
	SDL_SubmitGPUCommandBuffer(uploadCmdBuffer);
	SDL_ReleaseGPUTransferBuffer(m_Platform->m_GpuDevice, transferBuffer);

	return true;
}


bool Engine::Core::Rendering::RendererService::DeleteMesh(RendererMesh& inID)
{
	SDL_ReleaseGPUBuffer(m_Platform->m_GpuDevice, inID.m_IndexBuffer);
	SDL_ReleaseGPUBuffer(m_Platform->m_GpuDevice, inID.m_VertexBuffer);
	return true;
}


// TODO: what do we do about this?
bool Rendering::RendererService::ApplyDynamicShaderParameter(const Rendering::DynamicShaderParameter* shaderParam, Rendering::RendererMaterial* material)
{
	return false;
}


bool Rendering::RendererService::QueueRender(RendererMesh* mesh, RendererMaterial* material, const Rendering::DynamicShaderParameter* shaderParams, unsigned int shaderParamCount)
{
	return false;
}
