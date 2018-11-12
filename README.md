# FreeChaF
FreeChaF is a libretro emulation core for the Fairchild ChannelF / Video Entertainment System designed to be compatible with joypads from the SNES era forward.

## Authors

FreeIntv was created by David Richardson.

## License
The FreeIntv core is licensed under GPLv3.

## BIOS
FreeChaF requires two BIOS files to be placed in the libretro 'system' folder:

|Filename | Description |
|---|---|
|sl31253.bin | ChannelF BIOS (PSU 1)|
|sl31254.bin | ChannelF BIOS (PSU 2)|

If the ChannelF II BIOS is included, it will be used instead of sl31253.  All games are compatible with both.

|Filename | Description |
|---|---|
|sl90025.bin | ChannelF II BIOS (PSU 1)|

* BIOS filenames are case-sensitive

## Console button overlay
Access to the console buttons is provided via an overlay.  Pressing 'start' on either controller will display the console buttons.  You can select a button by moving left and right and press the button with any of the face buttons (A, B, X, Y).  Pressing 'start' a second time will hide the overlay.

## Controls
* **Console Overlay** - allows the user to view and select console buttons.
* **Controller Swap** - Controller Swap swaps the player 1 and player 2 controllers.

| FreeChaF Function | Retropad |
| --- | --- |
|Forward| D-Pad Up, Left-Analog Up|
|Backward| D-Pad Down, Left-Analog Down|
|Rotate Left | Y, L, Right-Analog Left |
|Rotate Right | A, R, Right-Analog Right |
|Pull Up | X, Right-Analog Up |
|Push Down | B, Right-Analog Down |
|Show/Hide Console Overlay | Start |
|Controller Swap | Select |

