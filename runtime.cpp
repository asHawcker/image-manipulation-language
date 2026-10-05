#include <iostream>
#include <cstdint>

// extern "C" prevents C++ name mangling so LLVM can find these exactly by name
extern "C"
{
    void print(int x)
    {
        printf("%d\n", x);
    }
    void fprint(float x)
    {
        printf("%f\n", x);
    }
    void print_str(const char *s)
    {
        printf("%s\n", s);
    }
}
