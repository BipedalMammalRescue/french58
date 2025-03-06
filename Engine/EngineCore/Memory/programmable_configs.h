namespace Engine {
namespace Core {
namespace Memory {

template <size_t K = 0, size_t B = 1> class LinearGrowth
{
  public:
    static size_t NextAllocationSize(size_t CurrentSize)
    {
        return K * CurrentSize + B;
    }
};

} // namespace Memory
} // namespace Core
} // namespace Engine
