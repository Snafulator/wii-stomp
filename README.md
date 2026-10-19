# wii-stomp

**A Wii Balance Board driver for Linux and Steam: bind any key or button to a foot bash.**

Bash your left or right foot down on the board and wii-stomp presses whatever you've bound to it: a keyboard key, a mouse button, or a controller button. Lean on the board to move, and use its power button as one more input. It works alongside your normal controls:

- **Add foot controls to keyboard-and-mouse games:** bash to jump, dodge or reload while your hands stay on the keyboard and mouse.
- **On a Steam Deck,** use the board in place of the awkward back buttons.

### Built for Steam

- **Steam Input:** with an xbox profile, Steam sees the board as a standard **Xbox 360 Controller**. You can remap it per game in Steam's controller settings, like any other controller.
- **Gamescope and the Steam Deck:** with a kbm profile, the board presses real **keyboard keys and mouse buttons**. The profile sets which keys it presses, so it works well with gamescope without any Steam Input remapping.
- **Nothing extra to install:** it runs on the `hid-wiimote` driver built into Linux and the device permissions Steam already sets up, so on SteamOS and Bazzite there's nothing else to install.

See [docs/steam.md](docs/steam.md) for setup.

## What it does

| Board input | What happens |
|---|---|
| **Lean** forward/back/left/right | moves the Xbox controller's left stick (xbox profiles) |
| **Bash** your left or right foot down | presses whatever the profile binds to that side |
| **Power button** (front of the board) | toggles lean on/off, or presses a bound key/button |

A bash is detected anywhere on that half of the board (toe or heel), and it's told apart from leaning or shifting your weight. `calibrate.py` tunes this to how you bash and move.

## Quick start

