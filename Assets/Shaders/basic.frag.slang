import data;

layout(std140, set = 2, binding = 0) StructuredBuffer<DirectionalLight> DirectionalLightBuffer;
layout(set = 3, binding = 0) ConstantBuffer<uint> DirectionalLightCount;

[shader("fragment")]
float4 main(VertexStageOutput input)
{
    float3 normal = normalize(input.Normal);
    float3 viewAngle = normalize(input.ViewAngle);

    // phong reflection parameters
    // TODO: move all these into material editor
    float kSpecular = 1.0;
    float kDiffuse = 0.5;
    float kAmbient = 0;
    float shininess = 10;
    float4 ambientColor = float4(0, 1, 0, 1);

    float4 outputColor = float4(0, 0, 0, 1);

    for (uint i = 0; i < DirectionalLightCount; i++)
    {
        float3 lightDirection = normalize(DirectionalLightBuffer[i].Direction);
        float4 lightColor = float4(DirectionalLightBuffer[i].Color, 1.0);

        // diffuse switch
        float diffuseStrength = dot(lightDirection, normal);
        float diffuseSwitch = step(0.0, diffuseStrength);

        // light model
        float4 ambientShade = kAmbient * ambientColor;
        float4 diffuseShade = kDiffuse * max(diffuseStrength, 0) * (lightColor * ambientColor);

        float3 reflectedLight = normalize(2 * dot(lightDirection, normal) * normal - lightDirection);
        float4 specularShade = diffuseSwitch * kSpecular * pow(max(dot(reflectedLight, viewAngle), 0), shininess) * (lightColor * ambientColor);

        // add the components up
        outputColor += ambientShade + diffuseShade + specularShade;
    }

    return outputColor;
}