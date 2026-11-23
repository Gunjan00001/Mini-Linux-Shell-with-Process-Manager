#!/usr/bin/env bash
# Mini Linux Shell installer.
#
#   curl -fsSL https://raw.githubusercontent.com/Gunjan00001/Mini-Linux-Shell-with-Process-Manager/main/install.sh | bash
#   curl -fsSL .../install.sh | bash -s -- --uninstall
#
# Clones the repository into ~/.local/src, builds it, and puts the binary in
# ~/.local/bin. Nothing outside your home directory is touched and sudo is
# never used.
set -euo pipefail

REPO_URL="https://github.com/Gunjan00001/Mini-Linux-Shell-with-Process-Manager.git"
SRC_DIR="${HOME}/.local/src/mini-linux-shell"
BIN_DIR="${HOME}/.local/bin"
BIN_PATH="${BIN_DIR}/minishell"

usage() {
    cat <<'EOF'
Mini Linux Shell installer

Usage:
  install.sh              install or update
  install.sh --uninstall  remove the binary and the source checkout
  install.sh --help       this text

Installs to:
  ~/.local/src/mini-linux-shell   source checkout
  ~/.local/bin/minishell          the shell
EOF
}

UNINSTALL=0
for arg in "$@"; do
    case "$arg" in
        --uninstall) UNINSTALL=1 ;;
        -h|--help) usage; exit 0 ;;
        *)
            echo "unknown option: $arg" >&2
            usage >&2
            exit 2
            ;;
    esac
done

if [ "$(uname -s)" != "Linux" ]; then
    cat >&2 <<'EOF'
error: this shell uses Linux system calls, so it builds only on Linux.

  Windows users: install WSL2 and run this inside Ubuntu
    wsl --install -d Ubuntu
  macOS: not supported (no fork/exec/waitpid semantics; use a VM or container)
EOF
    exit 1
fi

if [ "$UNINSTALL" -eq 1 ]; then
    rm -f "$BIN_PATH"
    rm -rf "$SRC_DIR"
    echo "removed $BIN_PATH"
    echo "removed $SRC_DIR"
    exit 0
fi

missing=0
for tool in git cmake; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "missing dependency: $tool" >&2
        missing=1
    fi
done
if ! command -v c++ >/dev/null 2>&1 &&
    ! command -v g++ >/dev/null 2>&1 &&
    ! command -v clang++ >/dev/null 2>&1; then
    echo "missing dependency: a C++17 compiler (g++ or clang++)" >&2
    missing=1
fi
if [ "$missing" -ne 0 ]; then
    cat >&2 <<'EOF'

Install them first, for example on Ubuntu/Debian:
  sudo apt update && sudo apt install -y build-essential cmake git
EOF
    exit 1
fi

if [ -d "$SRC_DIR/.git" ]; then
    echo "updating $SRC_DIR"
    git -C "$SRC_DIR" pull --ff-only ||
        echo "warning: could not update (local changes?); building the existing checkout"
else
    mkdir -p "$(dirname "$SRC_DIR")"
    echo "cloning into $SRC_DIR"
    git clone --depth 1 "$REPO_URL" "$SRC_DIR"
fi

echo "building (this takes a few seconds)"
cmake -S "$SRC_DIR" -B "$SRC_DIR/build" -DCMAKE_BUILD_TYPE=Release >/dev/null
cmake --build "$SRC_DIR/build" -j"$(nproc 2>/dev/null || echo 2)" >/dev/null

mkdir -p "$BIN_DIR"
install -m 755 "$SRC_DIR/build/minishell" "$BIN_PATH"
echo "installed: $BIN_PATH"

case ":$PATH:" in
    *":$BIN_DIR:"*) ;;
    *)
        echo
        echo "Add this line to ~/.bashrc (or your shell profile), then reopen the terminal:"
        printf '  export PATH="%s:$PATH"\n' "$BIN_DIR"
        ;;
esac

cat <<EOF

Run it:
  $BIN_PATH

Uninstall:
  curl -fsSL https://raw.githubusercontent.com/Gunjan00001/Mini-Linux-Shell-with-Process-Manager/main/install.sh | bash -s -- --uninstall
EOF
