import data;

// these are injected
layout(set = 1, binding = 0) ConstantBuffer<float4x4> ModelMatrix;
layout(set = 1, binding = 1) ConstantBuffer<float4x4> ViewMatrix;
layout(set = 1, binding = 2) ConstantBuffer<float4x4> ProjectionMatrix;

[shader("vertex")]
VertexStageOutput main(VertexStageInput input)
{
    var CameraPosition = transpose(ViewMatrix)[3].xyz;

    VertexStageOutput output;

    float4x4 pvMatrix = mul(ProjectionMatrix, ViewMatrix);
    output.Position = mul(mul(pvMatrix, ModelMatrix), float4(input.Position, 1.0));

    float3 worldNormal = (mul(ModelMatrix, float4(input.Normal, 1.0)) - mul(ModelMatrix, float4(0, 0, 0, 1.0))).xyz;
    output.Normal = normalize(worldNormal);

    output.Uv = input.Uv;

    output.ViewAngle = normalize(CameraPosition - mul(ModelMatrix, float4(input.Position, 1.0)).xyz);

    return output;
}