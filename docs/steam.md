# Using the balance board with Steam

wii-stomp can present the board to games in two ways. Pick the profile type that suits the game.

## Xbox controller profiles (`"device": "xbox"`)

Steam sees the board as an **Xbox 360 Controller**. Leaning moves its left stick, and bashes press the buttons the profile binds (`LB`/`RB` in `profiles/xbox.json`).

Because it looks like a normal controller, **Steam Input** can remap it per game: open the game's controller settings in Steam, choose the Xbox 360 Controller, and bind LB/RB (and the stick) to whatever you like, including keyboard keys. This is the most flexible option for anything launched through Steam.

To check that Steam sees it: **Settings → Controller**, find the Xbox 360 Controller, then **Test Device Inputs**. Lean and bash, and the stick and LB/RB should respond.

## Keyboard/mouse profiles (`"device": "kbm"`)

The board presses real keyboard keys and mouse buttons. Use this when:

- **you use gamescope.** The profile itself sets which keys the board presses, so there's no Steam Input remapping to set up;
- you play with keyboard and mouse and just want the board to press keys the game already uses;
- you don't want the game to see a second controller.

Make a profile with the keys the game uses, for example a copy of `profiles/darktide.json`.

## Steam Deck

The board can stand in for the Deck's back buttons (L4/L5/R4/R5): bash a foot instead of reaching behind the Deck.

- **Easiest:** use a **kbm** profile that presses the keys the game uses for those actions. It works alongside the Deck's own controls with no Steam Input setup.
- **Or:** use an **xbox** profile and remap its buttons in Steam Input. Some games only listen to one controller at a time, so a second controller may not be picked up by every game.

**Building on the Deck:** SteamOS doesn't include a C compiler and its system is read-only. Either build inside a container (`distrobox` or `toolbox`, with `make` and `gcc` installed) and run the resulting `wii-stomp` from your home folder, or build on another Linux PC and copy it over.

Pair the board in Desktop Mode first: see [bluetooth.md](bluetooth.md).

## Tips

- Start wii-stomp **before** the game. Some games only look for controllers at startup.
- Keep the terminal open while playing; closing it stops wii-stomp.
- Watch its output while you tune things: every detected bash is printed with its strength.
