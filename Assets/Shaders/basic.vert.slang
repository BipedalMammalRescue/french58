import data;

layout(set = 1, binding = 0) ConstantBuffer<float4x4> ModelMatrix;
layout(set = 1, binding = 1) ConstantBuffer<float4x4> PvMatrix;
layout(set = 1, binding = 2) ConstantBuffer<float3> CameraPosition;

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