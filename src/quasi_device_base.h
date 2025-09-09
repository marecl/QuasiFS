#include <cstdint>

class QuasiBase
{
    QuasiBase() = default;
    ~QuasiBase() = default;

protected:
    uint32_t fileno{};
    
};