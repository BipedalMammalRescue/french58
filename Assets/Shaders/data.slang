module data;

public struct DirectionalLight
{
    public float3 Direction;
    public float3 Color;
};

public struct VertexStageInput
{
    public float3 Position;
    public float3 Normal;
    public float2 Uv;
};

public struct VertexStageOutput
{
    public float4 Position : SV_Position;
    public float3 Normal : Normal;
    public float2 Uv : UV;
    public float3 ViewAngle : ViewAngle;
};