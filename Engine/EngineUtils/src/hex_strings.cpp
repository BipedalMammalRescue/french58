#include "EngineUtils/String/hex_strings.h"

static const char Quads[16] = {
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    'A',
    'B',
    'C',
    'D',
    'E',
    'F'
};

void Engine::Utils::String::BinaryToHex(size_t inputLen, const unsigned char* input, char* output)
{
    for (size_t i = 0; i < inputLen; i++)
    {
        unsigned char current = input[i];
        unsigned char currentL = current & 0x0F;
        unsigned char currentH = (current & 0xF0) >> 4;

        output[i * 2] = Quads[currentH];
        output[i * 2 + 1] = Quads[currentL];
    }
}