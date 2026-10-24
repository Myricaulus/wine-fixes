# foundation-overlay

Fixes **mouse clicks not working** in **Foundation: Galactic Frontier** (Demo, Steam AppID 5143870) under Proton / Wine.

## Symptom

After the loading screen, clicks are ignored in a large part of the window (e.g. you can't pick
"Rescue the crew" / "Retrieve the cargo"). The cursor moves and the keyboard works. Which part is
blocked changes when you move or resize the window; in fullscreen almost everything is blocked.

## Cause

The Funplus SDK in the game (`FPX.dll`, Qt 5.15.9) puts an almost fully transparent Qt window
(`Qt5159QWindow`, title "launcher") on top of the game as a child window and draws it with
`UpdateLayeredWindow` (per-pixel alpha). Windows lets clicks pass through transparent pixels of such
a window, Wine doesn't do alpha hit-testing – so the overlay swallows every click in its rectangle.

Shown with `WINEDEBUG=+timestamp,+tid,+win`: `window_from_point` returns the Qt window instead of
the game window.

## Fix

`overlay-durchklick.exe` starts the game and adds `WS_EX_TRANSPARENT` to the game's layered Qt
windows (checked every 0.5 s until the game exits). Wine skips `WS_EX_LAYERED | WS_EX_TRANSPARENT`
windows when hit-testing. The overlay is still drawn but no longer catches clicks.
Nothing in the game files or the Wine prefix is changed.

### Install

1. Download `overlay-durchklick.exe` and `start.sh` from the
   [latest release](../../releases/latest) (or build it, see below) into one folder, e.g.
   `~/foundation-overlay/`.
2. `chmod +x ~/foundation-overlay/start.sh`
3. Steam → game → Properties → General → Launch Options:

   ```
   ~/foundation-overlay/start.sh %command%
   ```

   Use the full path if `~` doesn't work for you, e.g. `/home/<user>/foundation-overlay/start.sh %command%`.

Tested with GE-Proton10-34 on KDE Plasma 6.7 (Wayland), works in fullscreen.

Put `OVERLAY_FIX=0` in front to disable it for one launch, e.g. if an SDK dialog (login) has to be
clicked. A log is written to `overlay-durchklick.log` next to the exe.

## Build

```
./build.sh
```

Needs only `clang`, `lld-link` and `llvm-dlltool` (no MinGW, no CRT).

## License

MIT
