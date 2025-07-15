This folder contains the perft testing tool, which compares our move generation to known results for debugging purposes.
There are two ways to run it: providing a test suite in epd format (see the example in this directory), or interactively
using [perftree](https://github.com/agausmann/perftree), which compares the perft results to Stockfish. The former is
good for CI/testing, and the latter is helpful for debugging.