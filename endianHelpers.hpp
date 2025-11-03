#include <bit>

inline void writeUint64LE(std::ofstream& ofs, uint64_t value) {
    if constexpr (std::endian::native == std::endian::big)
        value = __builtin_bswap64(value);
    ofs.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

inline uint64_t readUint64LE(std::ifstream& ifs) {
    uint64_t value;
    ifs.read(reinterpret_cast<char*>(&value), sizeof(value));
    if constexpr (std::endian::native == std::endian::big)
        value = __builtin_bswap64(value);
    return value;
}
