import data;

// storage buffers
layout(std140, set = 2, binding = 0) StructuredBuffer<DirectionalLight> DirectionalLightBuffer;

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
    float3 ambientColor = float3(0, 1, 0);

    float3 outputColor = float3(0, 0, 0);

    for (uint i = 0; i < DirectionalLightBuffer.getCount(); i++)
    {
        float3 lightDirection = normalize(DirectionalLightBuffer[i].Direction);
        float3 lightColor = float3(DirectionalLightBuffer[i].Color);

        // diffuse switch
        float diffuseStrength = dot(lightDirection, normal);
        float diffuseSwitch = step(0.0, diffuseStrength);

        // light model
        float3 ambientShade = kAmbient * ambientColor;
        float3 diffuseShade = kDiffuse * max(diffuseStrength, 0) * (lightColor * ambientColor);

        float3 reflectedLight = normalize(2 * dot(lightDirection, normal) * normal - lightDirection);
        float3 specularShade = diffuseSwitch * kSpecular * pow(max(dot(reflectedLight, viewAngle), 0), shininess) * (lightColor * ambientColor);

        // add the components up
        outputColor += ambientShade + diffuseShade + specularShade;
    }

    return float4(outputColor, 1.0);
}