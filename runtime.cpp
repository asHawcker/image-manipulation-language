#include <iostream>
#include <cstdint>

// extern "C" prevents C++ name mangling so LLVM can find these exactly by name
extern "C"
{
    void print(int x)
    {
        printf("%d\n", x);
    }
}
