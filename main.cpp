#include "archiver.hpp"
#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 4)
    {
        std::cout << "Usage:\n"
                  << "  pack <folder> <archive>\n"
                  << "  unpack <archive> <outdir>\n";
        return 0;
    }

    std::string mode = argv[1];
    if (mode == "pack")
    {
        fs::path folder  = argv[2];
        fs::path archive = argv[3];
        ArchiveWriter writer(archive);
        mini_json::object root;

        if(!fs::exists(folder))
        {
            std::cout << "The folder " << folder << " does not exist.\n";
            return 1;
        }
        build_structure(folder, root, writer);
        std::string header = mini_json::dump(root, 2);
        writer.write_header(header);
        std::cout << "structure: " << header << "\n";
    }
    else if (mode == "unpack")
    {        
        fs::path archive = argv[2];
        fs::path outdir  = argv[3];

        if(!fs::exists(archive))
        {
            std::cout << "The archive " << archive << " does not exist.\n";
            return 1;
        }

        if(!fs::exists(outdir))
        {
            fs::create_directories(outdir);
        }

        ArchiveReader reader(archive);
        std::string hdr        = reader.read_header();
        mini_json::object root = mini_json::parse(hdr).as_object();
        std::unordered_map<uint64_t, fs::path> cache;
        restore_structure(root, outdir, reader, cache);
        std::cout << "Unpacked to " << outdir << "\n";
    }
}
