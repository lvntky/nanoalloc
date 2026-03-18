#!/usr/bin/env bash

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# default .so path
LIB_PATH="$SCRIPT_DIR/build/libnanoalloc.so"

# check if .so exists
if [[ ! -f "$LIB_PATH" ]]; then
    echo "Error: $LIB_PATH not found. Build the project first."
    exit 1
fi

# ask user for command
read -p "Enter the command to run under na_alloc (e.g. ls /usr/bin, python3 -c 'print(123)'): " USER_CMD

# check if empty
if [[ -z "$USER_CMD" ]]; then
    echo "No command entered. Exiting."
    exit 1
fi

# optional flags
export NA_STATS=1

echo
echo "Running: LD_PRELOAD=$LIB_PATH $USER_CMD"
echo "---------------------------------------"
LD_PRELOAD="$LIB_PATH" $USER_CMD
echo "---------------------------------------"
echo "Done."
