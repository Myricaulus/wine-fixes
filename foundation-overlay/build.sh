#!/bin/bash
# Builds overlay-durchklick.exe with clang + lld-link, without MinGW and without a CRT.
set -euo pipefail
cd "$(dirname "$(readlink -f "$0")")"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT

cat > "$tmp/kernel32.def" <<'EOF'
LIBRARY kernel32.dll
EXPORTS
GetCommandLineW
CreateProcessW
WaitForSingleObject
GetExitCodeProcess
ExitProcess
GetModuleFileNameW
CreateFileW
WriteFile
FlushFileBuffers
GetLastError
EOF
cat > "$tmp/user32.def" <<'EOF'
LIBRARY user32.dll
EXPORTS
EnumWindows
EnumChildWindows
GetClassNameW
GetWindowThreadProcessId
GetWindowLongPtrW
SetWindowLongPtrW
wsprintfW
EOF
for d in kernel32 user32; do
    llvm-dlltool -m i386:x86-64 -d "$tmp/$d.def" -l "$tmp/$d.lib"
done

clang --target=x86_64-pc-windows-msvc -O2 -ffreestanding -fno-builtin -fno-stack-protector \
      -mno-stack-arg-probe -Wall -Wextra -c overlay-durchklick.c -o "$tmp/overlay-durchklick.obj"
lld-link /nologo /entry:start /subsystem:windows /nodefaultlib /out:overlay-durchklick.exe \
         "$tmp/overlay-durchklick.obj" "$tmp/kernel32.lib" "$tmp/user32.lib"
echo "gebaut: $(pwd)/overlay-durchklick.exe"
