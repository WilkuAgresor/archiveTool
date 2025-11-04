Time traveller is stuck and needs your help.
Provide a console application that will create a single archive file containing the specified folder structure.

The program is architecture independent. The only dependency is Zstandard library (https://github.com/facebook/zstd).
C++20

Usage:
./archiveTool pack [input_dir] [archive_path]
./archiveTool unpack [archive_path] [output_dir]

Results were compared to the tar->gzip lvl 6 result. Overall - significantly better in every metric. Zstd and custom deduplication made wonders.

Example files generator:
./benchmarkGen.sh - creates random binary files, uncompressable. tar+gzip scored negative compression values, while archiveTool scored about 1.
./benchmarkFsGenText.sh - populates the folder structure with large simulated log text files. It often stays in infinite loop - just kill it after you have enough data (multiple GB) - faster than try to fix it, does the job. 

Benchmark tools:
./compressionRatio.sh [input_folder] [archive_file]
Run the program through 'time'.

Flaws:
The archive does not retain content metadata (last-modified-time, permissions, owners), so validation of pack/unpack operation requires 'rinc'.
rsync -rinc --delete --out-format='%n' root/ unpacked/
