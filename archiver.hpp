#include "fnv1a.hpp"
#include "miniJson.hpp"
#include "endianHelpers.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <zstd.h>

namespace fs = std::filesystem;

// --- archive writer ---
class ArchiveWriter
{
    std::ofstream ofs;
    std::unordered_map<uint64_t, uint64_t> written; // hash -> position
public:
    ArchiveWriter(const fs::path& out) : ofs(out, std::ios::binary) {}

    void add_file_if_new(const fs::path& path, uint64_t hash, const std::string& data)
    {
        if (written.count(hash))
        {
            std::cout << "file: " << path << " already added.\n";
            return;
        }

        size_t bound = ZSTD_compressBound(data.size());
        std::string compressed(bound, '\0');
        size_t csize = ZSTD_compress(compressed.data(), bound, data.data(), data.size(), 3);
        ofs.write("ZSTD", 4);
        //for hashes, endianness doesn't matter
        ofs.write(reinterpret_cast<char*>(&hash), sizeof(hash));
        uint64_t usize = data.size();

        writeUint64LE(ofs, usize);
        writeUint64LE(ofs, csize);

        ofs.write(compressed.data(), csize);
        written[hash] = ofs.tellp();
    }

    void write_header(const std::string& header)
    {
        uint64_t hlen = header.size();
        ofs.write("HDR0", 4);        
        writeUint64LE(ofs, hlen);
        ofs.write(header.data(), hlen);
    }
};

// --- archive reader ---
class ArchiveReader
{
    std::ifstream ifs;

public:
    ArchiveReader(const fs::path& in) : ifs(in, std::ios::binary) {}

    std::string read_header()
    {
        // Read from end
        ifs.seekg(0, std::ios::end);
        std::streampos end       = ifs.tellg();
        std::streamoff pos       = end;
        const std::string marker = "HDR0";

        // scan backwards up to some limit
        const size_t scan_window = 4096;
        while (pos > 0)
        {
            std::streamoff chunk_size = std::min<std::streamoff>(scan_window, pos);
            pos -= chunk_size;
            ifs.seekg(pos);
            std::string buf(chunk_size, '\0');
            ifs.read(buf.data(), chunk_size);
            size_t found = buf.rfind(marker);
            if (found != std::string::npos)
            {
                // found header tag
                ifs.seekg(pos + found + 4);
                uint64_t len = readUint64LE(ifs);

                std::string h(len, '\0');
                ifs.read(h.data(), len);
                return h;
            }
        }
        throw std::runtime_error("no header");
    }

    void extract_file(const uint64_t hash, const fs::path& outdir)
    {
        ifs.clear();
        ifs.seekg(0);
        while (true)
        {
            char tag[4];
            if (!ifs.read(tag, 4))
                break;
            if (std::string(tag, 4) != "ZSTD")
            {
                ifs.seekg(-3, std::ios::cur);
                continue;
            }
            uint64_t h = readUint64LE(ifs);
            uint64_t usize = readUint64LE(ifs);
            uint64_t csize = readUint64LE(ifs);

            std::string comp(csize, '\0');
            ifs.read(comp.data(), csize);
            if (h == hash)
            {
                std::string data(usize, '\0');
                ZSTD_decompress(data.data(), usize, comp.data(), csize);
                std::ofstream ofs(outdir, std::ios::binary);
                ofs.write(data.data(), data.size());
                return;
            }
        }
    }
};

static void build_structure(const fs::path& dir, mini_json::object& node, ArchiveWriter& writer)
{
    for (auto& e : fs::directory_iterator(dir))
    {
        if (e.is_directory())
        {
            mini_json::object sub;
            build_structure(e.path(), sub, writer);
            node[e.path().filename().string()] = std::move(sub);
        }
        else if (e.is_regular_file())
        {
            //file size < 2GB, can safely load all to memory. Reuse the buffer (hashing+compression) for performance.
            std::string data;
            auto hash = fnv1a_hash_file(e.path(), data);
            writer.add_file_if_new(e.path(), hash, data);
            node[e.path().filename().string()] = hash;
        }
    }
}

static void restore_structure(const mini_json::object& node, const fs::path& base, ArchiveReader& reader, std::unordered_map<uint64_t, fs::path>& cache)
{
    for (auto& [name, val] : node)
    {
        if (val.is_object())
        {
            fs::create_directories(base / name);
            restore_structure(val.as_object(), base / name, reader, cache);
        }
        else
        {
            auto hash        = val.as_uint64();
            fs::path outpath = base / name;
            if (auto it = cache.find(hash); it != cache.end())
            {
                fs::copy_file(it->second, outpath, fs::copy_options::overwrite_existing);
            }
            else
            {
                reader.extract_file(hash, outpath);
                cache[hash] = outpath;
            }
        }
    }
}
