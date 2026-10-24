# foundation-overlay

[Deutsch](#deutsch) · [English](#english)

---

## Deutsch

Behebt **Mausklicks, die nicht ankommen**, in **Foundation: Galactic Frontier** (Demo, Steam-AppID 5143870) unter Proton / Wine.

### Symptom

Nach dem Ladebildschirm werden Klicks in einem großen Teil des Fensters ignoriert (z. B. lässt sich
„Rescue the crew“ / „Retrieve the cargo“ nicht auswählen). Der Zeiger bewegt sich, die Tastatur geht.
Welcher Teil blockiert ist, ändert sich beim Verschieben oder Vergrößern des Fensters; im Vollbild ist
fast alles blockiert.

### Ursache

Das Funplus-SDK im Spiel (`FPX.dll`, Qt 5.15.9) legt ein fast vollständig transparentes Qt-Fenster
(`Qt5159QWindow`, Titel „launcher“) als Kindfenster über das Spiel und zeichnet es mit
`UpdateLayeredWindow` (Pixel-Alpha). Windows lässt Klicks an transparenten Pixeln eines solchen Fensters
durch, Wine macht kein Alpha-Hit-Testing – das Overlay schluckt also jeden Klick in seinem Rechteck.

Belegt mit `WINEDEBUG=+timestamp,+tid,+win`: `window_from_point` liefert das Qt-Fenster statt des
Spielfensters.

### Lösung

`overlay-durchklick.exe` startet das Spiel und setzt bei dessen layered Qt-Fenstern zusätzlich
`WS_EX_TRANSPARENT` (alle 0,5 s, bis das Spiel endet). Fenster mit `WS_EX_LAYERED | WS_EX_TRANSPARENT`
überspringt Wine beim Hit-Test. Das Overlay wird weiter gezeichnet, fängt aber keine Klicks mehr ab.
An Spieldateien und Wine-Prefix wird nichts geändert.

#### Einrichtung

1. `overlay-durchklick.exe` und `start.sh` aus dem
   [neuesten Release](https://github.com/Myricaulus/wine-fixes/releases/latest) herunterladen
   (oder selbst bauen, siehe unten) und in einen Ordner legen, z. B. `~/foundation-overlay/`.
2. `chmod +x ~/foundation-overlay/start.sh`
3. Steam → Spiel → Eigenschaften → Allgemein → Startoptionen:

   ```
   ~/foundation-overlay/start.sh %command%
   ```

   Falls `~` nicht klappt, den vollen Pfad nehmen, z. B. `/home/<benutzer>/foundation-overlay/start.sh %command%`.

Getestet mit GE-Proton10-34 unter KDE Plasma 6.7 (Wayland), funktioniert auch im Vollbild.

Mit `OVERLAY_FIX=0` davor ist die Umgehung für einen Start aus, etwa wenn ein SDK-Dialog (Login)
geklickt werden muss. Ein Log landet als `overlay-durchklick.log` neben der exe.

### Bauen

```
./build.sh
```

Braucht nur `clang`, `lld-link` und `llvm-dlltool` (kein MinGW, keine CRT).

### Lizenz

MIT

---

## English

Fixes **mouse clicks not working** in **Foundation: Galactic Frontier** (Demo, Steam AppID 5143870) under Proton / Wine.

### Symptom

After the loading screen, clicks are ignored in a large part of the window (e.g. you can't pick
"Rescue the crew" / "Retrieve the cargo"). The cursor moves and the keyboard works. Which part is
blocked changes when you move or resize the window; in fullscreen almost everything is blocked.

### Cause

The Funplus SDK in the game (`FPX.dll`, Qt 5.15.9) puts an almost fully transparent Qt window
(`Qt5159QWindow`, title "launcher") on top of the game as a child window and draws it with
`UpdateLayeredWindow` (per-pixel alpha). Windows lets clicks pass through transparent pixels of such
a window, Wine doesn't do alpha hit-testing – so the overlay swallows every click in its rectangle.

Shown with `WINEDEBUG=+timestamp,+tid,+win`: `window_from_point` returns the Qt window instead of
the game window.

### Fix

`overlay-durchklick.exe` starts the game and adds `WS_EX_TRANSPARENT` to the game's layered Qt
windows (checked every 0.5 s until the game exits). Wine skips `WS_EX_LAYERED | WS_EX_TRANSPARENT`
windows when hit-testing. The overlay is still drawn but no longer catches clicks.
Nothing in the game files or the Wine prefix is changed.

#### Install

1. Download `overlay-durchklick.exe` and `start.sh` from the
   [latest release](https://github.com/Myricaulus/wine-fixes/releases/latest) (or build it, see below)
   into one folder, e.g. `~/foundation-overlay/`.
2. `chmod +x ~/foundation-overlay/start.sh`
3. Steam → game → Properties → General → Launch Options:

   ```
   ~/foundation-overlay/start.sh %command%
   ```

   Use the full path if `~` doesn't work for you, e.g. `/home/<user>/foundation-overlay/start.sh %command%`.

Tested with GE-Proton10-34 on KDE Plasma 6.7 (Wayland), works in fullscreen.

Put `OVERLAY_FIX=0` in front to disable it for one launch, e.g. if an SDK dialog (login) has to be
clicked. A log is written to `overlay-durchklick.log` next to the exe.

### Build

```
./build.sh
```

Needs only `clang`, `lld-link` and `llvm-dlltool` (no MinGW, no CRT).

### License

MIT
