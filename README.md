# Gideon's Chess Engine

This is a chess engine I'm developing to learn more about chess and C programming. GCE ships with a graphical user
interface, and is compatible with the Universial Chess Interface protocol so you can use it in your favorite software (
cutechess, Banskia, Arena, etc).

The engine's core is written in C99, and the various interfaces and extraneous tools are written in C++. It currently
features bitboard-based move generation, alpha-beta pruning, a transposition table, a basic eval function, and extremely
rudimentary time management. The GUI interface supports Polyglot opening books.

The project builds using CMake. An [Emscripten version](https://gideongrinberg.github.io/gce/) of the GUI is available,
although the graphics are a bit blurry due to limitations of WebGL. Executables of the UCI and GUI interfaces are
available in the releases tab. They are built for Windows, Linux, and Mac (Intel/Silicon) using Github Actions. 