1. **Connect the board over Bluetooth.** This is the fiddliest part; see [docs/bluetooth.md](docs/bluetooth.md).
2. **Build:**
   ```sh
   make
   ```
   You need a C compiler and `make`. On a Steam Deck, see [docs/steam.md](docs/steam.md#steam-deck).
3. **Run it with a profile:**
   ```sh
   ./wii-stomp -p xbox        # Xbox controller: lean = left stick, bashes = LB/RB
   ./wii-stomp -p darktide    # keyboard/mouse: bashes = Space / mouse 5
   ```
   It finds the board by itself. Put your feet on the board and keep still for about 2 seconds while it measures your resting balance.
4. **Calibrate** (optional, but bash detection works much better):
   ```sh
   ./calibrate.py
   ```

To install it for your user (to `~/.local`), run `make install`, then use `wii-stomp` from anywhere.

## Profiles

A profile says which virtual device to create and what each board input does. Pick one with `-p name`. Profiles are looked up in `~/.config/wii-stomp/profiles/` first, then in the `profiles/` folder that comes with the program, so you can copy one there and change it. You can also pass a path to a `.json` file.

```json
{
	"name": "wii-stomp - KBM",
	"description": "Keyboard/mouse for Darktide: left bash presses Space, right bash presses mouse 5",
	"device": "kbm",
	"lean": false,
	"bash": { "left": "SPACE", "right": "MOUSE5" },
	"button": "none"
}
```

| Field | Meaning |
|---|---|
| `name` | shown in wii-stomp's output when it starts (defaults to what you passed to `-p`) |
| `description` | optional notes on what the profile is for; wii-stomp ignores it |
| `device` | `"xbox"` creates a virtual Xbox 360 controller; `"kbm"` creates a virtual keyboard and mouse |
| `lean` | whether leaning moves the stick at start (xbox only; defaults to `true`) |
| `bash.left`, `bash.right` | what a bash on that side does |
| `button` | what the board's power button does; a bound key is held for as long as you hold the button |
| `settings` | optional overrides of any [setting](#settings), just for this profile |

Anything a profile leaves out does nothing. Each binding is one of:

- `"none"`
- `"toggle_lean"`: turns lean movement on/off (xbox only)
- **xbox:** `PAD_A` `PAD_B` `PAD_X` `PAD_Y` `LB` `RB` `BACK` `START` `GUIDE` `LS` `RS`
- **kbm:** `MOUSE1`–`MOUSE5`, or a key name: Linux key names without `KEY_` (case doesn't matter), e.g. `SPACE` `G` `1` `F5` `ENTER` `TAB` `LEFTSHIFT`; `SHIFT` `CTRL` `ALT` mean the left-hand ones

Mixing devices in one profile (e.g. a key in an xbox profile) is an error, so each profile clearly uses one device. With no `-p`, it behaves like `profiles/xbox.json`.

## Settings

Tuning values live in `settings.json` files, layered so later ones win:

1. built-in defaults (`wii-stomp -d` prints them)
2. `settings.json` next to the program
3. `~/.config/wii-stomp/settings.json` (where `calibrate.py` saves)
4. a file given with `-s file.json`
5. the profile's `settings` block

Each file only needs the values it changes.

| Setting | Default | Meaning |
|---|---|---|
| `x_mult` / `y_mult` | 9 / 3 | left/right and forward/back sensitivity (negative = inverted) |
| `x_deadzone` | 100 | left/right output is ignored below this |
| `fwd_on` / `fwd_off` | 600 / 400 | forward goes full tilt once lean passes ON, and back to centre below OFF |
| `back_on` / `back_off` | 1500 / 1100 | same for leaning back |
| `settle_ms` / `calib_ms` | 700 / 1000 | after your feet go down: ignore this long, then measure resting balance for this long |
| `min_weight` | 400 | total weight below this counts as feet off (an empty board reads ~90) |
| `bash_window_ms` | 150 | a bash's jump has to happen within this long (max 500) |
| `bash_rise` | 400 | how much one side must jump to count as a bash |
| `bash_margin` | 300 | ...and how much more than the other side, so shifting weight doesn't count |
| `bash_jolt` | 40 | the other side must jump at least this much too (a bash jolts the whole board; leaning takes weight off it) |
| `bash_hold_ms` | 100 | how long a bash holds its key/button down |
| `bash_cooldown_ms` | 300 | after a bash, that side ignores bashes for this long |

## Calibration

```sh
./calibrate.py              # lean and bash
./calibrate.py --only bash  # just one part
```

It walks you through some short actions: feet off, sitting still, holding each lean, 5 bashes per side, then moving around without bashing. From those it works out the settings, shows each change as old → new, and asks before saving them to `~/.config/wii-stomp/settings.json`. Calibrate in the position you'll play in (seated or standing), and bash the way you will in game.

Each run is saved to `calibration-recording.json`, and `./calibrate.py --replay calibration-recording.json` reruns the analysis without the board. That's useful when reporting a problem.

Stop any running `wii-stomp` first, or it will press keys while you calibrate.

## Using it with Steam

See [docs/steam.md](docs/steam.md) for Steam Input, gamescope and the Steam Deck.

## Command line

```
wii-stomp [-p profile] [-s settings.json] [/dev/input/eventN]
  -p  profile name or .json path
  -s  extra settings file
  -r  print raw sensor readings instead of creating devices (calibrate.py uses this)
  -d  print the built-in default settings as JSON
```

The board's device is found automatically. Pass it only if you have more than one board.

It prints what it's doing to the terminal: which settings and profile it loaded, when your feet go on and off, and each detected bash with its strength, e.g. `left bash (rise 812, other 140)`. That's handy when tuning settings by hand.

## Permissions

wii-stomp needs to read the board (`/dev/input/event*`) and create devices (`/dev/uinput`). If Steam is installed, its udev rules usually give your user both already. If you get "Permission denied" on `/dev/uinput`, add a udev rule like Steam's:

```sh
echo 'KERNEL=="uinput", SUBSYSTEM=="misc", TAG+="uaccess", OPTIONS+="static_node=uinput"' \
  | sudo tee /etc/udev/rules.d/60-wii-stomp.rules
sudo udevadm control --reload && sudo udevadm trigger
```

See also the [Arch wiki](https://wiki.archlinux.org/title/Wiimote#Unable_to_open_uinput).

## Project layout

| Path | What it is |
|---|---|
| `src/main.c` | command line and main loop |
| `src/board.c` | finds the board, reads its sensors, feet on/off, resting-balance calibration, lean → stick |
| `src/bash.c` | bash detection |
| `src/output.c` | the virtual Xbox controller and keyboard/mouse devices |
| `src/config.c`, `src/json.c` | settings and profile loading |
| `calibrate.py`, `wii_stomp_tools/` | the calibration script and its helpers: reading the board, settings files, prompts, lean and bash analysis |
| `profiles/` | the profiles that come with the program |

## Credits

This project builds on the work of others:

- **[g-rden/wbb-uinput](https://github.com/g-rden/wbb-uinput)**: the original wbb-uinput, which turned the balance board into a uinput controller. This project is a fork of it.
- **[keldu/WiiWeight](https://github.com/keldu/WiiWeight)** (GPL-3.0): reading the board's four weight sensors through the `hid-wiimote` driver.
- **[GrantEdwards/uinput-joystick-demo](https://github.com/GrantEdwards/uinput-joystick-demo)**: creating a virtual joystick with the Linux uinput API.

## License

[GPL-3.0](LICENSE). wii-stomp is based on WiiWeight, which is GPL-3.0, so this project is too. You're free to use, change and share it, as long as anything you distribute based on it is also GPL-3.0, with its source available.
