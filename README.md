# KLTI

[English](README.md) | [Українська](README.uk.md)

**KLTI (Keyboard Layout Tray Icon)** is a lightweight Windows system-tray utility that displays the icon associated with the currently active keyboard layout.

## Motivation

The standard Windows keyboard layout indicator is text-based, which can make it difficult to notice the currently active layout during practical work, especially when multiple keyboard input layouts are installed.

A visual indicator can make the active layout much easier to recognize at a glance. However, predefined visual symbols are not always an appropriate or desirable choice. Language and keyboard layout indicators often use country flags, for example, but a country flag does not necessarily represent the language or layout itself.

KLTI therefore uses user-provided icons and does not impose any particular visual representation. The indicator can be a country flag, another image, or even a plain color chosen to be easily noticeable in peripheral vision. This allows the user to choose a representation that best fits their preferences or use case.

## Description

KLTI identifies the active keyboard layout by its Windows **LANGID** and looks for the corresponding icon in the `icons` subdirectory of the directory containing the KLTI executable:

`<KLTI executable directory>/icons`

For example, if the installed layouts have the following LANGIDs:

* `0x0409` — English (United States)
* `0x0422` — Ukrainian
* `0x0804` — Chinese (Simplified, PRC)

KLTI looks for the corresponding icon files using two naming conventions.

The preferred format is hexadecimal, using exactly four hexadecimal digits: `0x0409.ico`, `0x0422.ico`, `0x0804.ico`.

A decimal filename is also supported as a fallback: `1033.ico`, `1058.ico`, `2052.ico`.

If both formats exist, the hexadecimal filename takes precedence.

The special `unknown.ico` icon is used as a fallback when no icon is available for the active LANGID.

KLTI checks the icons for installed keyboard layouts when it starts and reports missing icons. At runtime, if the icon for the current layout cannot be found, `unknown.ico` is used instead.

## Building

### Requirements

* Windows
* x64
* C++17
* Visual Studio 2022 Build Tools with MSVC v143
* ImageMagick, available as `magick.exe` in `PATH`

The project does not require third-party runtime libraries. The code intentionally makes limited use of the C++ standard library to keep the executable small. The resulting Release build is approximately **18 KB**, or approximately **24 KB with the application icon embedded**.

### Build commands

For a normal build:

```text
build.bat Res
build.bat
```

`Res` generates the required `*.ico` files and must be run before building Debug or Release.

`build.bat Res`

: Generate `*.ico` files. **Required** before Debug or Release builds.

`build.bat`
: Build the Release version and populate `dist/Release`.

`build.bat Debug`
: Build the Debug version.

The `Res` command converts the SVG source assets into ICO files. It does not require Visual Studio.

A normal build does not generate resources automatically. If the required `build/resources/klti.ico` file is missing, the build instructs the user to run `build.bat Res` first.

### Icons and assets

The SVG source files used to create the icons are **not published in this repository**.

The source assets are expected in the following locations:

```text
assets/
  klti.svg
  unknown.svg
  layout-icons/
    0x0409.svg
    ...
```

The layout icon filenames correspond to Windows LANGIDs. For example, `0x0409.svg` is used to generate the icon for LANGID `0x0409`.

`build.bat Res` generates:

* layout icons from `assets/layout-icons/*.svg` into `dist/Release/icons/`;
* `unknown.ico` from `assets/unknown.svg` into `dist/Release/icons/`;
* `klti.ico` from `assets/klti.svg` into `build/resources/`.

The `klti.ico` file is embedded into the application executable. The other ICO files are runtime resources and are loaded from the `icons` directory next to the executable.

The repository does **not** publish the SVG artwork required to generate these resources. In particular, the following are intentionally not included:

* the SVG source for the application icon (**Required for build**);
* the SVG source for `unknown.ico` (**Required for run**);
* the SVG files used for the keyboard-layout icons (**Required for meaningful run**).

An application icon is required to compile the application, and a complete set of layout icons is required for full functionality. Users must therefore provide or create the required SVG assets themselves before running `build.bat Res`.

## License

This project is licensed under the GNU General Public License, version 3 or later (GPL-3.0-or-later).
