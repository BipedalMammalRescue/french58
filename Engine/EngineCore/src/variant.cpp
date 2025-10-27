#include "EngineCore/Pipeline/variant.h"

size_t Engine::Core::Pipeline::GetVariantPayloadSize(const Variant& source)
{
    size_t dataSize = 0;
    switch (source.Type)
    {
    case VariantType::Byte:
    case VariantType::Bool:
        dataSize = 1;
        break;
    case VariantType::Int32:
    case VariantType::Uint32:
    case VariantType::Float:
        dataSize = 4;
        break;
    case VariantType::Int64:
    case VariantType::Uint64:
        dataSize = 8;
        break;
    case VariantType::Vec2:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Vec2);
        break;
    case VariantType::Vec3:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Vec3);
        break;
    case VariantType::Vec4:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Vec4);
        break;
    case VariantType::Mat2:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Mat2);
        break;
    case VariantType::Mat3:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Mat4);
        break;
    case VariantType::Mat4:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Mat4);
        break;
    case VariantType::Path:
        dataSize = sizeof(Core::Pipeline::Variant::Data.Path);
        break;
    case VariantType::Invalid:
        dataSize = 0;
        break;
    }
    return dataSize;
}