#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>

#include <komihash.h>
#include <vector>

using namespace std;

template <typename TCallback> void Tokenize(std::istream &input, TCallback &&callback)
{
    size_t count = 0;
    std::string lineCache;
    std::vector<const char *> words;
    words.reserve(5);

    while (std::getline(input, lineCache))
    {
        // split the line into words
        size_t currentSegment = 0;
        for (size_t pos = 0; pos < lineCache.size(); pos++)
        {
            if (lineCache[pos] == ' ')
            {
                words.push_back(lineCache.c_str() + currentSegment);
                lineCache[pos] = 0;
                currentSegment = pos + 1;
            }
        }

        words.push_back(lineCache.c_str() + currentSegment);
        callback(words);
        words.clear();
    }
}

int main()
{
    ifstream file("test.txt");
    istream &filePtr = file;
    string line;

    Tokenize(file, [](const std::vector<const char *> &line) {
        for (const char *word : line)
        {
            printf("{%s} ", word);
        }
        printf("\n");
    });

    return 0;
}
