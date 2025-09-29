
#include <exception>
#include <optional>
#include <stdexcept>
#include <stdio.h>
class Foobar
{
public:
    ~Foobar() 
    {
        printf("dtor\n");
    }
};

int main()
{
    std::optional<Foobar> result((Foobar()));

    try 
    {
        Foobar var;
        throw std::runtime_error("1111");
    }
    catch (std::exception& ex)
    {
        printf("%s\n", ex.what());
    }

    return 0;
}
