# Chroma

Chroma is a WIP Game Boy [Color] emulator written in C++. It has currently only been tested on Linux, however support for other operating systems is planned.

Chroma is currently capable of playing a number of commercial games quite well, but the emulator is still at a very experimental stage and many things will not work correctly. In particular, Chroma does not yet have support for audio.

Chroma is licensed under the GPLv3.

### Dependencies
At the moment, Chroma's only dependency is SDL2.

* Linux: Install SDL2 from your distribution's repositories.
* macOS: Install SDL2 through homebrew: `brew install sdl2`.
* Windows: Download SDL2 from their [website](https://www.libsdl.org/download-2.0.php).

### Building
A reasonably recent compiler is required to build Chroma (at least GCC 5.x or Clang 3.4).

`mkdir build`

`cd build`

`cmake ..`

`make`


### Controls

| Button     | Key        |
| ---------- | ---------- |
| Up         | W          |
| Left       | A          |
| Down       | S          |
| Right      | D          |
| A          | K          |
| B          | J          |
| Start      | Return/I   |
| Select     | U          |

| Command    | Key        |
| ---------- | ---------- |
| Quit       | Escape/Q   |
| Pause      | P          |
| Fullscreen | V          |
| Screenshot | T          |
