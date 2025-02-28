#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

using namespace std;



int main()
{
    std::stringstream stream;
    stream << "hello world" << endl;

    string data;
    stream >> data;

    cout << data << endl;
}
