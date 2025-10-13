#version 450

layout(location = 0) in vec3 v_Normal;
layout(location = 1) in vec2 v_Uv;
layout(location = 2) in vec3 v_ViewAngle;

// lights
struct DirectionalLight 
{
    vec3 Direction;
    vec3 Color;
};

layout(std140, set = 2, binding = 0) readlonly buffer t_DirectionalLights
{
    DirectionalLight b_DirectionalLights[];
}

layout(set = 3, binding = 0) uniform t_DirectionalLightCount
{
    uint u_DirectionalLightCount;
}

layout(location = 0) out vec4 f_color;

void main()
{
    vec3 normal = normalize(v_Normal);
    vec3 viewAngle = normalize(v_ViewAngle);

    // phong reflection parameters
    float kSpecular = 1.0;
    float kDiffuse = 1.0;
    float kAmbient = 0;
    float shininess = 3.0;
    vec4 ambientColor = vec4(0, 1, 0, 1);

    f_color = vec4(0, 0, 0, 1);

    // define these in world space (same as the vertex normals)
    for (uint i = 0; i < u_DirectionalLightCount; i++)
    {
        vec3 lightDirection = b_DirectionalLights[i].Direction;
        vec4 lightColor = vec4(b_DirectionalLights[i].Color, 1.0);

        // diffuse switch
        float diffuseStrength = dot(lightDirection, normal);
        float diffuseSwitch = step(0.0, diffuseStrength);
        
        // light model
        vec4 ambientShade = kAmbient * ambientColor;
        vec4 diffuseShade = kDiffuse * max(diffuseStrength, 0) * (lightColor * ambientColor);

        vec3 reflectedLight = normalize(2 * dot(lightDirection, normal) * normal - lightDirection);
        vec4 specularShade = diffuseSwitch * kSpecular * pow(max(dot(reflectedLight, viewAngle), 0), shininess) * (lightColor * ambientColor);

        // add the components up
        f_color += ambientShade + diffuseShade + specularShade;
    }
}
