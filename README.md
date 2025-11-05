🕰️ ArchiveTool – Help the Time Traveller!

The Time Traveller is stuck in time — and only your archive can save him.
The mission: build a fast, architecture-independent console application that packs and unpacks entire folder structures into a single archive file.

🚀 Overview

ArchiveTool is a cross-platform, C++20 application for efficient folder archiving using Zstandard (Zstd) compression.
It features custom deduplication and outperforms traditional tools like tar + gzip in both compression ratio and speed.

Library dependencies: Zstandard by Meta (Facebook) https://github.com/facebook/zstd

🧩 Features
✅ Architecture-independent (only depends on Zstd)
⚙️ High-performance compression with custom deduplication
🧮 Better compression ratio than tar + gzip level 6
📦 Simple CLI interface for packing/unpacking folders
⚡ Benchmark scripts included for reproducible tests

🧰 Usage
# Pack a directory into an archive
./archiveTool pack [input_dir] [archive_path]

# Unpack an archive to a directory
./archiveTool unpack [archive_path] [output_dir]

📊 Benchmarks

Results were compared against tar -> gzip -6.
ArchiveTool consistently achieved significantly better compression ratios and faster processing times.

Zstd combined with custom deduplication made wonders.

🧪 Benchmark Tools

🔹 Random binary data generator

Creates some uncompressible binary random files, tar+gzip scored negative compression values, while archiveTool typically scores near 1:

./benchmarkGen.sh


🔹 Simulated log/text filesystem generator

Populates a folder tree with large simulated text/log files:

./benchmarkFsGenText.sh

⚠️ This script may loop indefinitely — just stop it once you have enough data (a few GB). It’s faster to kill than fix, and it does the job well.

🔹 Compression ratio checker
Compare input folder size to the resulting archive:
./compressionRatio.sh [input_folder] [archive_file]


Use with time to measure runtime:
time ./archiveTool pack ...

⚠️ Known Limitations
File metadata (timestamps, permissions, ownership) is not preserved.
To verify integrity, compare file checksums:

rsync -rinc --delete --out-format='%n' root/ unpacked/
