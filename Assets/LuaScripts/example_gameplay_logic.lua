-- trigger on tick
if EngineQuery(SE_API_TABLE.EngineRootModule.CheckTickEvent) then
    rotation_speed = GetParameter("rotation_speed")
    delta_time = EngineQuery(SE_API_TABLE.EngineRootModule.GetDeltaTime)
    rotation_angle = rotation_speed * delta_time / 1000.0

    translation = vec3(0, 0, 0)
    scale = vec3(1, 1, 1)
    rotation = vec3(0, rotation_angle, 0)

    RaiseEvent(SE_EVENT_TABLE.EngineRootModule.TransformUpdateEvent, SE_ENTITY_ID, translation, scale, rotation)
end