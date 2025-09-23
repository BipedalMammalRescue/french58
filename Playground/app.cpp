#include <array>
#include <iostream>
#include <md5.h>

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
    constexpr std::array<unsigned char, 16> hash = md5::compute("111");
    for (char i = 0; i < 16; i++)
    {
        char low = hash[i] & 0x0F;
        char high = (hash[i] & 0xF0) >> 4;

        low = low <= 9 ? (low + '0') : (low - 9 + 'A');
        high = high <= 9 ? (high + '0') : (high - 9 + 'A');

        std::cout << high << low << '-';
    }

    std::cout << endl;

    return 0;
}
