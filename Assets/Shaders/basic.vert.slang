import data;

layout(binding = 0, set = 1) ConstantBuffer<float4x4> MvpMatrix;
layout(binding = 1, set = 1) ConstantBuffer<float3> CameraPosition;

[shader("vertex")]
VertexStageOutput main(VertexStageInput input)
{
    VertexStageOutput output;

    output.Position = mul(MvpMatrix, float4(input.Position, 1.0));

    output.Normal = input.Normal;
    output.Uv = input.Uv;
    output.ViewAngle = normalize(CameraPosition - input.Position);

    return output;
}