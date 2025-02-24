#include "byte_stream.h"
#include <ios>

using namespace Engine::Core::AssetManagement;

Engine::Core::AssetManagement::InFileStream::InFileStream(std::ifstream& backend)
	: m_Backend(backend)
{
	m_CanRead = true;
	m_CanCount = true;

	// calculate file length
	std::streampos begin,end;
	begin = backend.tellg();
	backend.seekg(0, std::ios::end);
	end = backend.tellg();
	m_Count = end - begin;
	backend.seekg(0, std::ios::beg);
}

long long InFileStream::Read(char* buffer, long long count)
{
	m_Backend.read(buffer, count);
	return m_Backend.gcount();
}

long long InFileStream::Count() const 
{
	return m_Count;
}

OutFileStream::OutFileStream(std::ofstream& backend)
	: m_Backend(backend)
{
}

void OutFileStream::Write(char* buffer, long long count)
{
	m_Backend.write(buffer, count);
}

bool ByteStream::FillBuffer(void* dest, size_t length)
{
	char* destBuffer = (char*)dest;

	size_t totalReads = 0;
	size_t newReads = 0;

	while (totalReads < length && (newReads = Read(destBuffer + totalReads, length)) > 0)
	{
		totalReads += newReads;
	}

	return totalReads == length;
}
