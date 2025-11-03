#!/usr/bin/env bash
set -euo pipefail

RANDOM=$(( (SECONDS ^ $$ ^ $(date +%N)) & 0x7FFF ))

ROOT_DIR="random_fs"
MAX_DEPTH=10
MIN_FILES=5
MAX_FILES=20
MIN_SIZE=500M
MAX_SIZE=2000M
SHARED_COUNT=14

mkdir -p "$ROOT_DIR"

# redirect all errors to stderr only, don't mix with stdout
exec 3>&2

echo "📦 Generating $SHARED_COUNT shared files..."
mkdir -p "$ROOT_DIR/shared"
for ((i=1; i<=SHARED_COUNT; i++)); do
    size=$(( (RANDOM % 102400000) + 1000000 ))K
    truncate -s "$size" "$ROOT_DIR/shared/shared_$i.bin"
done
mapfile -t SHARED_FILES < <(find "$ROOT_DIR/shared" -type f)
SHARED_LEN=${#SHARED_FILES[@]}

generate_files() {
    local dir="$1"
    local depth="$2"

    echo "  → generate_files in $dir" >&3
    local num_files=$(( RANDOM % (MAX_FILES - MIN_FILES + 1) + MIN_FILES ))

    for ((i=1; i<=num_files; i++)); do
        local fpath="$dir/file_$i.bin"
        local mode=$(( RANDOM % 3 ))

	 # Weighted file size selection: small(50%) medium(30%) large(20%)
        local roll=$(( RANDOM % 100 ))
        local size_bytes
        if (( roll < 30 )); then
            size_bytes=$(( (RANDOM % (1024*1024)) + 1024 ))             # up to 1 MB
        elif (( roll < 90 )); then
            size_bytes=$(( (RANDOM % (100*1024*1024)) + (1*1024*1024) )) # up to 100 MB
        else
            size_bytes=$(( (RANDOM % (2*1024*1024*1024)) + (10*1024*1024) )) # up to 2 GB
        fi
	echo size "$size_bytes" bytes for "$fpath" using mode $mode >&3

        case $mode in
            0)
                truncate -s "$size_bytes" "$fpath" 2>/dev/null || true
                ;;
            1)
                cp "${SHARED_FILES[RANDOM % SHARED_LEN]}" "$fpath" 2>/dev/null || true
                ;;
            2)
                # For large files, avoid /dev/urandom full write — too slow
                if (( size_bytes < 5*1024*1024 )); then
                    head -c "$size_bytes" /dev/urandom >"$fpath" 2>/dev/null || true
                else
                    dd if=/dev/urandom of="$fpath" bs=1M count=20 status=none 2>/dev/null || true
                    truncate -s "$size_bytes" "$fpath"
                fi
                ;;
        esac
    done
}

generate_dir() {
    local dir="$1"
    local depth="$2"

    echo "📁 Entering $dir (depth=$depth)" >&3
    mkdir -p "$dir"

    generate_files "$dir" "$depth"

    if (( depth < MAX_DEPTH )); then
        local subdirs=$(( RANDOM % 10 + 1 ))
        for ((i=1; i<=subdirs; i++)); do
            generate_dir "$dir/child_$i" $((depth + 1))
        done
    fi

    echo "📁 Leaving $dir (depth=$depth)" >&3
}

generate_dir "$ROOT_DIR/root" 1
echo "✅ done!" >&3

