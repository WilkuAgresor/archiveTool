#include <fstream>
#include <vector>
#include <string>
#include <stdexcept>
#include <cstdint>

constexpr uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
constexpr uint64_t FNV_PRIME        = 1099511628211ULL;
constexpr size_t   HASH_PREFIX_SIZE = 64 * 1024; // 64 KiB

static uint64_t fnv1a_update(uint64_t hash, const void* data, size_t len)
{
    const unsigned char* p = static_cast<const unsigned char*>(data);
    for (size_t i = 0; i < len; ++i)
        hash = (hash ^ p[i]) * FNV_PRIME;
    return hash;
}
// Compute the FNV-1a hash of the first `prefixSize` bytes of the file
uint64_t computePrefixHash(const std::string& path, size_t prefixSize = HASH_PREFIX_SIZE)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open file: " + path);

    std::vector<char> buffer(prefixSize);
    file.read(buffer.data(), buffer.size());
    std::streamsize bytesRead = file.gcount();

    uint64_t hash = FNV_OFFSET_BASIS;
    hash = fnv1a_update(hash, buffer.data(), static_cast<size_t>(bytesRead));

    return hash;
}

// Compute the FNV-1a hash of the *entire* file efficiently
uint64_t computeFullHash(const std::string& path, size_t chunkSize = 256 * 1024)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open file: " + path);

    std::vector<char> buffer(chunkSize);
    uint64_t hash = FNV_OFFSET_BASIS;

    while (file)
    {
        file.read(buffer.data(), buffer.size());
        std::streamsize bytesRead = file.gcount();
        if (bytesRead > 0)
            hash = fnv1a_update(hash, buffer.data(), static_cast<size_t>(bytesRead));
    }

    return hash;
}

