import data;

layout(binding = 0, set = 1) ConstantBuffer<float4x4> ModelMatrix;
layout(binding = 1, set = 1) ConstantBuffer<float4x4> PvMatrix;
layout(binding = 2, set = 1) ConstantBuffer<float3> CameraPosition;

[shader("vertex")]
VertexStageOutput main(VertexStageInput input)
{
    VertexStageOutput output;

    output.Position = mul(mul(PvMatrix, ModelMatrix), float4(input.Position, 1.0));

    float3 worldNormal = (mul(ModelMatrix, float4(input.Normal, 1.0)) - mul(ModelMatrix, float4(0, 0, 0, 1.0))).xyz;
    output.Normal = normalize(worldNormal);

    output.Uv = input.Uv;

    output.ViewAngle = normalize(CameraPosition - mul(ModelMatrix, float4(input.Position, 1.0)).xyz);

    return output;
}