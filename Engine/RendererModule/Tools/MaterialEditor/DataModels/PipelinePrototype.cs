namespace DataModels;

public class ShaderStage
{
    public ShaderUniform[] Uniforms { get; set; } = [];
    public ShaderStorageBuffer[] StorageBuffers { get; set; } = [];
}

public class PipelinePrototype
{
    // probably allow other stages here too, v/f are the only mandatory elements
    public required ShaderStage VertexShader { get; set; }
    public required ShaderStage FragmentShader { get; set; }
}
