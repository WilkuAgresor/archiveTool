#include <bit>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <type_traits>

// --- Write any integer (LE) ---
template <typename T>
inline void writeLE(std::ofstream& ofs, T value)
{
    static_assert(std::is_integral_v<T>, "writeLE requires an integer type");

    if constexpr (std::endian::native == std::endian::big)
    {
        // simple manual byte swap for big-endian systems
        unsigned char bytes[sizeof(T)];
        for (size_t i = 0; i < sizeof(T); ++i)
            bytes[i] = static_cast<unsigned char>((value >> (8 * i)) & 0xFF);
        ofs.write(reinterpret_cast<char*>(bytes), sizeof(T));
    }
    else
    {
        ofs.write(reinterpret_cast<const char*>(&value), sizeof(T));
    }
}

// --- Read any integer (LE) ---
template <typename T>
inline T readLE(std::ifstream& ifs)
{
    static_assert(std::is_integral_v<T>, "readLE requires an integer type");

    T value{};
    unsigned char bytes[sizeof(T)];
    ifs.read(reinterpret_cast<char*>(bytes), sizeof(T));

    if constexpr (std::endian::native == std::endian::big)
    {
        for (size_t i = 0; i < sizeof(T); ++i)
            value |= static_cast<T>(bytes[i]) << (8 * i);
    }
    else
    {
        std::memcpy(&value, bytes, sizeof(T));
    }

    return value;
}
