#include "EngineCore/AssetManagement/asset_manager.h"
#include "EngineUtils/String/hex_strings.h"

#include <fstream>
#include <cstring>

using namespace Engine::Core::AssetManagement;

static const char FileExtension[] = ".se_bin";

std::ifstream AssetManager::OpenAsset(const unsigned char* id) 
{
    // construct a path "<MD5>.se_bin"
    char path[16 * 2 + sizeof(FileExtension)];
    Utils::String::BinaryToHex(16, id, path);
    memcpy(path + 32, FileExtension, sizeof(FileExtension));

    // TODO: this implementation of asset folder is WRONG, need to figure out file organization
    std::ifstream result;
    result.open(path, std::ios::binary);
    return result;
}