# CEleste Speedrun

Celeste Classic for TI-84 Plus CE and TI-83 Premium CE, with an always-visible
timer, pause, quick reset, saved personal bests and split comparisons.
Based on [commandz0/CEleste](https://github.com/commandz0/CEleste).

![Original port screenshot](screenshot.png)

## Controls

| Key | Action |
| --- | --- |
| Arrows | Move |
| 2nd | Jump |
| Alpha | Dash |
| Y= | Pause / resume |
| Del | Reset to room one, preserving PBs |
| Clear | Save progress and PBs, then quit |

Pause freezes the game, hitstop, animation, timer and split popup. A small
`paused` label appears at the top right. Press Y= again to continue from the
same frame. Del also works while paused. Holding either key triggers only once.
Enter is no longer the reset key.

The timer reads `MM:SS.mmm` using the original 30 Hz simulation clock, so its
millisecond display advances in 33/34 ms steps. Paused time is excluded from PBs.
This timing convention is documented for anyone comparing runs.

PBs and their cumulative room splits are archived. A small green minus or red
plus briefly shows the difference from the same room in your completed PB.
Player movement, collision, jump/dash behavior and the original tick limiter
are retained. No measured performance gain is claimed.

## Install and build

Download the install ZIP from [Releases](https://github.com/silo555/celeste-ce-upgraded/releases),
then follow [INSTALL.txt](INSTALL.txt). The program name remains `CELESTE` and
replaces an older copy of that program; back up your saves first.

For compilation, automated verification and remaining device checks, see
[docs/BUILDING.md](docs/BUILDING.md). Native compilation and host regression
tests cover the control loop and record storage. Calculator and emulator
gameplay testing remain pending.

## Built-in practice mode

Press Graph on the title screen. Rooms loop after completion. Trace selects the
previous room, Graph the next, Window toggles the HUD and Zoom toggles gemskip.
Y= pauses here too; Del returns to a fresh normal run. The previous-room binding
moved from Y= to Trace to make room for pause.

## Credits

Celeste Classic by Maddy Thorson and Noel Berry. C++ calculator port by John
"commandblockguy" Cesarz. The upstream history and MIT license are preserved.
See [LICENSE](LICENSE) and [THIRD_PARTY.md](THIRD_PARTY.md).
