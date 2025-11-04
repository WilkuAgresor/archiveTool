#!/usr/bin/env bash
set -euo pipefail

RANDOM=$(( (SECONDS ^ $$ ^ $(date +%N)) & 0x7FFF ))

ROOT_DIR="random_fs"
MAX_DEPTH=6
MIN_FILES=2
MAX_FILES=5
SHARED_COUNT=14

mkdir -p "$ROOT_DIR"

# redirect all errors to stderr only, don't mix with stdout
exec 3>&2

gen_mixed_data2() {
    local fpath="$1"
    local size_bytes=$(( RANDOM % (2 * 1024 * 1024 * 1024) )) # up to 2 GiB
    (( size_bytes < 1024 * 1024 )) && size_bytes=$(( 1024 * 1024 )) # min 1 MiB

    echo "Generating $fpath (~$((size_bytes / 1024 / 1024)) MiB)..."

    {
        echo "{"
        echo "  \"id\": $RANDOM,"
        echo "  \"timestamp\": \"$(date -Iseconds)\","
        echo "  \"message\": \"Random structured data\""
        echo "}"
        echo ""
        seq 1 1000 | awk '{printf "log entry %d: random text %s\n", NR, rand()}'
        echo ""
        for j in {1..5}; do
            echo "col1,col2,col3"
            seq 1 100 | awk -v j="$j" '{printf "%d,%f,%s\n", NR, rand(), "sample"}'
        done
    } | head -c "$size_bytes" > "$fpath"

    echo "Generated: $fpath ($(du -h "$fpath" | cut -f1))"
}

gen_mixed_data() {
    local fpath="$1"

    local size_mb=$(( (RANDOM % 2048) + 1 ))  # random size: 1–2048 MiB

    echo "Generating $fpath (~${size_mb} MiB)..."

    {
        echo "# --- BEGIN MIXED DATA FILE ---"

        echo "# JSON"
        for ((j = 0; j < 3000; ++j)); do
            echo "{\"id\":$j,\"user\":\"user_$i\",\"score\":$((RANDOM % 1000)),\"active\":$((RANDOM % 2))}"
        done

        echo "# LOGS"
        for ((j = 0; j < 5000; ++j)); do
            printf "[%s] %s: user_%d %s\n" \
                "$(date -d "@$((1700000000 + j))" '+%Y-%m-%d %H:%M:%S')" \
                "$(shuf -n1 -e INFO WARN ERROR DEBUG)" \
                "$((RANDOM % 50))" \
                "$(shuf -n1 -e connected disconnected timeout failed retried)"
        done

        echo "# CSV"
        echo "id,value,status"
        for ((j = 0; j < 1500; ++j)); do
            echo "$j,$RANDOM,$(shuf -n1 -e ok fail retry)"
        done

        echo "# CODE"
        for ((j = 0; j < 2000; ++j)); do
            echo "int func_$j(int x) { return x * $((j + 1)); }"
        done

        echo "# TEXT"
        for ((j = 0; j < 3000; ++j)); do
            echo "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua."
        done

        echo "# --- END MIXED DATA ---"

        # pad with base64 zeros for size target
        base64 /dev/zero | head -c "$((size_mb * 1024 * 1024))" || true
    } >"$fpath"

    echo "Generated: $fpath ($(du -h "$fpath" | cut -f1))"
}

echo "📦 Generating $SHARED_COUNT shared files..."
mkdir -p "$ROOT_DIR/shared"
for ((i=1; i<=SHARED_COUNT; i++)); do
    gen_mixed_data "$ROOT_DIR/shared/shared_file_$i.txt"
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
	local size_bytes=$(( (RANDOM % 2000) + 1 ))

	# 50% chance to use a shared file
	if (( RANDOM % 2 == 0 )) && (( SHARED_LEN > 0 )); then
	    local shared_index=$(( RANDOM % SHARED_LEN ))
	    cp "${SHARED_FILES[$shared_index]}" "$fpath"
	    echo "    • Copied shared file to $fpath" >&3
	    continue
    	else
	    echo "    • Creating new random file $fpath" >&3
	    gen_mixed_data "$fpath"
	fi
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

