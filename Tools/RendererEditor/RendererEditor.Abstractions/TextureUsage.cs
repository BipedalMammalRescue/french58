namespace RendererEditor.Abstractions;

public enum TextureUsage
{
    SAMPLER = 1 << 0, /**< Texture supports sampling. */
    COLOR_TARGET = 1 << 1, /**< Texture is a color render target. */
    DEPTH_STENCIL_TARGET = 1 << 2, /**< Texture is a depth stencil target. */
    GRAPHICS_STORAGE_READ = 1 << 3, /**< Texture supports storage reads in graphics stages. */
    COMPUTE_STORAGE_READ = 1 << 4, /**< Texture supports storage reads in the compute stage. */
    COMPUTE_STORAGE_WRITE = 1 << 5, /**< Texture supports storage writes in the compute stage. */
    COMPUTE_STORAGE_SIMULTANEOUS_READ_WRITE = 1 << 6 /**< Texture supports reads and writes in the same compute shader. This is NOT equivalent to READ | WRITE. */
}