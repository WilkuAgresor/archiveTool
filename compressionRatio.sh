#!/bin/bash
# Compute compression ratio between a source folder and its archive
# Usage: ./compression_ratio.sh <input_folder> <archive_file>

set -e

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <input_folder> <archive_file>"
    exit 1
fi

INPUT_DIR="$1"
ARCHIVE_FILE="$2"

# Ensure input folder exists
if [ ! -d "$INPUT_DIR" ]; then
    echo "Error: Input folder '$INPUT_DIR' not found."
    exit 1
fi

# Ensure archive file exists
if [ ! -f "$ARCHIVE_FILE" ]; then
    echo "Error: Archive file '$ARCHIVE_FILE' not found."
    exit 1
fi

# Compute folder size (in bytes)
FOLDER_SIZE=$(du -sb "$INPUT_DIR" | awk '{print $1}')

# Compute archive size (in bytes)
ARCHIVE_SIZE=$(stat -c%s "$ARCHIVE_FILE")

# Compute compression ratio (e.g., 23.45%)
RATIO=$(awk "BEGIN {printf \"%.2f\", 100 * $ARCHIVE_SIZE / $FOLDER_SIZE}")

# Human-readable sizes
HUMAN_FOLDER=$(du -sh "$INPUT_DIR" | awk '{print $1}')
HUMAN_ARCHIVE=$(du -sh "$ARCHIVE_FILE" | awk '{print $1}')

echo "📂 Input folder:   $INPUT_DIR ($HUMAN_FOLDER)"
echo "🗜️  Archive file:   $ARCHIVE_FILE ($HUMAN_ARCHIVE)"
echo "📊 Compression ratio: $RATIO %"
echo "   (Archive size is $RATIO% of original)"

