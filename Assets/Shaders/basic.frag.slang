import data;

// storage buffers are injected
layout(std140, set = 2, binding = 0) StructuredBuffer<DirectionalLight> DirectionalLightBuffer;

// uniforms that are fed through the material
layout(set = 3, binding = 0) ConstantBuffer<float> kSpecular;
layout(set = 3, binding = 1) ConstantBuffer<float> kDiffuse;
layout(set = 3, binding = 2) ConstantBuffer<float> kAmbient;
layout(set = 3, binding = 3) ConstantBuffer<float> kShininess;
layout(set = 3, binding = 4) ConstantBuffer<float3> ambientColor;

[shader("fragment")]
float4 main(VertexStageOutput input)
{
    float3 normal = normalize(input.Normal);
    float3 viewAngle = normalize(input.ViewAngle);

    // phong reflection parameters
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
        float3 specularShade = diffuseSwitch * kSpecular * pow(max(dot(reflectedLight, viewAngle), 0), kShininess) * (lightColor * ambientColor);

        // add the components up
        outputColor += ambientShade + diffuseShade + specularShade;
    }

    return float4(outputColor, 1.0);
}