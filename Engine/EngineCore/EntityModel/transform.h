#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/fwd.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Engine {
namespace Core {
namespace EntityModel {

// Sigourney Engine treats transforms as special objects attached to an entity, not a component; 
// It's motivated by the need to use entities as scene description directlly.
struct Transform 
{
    // nullptr means you are in world space
    const Transform* Parent = nullptr;
    glm::vec3 Translation;
    glm::quat Rotation;
    glm::vec3 Scale;

    inline glm::mat4 WorldToLocal() const 
    {
        if (Parent == nullptr) 
        {
            return ParentToLocal();
        }

        return ParentToLocal() * Parent->ParentToLocal();
    }

    inline glm::mat4 LocalToWorld() const 
    {
        if (Parent == nullptr)
        {
            return LocalToParent();
        }

        return Parent->LocalToParent() * LocalToParent();
    }
    
    inline glm::mat4 ParentToLocal() const 
    {
        // reverse self transform
        glm::mat4 reverseTranslate = glm::translate(glm::mat4(1.0f), -1.0f * Translation);
        glm::mat4 reverseRotation = glm::mat4_cast(glm::inverse(Rotation));
        glm::mat4 reverseScale = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f / Scale.x, 1.0f / Scale.y, 1.0f / Scale.z));
        return reverseScale * reverseRotation * reverseTranslate;
    }

    inline glm::mat4 LocalToParent() const
    {
        // forward self transform
        return glm::translate(glm::mat4_cast(Rotation) * glm::scale(glm::mat4(1.0f), Scale), Translation);
    }
};

}
}
}
