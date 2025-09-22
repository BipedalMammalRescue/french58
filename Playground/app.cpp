#include "glm/fwd.hpp"
#include "glm/gtc/constants.hpp"
#include <array>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace std;

void Foobar(const std::array<int, 4>& id)
{
    for (int i = 0; i < 4; i++)
    {
        std::cout << id[i] << std::endl;
    }
}

void Foobar(const std::array<int, 4>&& id)
{
    for (int i = 0; i < 4; i++)
    {
        std::cout << id[i] << std::endl;
    }
}

int main()
{
    Foobar({10, 20, 30, 40});
    std::array<int, 4> arr {10, 20, 30, 40};
    Foobar(arr);

    return 0;
}
