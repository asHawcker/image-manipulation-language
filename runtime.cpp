#include <iostream>
#include <cstdint>

// extern "C" prevents C++ name mangling so LLVM can find these exactly by name
extern "C"
{
    void print(int32_t val)
    {
        std::cout << "IML Output: " << val << std::endl;
    }

    void iml_main();
}

int main()
{
    std::cout << "--- Starting IML Program ---" << std::endl;

    iml_main();

    std::cout << "--- IML Program Finished ---" << std::endl;
    return 0;
}