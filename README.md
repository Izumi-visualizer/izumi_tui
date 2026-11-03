# izumi_tui (aka Izumi)

Izumi is a multi-format instruction pipeline dump TUI visualizer based on [shioyadan/Konata](https://github.com/shioyadan/Konata). Izumi is the reference frontend for [Izumi-visualizer/libizumi](https://github.com/Izumi-visualizer/libizumi), which provides the required parsing capabilities.

> [!Note]
> The tool is still under development and may not work properly with full
> features of the format.

## Supported features

- Instruction

    Only support for file identificator
- Stage
- Multiple Kanata lanes in a single trace, displayed as stacked subrows
- Line on left
- End of stage (for multiple cycle stages/stalling the pipeline)
- End of instruction

## Requirements

- `ncurses`
- `libizumi` (can be built alongside `izumi_tui`)
- `make` (build only)
- `autotools` (build from source only, not needed in distribution packages)

## Building (and installing)

If you cloned the repository instead of downloading a distribution package,
start by generating the build system:

```sh
autoreconf --install
```

With the `configure` script available, you can follow the traditional mantra:

```sh
mkdir build && cd build       # Any path supported!
../configure
make
(sudo) make install
```

Some intersting `configure` flags:
- `--without-system-libizumi` to build libizumi alongside izumi_tui (libizumi sources must be placed or linked in a `libizumi` directory next to the `configure` script)
- `--enable-sanitizers` to enable ASAN (not recommended if you aren't actively developing Izumi!)
- `--help` to see more options!

### Nix derivation

You can also use `nix` (flakes recommended):
```bash
nix run github:Izumi-visualizer/izumi_tui
```

## Usage

Use the installed `izumi` binary...
```sh
izumi path/to/file
```
... or use the `make` wrapper:
```sh
make run ARGS=path/to/file
```

> [!IMPORTANT]
> If sanitizers are enabled, an `AddressSanitizer:DEADLYSIGNAL` error can be raised randomly when starting the program. Just kill the program and try again. If it persists, please report it.

### Commands

| Command | Description |
|---------|-------------|
| `:q`    | Quit        |
| `:quit` | Quit        |
| `:open (path)`    | Open a file on the current panel |
| `:o (path)`    | Open a file on the current panel |
| `:findpc 0x(address)` | Search and go to the first appearance of the address |
| `:findinst (instruction)` | Search and go to the first appearance of the instruction |
| `:next` | Next appearance of the last search |
| `:prev` | Previous appearance of the last search |
| `:newpanel` | Create a new panel |
| `:panelcmd j` | Select next panel |
| `:panelcmd k` | Select previous panel |
| `:closepanel` | Close focused panel |
| `:c` | Close focused panel |
| `:closepanel (index)` | Close requested panel |
| `:c (index)` | Close requested panel |
| `:closeallpanels` | Close all panels |
| `:ca` | Close all panels |
| `:set bar_offset (amount)` | Set offset of the vertical bar (not persistent) |
| `:set stage_width (amount)` | Set width of the stage on the pipeline visualizer (not persistent) |
| `:toggleemptyrows` | Toggle empty instruction rows across all panels |
| `:set show_empty_rows on\|off` | Show empty instruction rows or hide them (default: `on`) |
| `:set color (element) (foreground color) (background color) <bold>` | Set color of the element (not persistent) |
| `:panelsync` | Sync movement of all the panels |
| `:paneldesync` | Desync movement of all the panels |

> [!NOTE]
> The `:set` commands are not persistent, so you will need to set them again on the next run.

Empty rows include instruction-ID slots with no instruction and instructions
with no recorded pipeline stages. Showing them preserves the original ID-space
spacing; hiding them packs the remaining rows together. Instruction IDs and
trace data are never renumbered or modified. When empty rows are hidden, `j`/`k`
and their numeric prefixes move through visible instructions. The status bar
shows the current setting. You can also put `set show_empty_rows on` or
`set show_empty_rows off` in your Izumi configuration file.

This toggle cannot create rows for instructions absent from the trace's ID
space: a file containing only sequential IDs for accepted FPU operations has
no gaps to reveal.

> [!IMPORTANT]
> Information for the `:set color` command
>
> The `<element>` option can be one of the following:
> - `text` Sets the color of the text shown on the screen
> - `commands` Sets the color of the commands shown on the bottom of the screen when typing one
> - `box` Sets the color of the box around the window
> - `status` Sets the color of the status of the application at the bar on the bottom of the screen
> - `error_text` Sets the color of the text shown on errors
> - `timeline` Sets the color of the timelines generated
> - `stage_1` Sets the color of the first stage of the pipeline
> - `stage_2` Sets the color of the second stage of the pipeline
> - `stage_3` Sets the color of the third stage of the pipeline
> - `stage_4` Sets the color of the fourth stage of the pipeline
> - `stage_5` Sets the color of the fifth stage of the pipeline
> - `stage_6` Sets the color of the sixth stage of the pipeline
> If there are more than six stages, the color of the seventh and following stages will return to the color of the first stage and repeat the cycle.
>
> It requires the color to be one of the list:
> - `black`
> - `white`
> - `red`
> - `green`
> - `yellow`
> - `blue`
> - `magenta`
> - `cyan`
>
> The `<bold>` option is optional. If you want to set a color to bold, just add `bold` at the end of the command.

### Keybindings

| Key | Description |
|-----|-------------|
| `n` | Next appearance of the last search |
| `N` | Previous appearance of the last search |
| `j` | Move down |
| `KEY_DOWN` | Move down |
| `k` | Move up |
| `KEY_UP` | Move up |
| `J` / `K` | Scroll down/up by one screen row, including within a tall instruction |
| `PageDown` / `PageUp` | Scroll down/up by a page of screen rows |

### Multiple lanes

Kanata `S` and `E` records identify their lane with the second argument after
the instruction ID. Izumi displays all active lanes of an instruction on
separate stacked subrows, on one shared cycle axis. Lane IDs are shown next to
the subrows and sorted numerically; gaps in the lane numbers do not reserve
blank lines. Lane 1 (commonly stalls) is stacked like any other lane.

An instruction with multiple active lanes has one header showing its instruction
ID, PC and label, followed by `Lane 0`, `Lane 1`, etc. Only lanes with a positive
duration or an unfinished stage are shown. Single-lane instructions retain the
existing two-line layout. `:set stage_width <n>` controls stage-name truncation
as before. Instruction IDs are not renumbered, and `:toggleemptyrows` still
controls entire empty instruction rows independently.

`j`/`k` navigate by instruction, while `J`/`K` and PageDown/PageUp let you
reach lower lanes of an instruction taller than the panel. Numeric prefixes
work with `J`/`K`. Searching or opening a trace resets the within-instruction
scroll offset. Resizing and timelines clip lane content to the available panel
area. This feature handles lanes already present in one Kanata file; attaching
separate trace files to a panel is not supported yet.

## Screenshot

![Screenshot](.github/screenshot.png)

I know it's not the best looking tool, but it's a start (now it has colors :D).

---

![Izumi Konata](https://external-content.duckduckgo.com/iu/?u=https%3A%2F%2Fstatic.zerochan.net%2FIzumi.Konata.full.955876.jpg&f=1&nofb=1&ipt=11ebd39eb2229bec63db528410089c03d01dd9a541df01063ee5c809b6c69f58&ipo=images)
