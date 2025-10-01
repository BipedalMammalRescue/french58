#include <stdio.h>
#include <array>


int main()
{

    std::array<int, 3> a = {1, 2, 3};
    std::array<int, 3> b = a;

    b[1] = 10;

    printf("%d", a[1]);
}
