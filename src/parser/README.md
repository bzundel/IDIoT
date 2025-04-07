# IDIoT Parser

Parser for the IDIoT configuration file format.

## Run instructions

Make sure you have a working installation of `elixir` with the `mix` utilities. Run `mix deps.get` to install the required dependencies. Use `mix parse FILENAME` to parse an IDIoT file.

## To-Dos
- [ ] Replace maps with structs
- [x] Allow environment variables in IDIoT file
- [ ] Implement phony target
    - Should not be built as a RIOT target, but only act as a receiving target for communication
