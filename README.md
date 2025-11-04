Time traveller is stuck and needs your help.
Provide a console application that will create a single archive file containing the specified folder structure.

The program is architecture independent. The only specific dependency is Zstandard library (https://github.com/facebook/zstd).

Usage:
./archiveTool pack [input_dir] [archive_path]
./archiveTool unpack [archive_path] [output_dir]

Results were compared to the tar->gzip lvl 6 result. Overall - significantly better in every metric. Zstd and custom deduplication made wonders.

Example files generator:
./benchmarkGen.sh - creates random binary files, uncompressable. tar+gzip scored negative compression values, while archiveTool scored about 1.

Benchmark tools:
./compressionRatio.sh [input_folder] [archive_file]
Run the program through time.

The archive does not retain content metadata (last-modified-time, permissions, owners), so validation of pack/unpack operation requires 'rinc'.
rsync -rinc --delete --out-format='%n' root/ unpacked/

