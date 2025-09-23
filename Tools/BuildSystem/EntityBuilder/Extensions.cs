using EntityBuilder.Abstractions;
using MuThr.Sdk;

namespace EntityBuilder;

public static class Extensions
{
    public static (CompiledEntity[], ComponentGroup[]) Compile(this Entity input, Coordinator assetOutlet)
    {
        List<CompiledEntity> entities = [];
        List<(string Module, string Type, CompiledComponent Component)> components = [];

        Queue<(Entity, int)> queue = [];
        queue.Enqueue((input, -1));
        while (queue.Count > 0)
        {
            (Entity current, int parent) = queue.Dequeue();
            int currentId = entities.Count;
            entities.Add(new CompiledEntity(currentId, parent));

            foreach (Entity child in current.Children)
            {
                queue.Enqueue((child, currentId));
            }

            foreach (Component component in current.Components)
            {
                int componentId = components.Count;
                components.Add((component.Module, component.Type, new CompiledComponent(componentId, currentId, [.. component.Fields.Select(pair => new ComponentField(pair.Key, pair.Value))])));
                foreach (Variant field in component.Fields.Values.Where(v => v.Type == VariantType.Path))
                {
                    assetOutlet.ScheduleTask(field.Path);
                }
            }
        }

        ComponentGroup[] componentGroups = [.. components.GroupBy(c => c.Module).SelectMany(moduleGroup => moduleGroup.GroupBy(c => c.Type).Select(typeGroup => new ComponentGroup(moduleGroup.Key, typeGroup.Key, [.. typeGroup.Select(x => x.Component)])))];
        return ([.. entities], componentGroups);
    }
}
