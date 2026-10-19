# Connecting the balance board over Bluetooth

The board talks ordinary Bluetooth and Linux has a driver for it built in (`hid-wiimote`), but pairing it is fussier than most devices. The desktop Bluetooth settings often fail with it, so these steps use `bluetoothctl` in a terminal. Do this once; after that the board reconnects with its power button.

## Before you start

- Put 4 AA batteries in the board. Low batteries cause a lot of strange connection problems.
- Find the red **SYNC** button: it's inside the battery compartment on the underside.
- Check the driver is there: `modinfo hid-wiimote` should print some information. It's included in most distros, SteamOS and Bazzite.
- Your Bluetooth adapter has to support "classic" Bluetooth (BR/EDR). Nearly all built-in adapters do; some Bluetooth-LE-only USB dongles don't.

## Pairing

Open a terminal and start `bluetoothctl`:

```
$ bluetoothctl
[bluetooth]# power on
[bluetooth]# agent on
[bluetooth]# default-agent
[bluetooth]# scan on
```

Now press the red **SYNC** button on the board. The blue light on the front starts flashing. Within a few seconds a line like this should appear:

```
[NEW] Device 00:22:4C:xx:xx:xx Nintendo RVL-WBC-01
```

`RVL-WBC-01` is the balance board. Use its address (the `00:22:4C:...` part) in the next commands. Do them reasonably quickly, because the board stops flashing and gives up after about 20 seconds. If it does, press SYNC again.

```
[bluetooth]# pair 00:22:4C:xx:xx:xx
[bluetooth]# trust 00:22:4C:xx:xx:xx
[bluetooth]# connect 00:22:4C:xx:xx:xx
[bluetooth]# scan off
[bluetooth]# quit
```

- `pair`: Wii devices use an unusual PIN. BlueZ (Linux's Bluetooth stack) knows about Nintendo devices and supplies it automatically, so you shouldn't be asked for one.
- `trust`: lets the board reconnect later without asking.
- `connect`: once connected, the blue light stays on solid instead of flashing.

Check that Linux sees it:

```sh
grep -A5 "Balance Board" /proc/bus/input/devices
```

You should see `Name="Nintendo Wii Remote Balance Board"`, and a `Handlers=` line with an `eventN`. wii-stomp finds this automatically.

## Reconnecting later

Once paired and trusted, press the **power button** on the front of the board. The light flashes, then stays on when it's connected.

If that doesn't work, run `bluetoothctl connect 00:22:4C:xx:xx:xx` and press the power button again. Or press SYNC.

The board turns itself off after a while without input, and when you turn it off with the power button.

## Steam Deck / SteamOS

In Desktop Mode, open **Konsole** and follow the same steps. Once paired, the board should also show up in Game Mode's Bluetooth settings for reconnecting.

## If it doesn't work

- **`pair` fails or times out:** press SYNC again right before `pair`. If it keeps failing, skip `pair` and just `trust` and `connect`. That works too, but then you'll need to press SYNC every time to reconnect.
- **It was paired before and now won't connect:** remove it and pair again from the start:
  ```
  bluetoothctl remove 00:22:4C:xx:xx:xx
  ```
- **The light flashes but it never appears in `scan on`:** check the batteries, keep the board close to the computer, and make sure nothing else (a Wii, another PC) grabs it first.
- **It connects but there's no `Balance Board` in `/proc/bus/input/devices`:** the driver isn't loaded. Try `sudo modprobe hid-wiimote`.
- **It connects, then drops after a few seconds:** usually low batteries.
- **Connected, but wii-stomp says "Permission denied":** see [Permissions](../README.md#permissions) in the README.
