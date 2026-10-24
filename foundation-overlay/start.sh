#!/bin/bash
# Steam launch option:  /path/to/foundation-overlay/start.sh %command%
# Puts overlay-durchklick.exe in front of the first *.exe in the Proton command line,
# so it starts the game inside the same prefix. OVERLAY_FIX=0 disables it.
helper="$(dirname "$(readlink -f "$0")")/overlay-durchklick.exe"
if [[ ${OVERLAY_FIX:-1} == 0 || ! -f $helper ]]; then
    exec "$@"
fi
args=() done=0
for a in "$@"; do
    if (( !done )) && [[ $a == *.exe && $a != */steam.exe ]]; then
        args+=("$helper")
        done=1
    fi
    args+=("$a")
done
exec "${args[@]}"
