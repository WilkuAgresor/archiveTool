#pragma once
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

inline constexpr uint64_t FNV1A_OFFSET_BASIS = 14695981039346656037ull;
inline constexpr uint64_t FNV1A_PRIME        = 1099511628211ull;

#include <bit>


/**
 * Compute 64-bit FNV-1a hash from memory buffer.
 */
inline uint64_t fnv1a_hash(const std::string& data)
{
    uint64_t hash = FNV1A_OFFSET_BASIS;
    for (unsigned char c : data)
    {
        hash = (hash ^ c) * FNV1A_PRIME;
    }
    return hash;
}

/**
 * Load file contents into provided buffer and compute FNV-1a hash.
 * The buffer is resized automatically and can be reused.
 */
inline uint64_t fnv1a_hash_file(const std::filesystem::path& path, std::string& buffer)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    if (!ifs)
        throw std::runtime_error("Failed to open file: " + path.string());

    std::streamsize size = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    buffer.resize(static_cast<size_t>(size));
    if (size > 0)
    {
        if (!ifs.read(buffer.data(), size))
            throw std::runtime_error("Failed to read file: " + path.string());
    }

    return fnv1a_hash(buffer);
}
