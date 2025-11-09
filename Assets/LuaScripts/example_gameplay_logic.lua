-- trigger on tick
if QueryEngineApi(SE_API_TABLE.EngineRootModule.CheckTickEvent) then
    print(QueryEngineApi(SE_API_TABLE.EngineRootModule.GetTotalTime))
end
