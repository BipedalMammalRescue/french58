using EntityBuilder.Abstractions;

namespace EntityBuilder;

public record ComponentGroup(string Module, string Type, CompiledComponent[] Components);

public record CompiledComponent(int Id, int Entity, ComponentField[] Fields);

public record ComponentField(string Name, Variant Value);