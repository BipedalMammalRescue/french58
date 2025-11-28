using System;

namespace RendererEditor.Abstractions;

public interface IRendererBuildingContext
{
    IRenderResource GetResource(string key);
}