#include <iostream>
#include <exception>

#define SE_TEST_RUNTEST(testName)\
try \
{\
    bool result = testName();\
    if (!result)\
    {\
        std::cout << "fail: " << #testName << std::endl;\
    }\
    else\
    {\
        std::cout << "pass: " << #testName << std::endl;\
    }\
}\
catch (const std::exception& ex)\
{\
    std::cout << "fail: " << #testName << " [err: " << ex.what() << "]" << std::endl;\
}

bool BpTreeTest() 
{
    return true;
}

int main()
{
    SE_TEST_RUNTEST(BpTreeTest);
    return 0;
}
