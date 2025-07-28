# ncurses-menu

A simple ncurses-based menu for the terminal

## Features
- Menu title with line wrapping support (`-t <title>`, use `\n` for new lines)
- Add options via `-o <option>` (can be used multiple times)
- Load options from a file (`-f <file>`, one option per line)
- Auto-refresh options from file every 400 ms (`-s`)
- Keyboard controls: arrows, PgUp/PgDn, Enter (select), Esc/q/Q (exit)

## Build

```sh
make                # dynamic build
make static         # static build
make static-small   # optimized static build
make static-packed  # static + UPX
make test           # test
make clean          # clean
```

## Usage

```sh
./ncurses-menu [OPTIONS]
```

### Options

- `-t <title>`      — menu title (supports `\n` for line breaks)
- `-o <option>`     — add a menu item (can be used multiple times)
- `-f <file>`       — load menu items from a file (one per line)
- `-s`              — auto-refresh options from file every 400 ms
- `-h`, `--help`    — show help

### Example usage

```sh
./ncurses-menu -t "Choose an action\n(arrows to navigate)" -o "Option 1" -o "Option 2" -o "Option 3"
./ncurses-menu -t "Menu" -f options.txt
./ncurses-menu -t "Menu" -f options.txt -s
```

### Example options.txt file

```
First option
Second option
Third option
```

## Controls
- <key>Up</key>/<key>Down</key> arrows — move between items
- <key>PgUp</key>/<key>PgDn</key> — scroll
- <key>Enter</key> — select
- <key>Esc</key>, <key>q</key>, <key>Q</key> — exit

## License
See LICENSE
