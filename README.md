# Saturn Link : Midgaard

```
              _.--------._
         _.-'      /\   +  '-._
      .-'  +      /**\         '-.
    .'    /\     /.* *\   +       '.
   /     /**\   /   .  \    /\      \
  |     /*.* \ /  .   . \  /**\      |
  |    /.   . V .   .   .\/ * *\     |
  |   /   .   .   .   .   .   . \    |
   \   ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^ ^    /
    './^\ /^\ /^\ /^\ /^\ /^\ /^\ .'
      '-.__|___|___|___|___|___.-'
         '-._~~~~~~~~~~~~~~_.-'
              '----------'
```

A Multi-User Dungeon (MUD) client for the Sega Saturn and Dreamcast.

A rehost of [CoffeeMUD](https://github.com/bozimmerman/CoffeeMud) for the Sega Saturn and Dreamcast.

CoffeeMUD is Bo Zimmerman's fantasy MUD engine. Saturn Link runs it at [suin.uk](https://suin.uk) on
a classic MUD world, and this repository is the client that lets a Saturn or a
Dreamcast dial in and play it.

Bringing dynamic weather, open-ocean naval combat, a crafting economy and
material gathering, and detailed character customization. Conquer dungeons, build an
economic empire, or command a fleet of ships.

Forge your identity from 25 races, from Humans, Elves and Dwarves to Pixies,
Centaurs and Merfolk, each with its own strengths. Start in one of six classes,
from Fighter, Mage and Cleric to the Thief, the Druid and the Bard, then train into
one of some forty specialists, from the Paladin to the Pirate and the Assassin. Every
choice shapes your stats, your abilities and how the world treats you. The
[player's handbook](https://suin.uk/mud/guides/handbook.html) covers them all.

- **Midgaard**, the starting city, with Haon Dor and the Sewers, comes from
  **DikuMUD**, written at the University of Copenhagen in 1990. (Most fantasy MUDs
  descend from DikuMUD, and Midgaard became the starting city of many of them.)
- 48 areas are the archive's native set (again many of them DikuMUD-era zones in
  CoffeeMUD's format)
- 45 areas come from CircleMUD pack and 4 from its SMAUG pack. (Both codebases descend
  from DikuMUD, which is why their exits lead into Midgaard).

Play from:

- **Sega Saturn**, through the NetLink browser and a DreamPi, at [suin.uk/mud](suin.uk/mud)
- **Dreamcast**, with a Broadband Adapter or a DreamPi

As a bonus, the same world also plays in any PC or phone browser at [suin.uk/mud](suin.uk/mud).

The [Saturn Link guides](https://suin.uk/mud/guides/) cover the game, its commands,
races and classes.

This repository is the console client: a single terminal for the Saturn and the
Dreamcast.

## Playing

### Saturn 
  Two options, both require a NetLink modem and a DreamPi, and one forgoes audio effects:

#### Through the browser (no audio)
- PlanetWeb 4.0. In PlanetWeb
- open `http://suin.uk/mud`

#### Using CD
- Download the Saturn disc from the [releases](https://github.com/suinevere/saturn-link-midgaard/releases) page
- Burn, mount, copy the disc to the Saturn,

### Dreamcast
- Download the Saturn disc from the [releases](https://github.com/suinevere/saturn-link-midgaard/releases) page
- Burn, mount, copy the disc to the Saturn
- Both 240p and 480p modes are supported

**All Versions require a keyboard.**

**Sound.** The Dreamcast disc and the burned Saturn disc play the game's sound
effects: type `SOUNDS` once in game to have them sent, and F4 mutes them. The
Saturn netbin has no room for them and plays none.

| Key | Action |
| --- | --- |
| Enter | Send the line |
| Left, Right, Home, End | Move in the line |
| Backspace, Delete | Edit the line |
| Up, Down | Previous and next line sent |
| Ctrl+C | Clear the line |
| Page Up, Page Down | Scroll back and forward |
| Ctrl+Up, Ctrl+Down | Scroll one line |
| Esc twice within a second | Hang up |
| F4 | Mute or unmute sound, on the discs |
| F5 | Cycle the colour of the MUD's plain text |
| F6 | Cycle the colour of what you type and your sent lines |
| F7 | Cycle the colour of the client's own messages |
| F8 | Cycle the MUD's own colours: ANSI, green, amber, white, cyan |
| F9 | Show the alignment ruler, for setting the picture on a TV |
| F10 | Saturn: switch between 78 and 80 columns. Dreamcast: switch between 480p and 240p |
| F11, F12 | Saturn: move the picture left and right. Dreamcast: widen and narrow the border above and below |

## Repository layout

```
core/            code shared by every build, one folder per feature
  session/       the connect, play, reconnect loop
  telnet/        telnet and ANSI: the server's bytes into text and colour
  screen/        scrollback, its view, colours, box glyphs, alignment ruler
  input/         key events and the line editor
  proxy/         the AUTH handshake the suin.uk proxy expects
  ports/         the screen, connection and sound interfaces each platform provides
adapters/tcp/    the socket code the Dreamcast and PC builds share
sound/           builds the discs' sound packs from CoffeeMUD's sounds.zip
saturn/          the Saturn build: SaturnRingLib project, modem, VDP2 screen
dreamcast/       the Dreamcast build: KallistiOS, framebuffer, BBA and PPP
host/            a PC build of the client, for testing without a console
server/          files for the Saturn Link server, at their paths under its tree
tests/           unit tests and a fake MUD server
SaturnRingLib/   the Saturn SDK, as a git submodule
```

## Building

### Saturn

Clone with the submodule, then fetch the SH-2 compiler into it once:

```bash
git clone --recursive https://github.com/suinevere/saturn-link-midgaard.git
cd saturn-link-midgaard/SaturnRingLib
./tools/scripts/getcompiler.sh 14.2.0
./tools/scripts/getiso2raw.sh v0.2.2
cd ..
```

On Windows, `SaturnRingLib/setup_compiler.bat` does the same. Then, from `saturn/`:

| Command | Output in `saturn/BuildDrop/` |
| --- | --- |
| `./compile-netbin.bat release` | `coffeemud.netbin`, the PlanetWeb download |
| `./compile.bat release` | `Saturn Link Midgaard (USA).cue` and `.bin`, the disc |
| `./clean.bat` | removes the build output |

Each `.bat` file also runs as a shell script, so the same commands work in Git
Bash, Linux and macOS. The netbin must stay under the PlanetWeb loader's 400 KB
limit, and `package-netbin.sh` fails the build if it does not.

For sound on the disc, first run `sh sound/sound-pack.sh saturn
saturn/cd/data/SOUNDS.PAK` from the repository root; it needs ffmpeg, unzip and
python3. Without the pack the disc still builds, silently.

`run_with_mednafen.bat` starts the disc in Mednafen, which needs a Saturn BIOS.
Mednafen has no NetLink, so it shows the screen but cannot connect.

### Dreamcast

Build inside a KallistiOS environment, such as the DreamSDK shell:

```sh
cd dreamcast
make                          # build/cmud.elf, for dcload-ip or an emulator
make cdi CMUD_SECRET=...      # build/cmud.cdi, for a CD-R or an emulator
make run                      # send the ELF to a console over dcload-ip
```

For sound, run `sh sound/sound-pack.sh dreamcast dreamcast/sounds` from the
repository root and add `SOUNDS=sounds` to `make cdi`.

The suin.uk proxy only relays connections that send its shared secret first. On
the Saturn the DreamPi sends it; the Dreamcast sends it itself, so `make cdi`
needs `CMUD_SECRET`. The secret is readable in the finished disc image. Leave it
empty to connect to a server without the proxy. `CMUD_HOST` and `CMUD_PORT`
change the destination.

To test in Flycast, turn on Broadband Adapter emulation and set `DCNet = no` under
`[network]` in `emu.cfg`, so the emulated adapter can reach your own network.

### PC and tests

```bash
tests/run.sh                          # unit tests
host/build.sh                         # builds host/cmud-host
host/cmud-host 127.0.0.1 5555         # plays a MUD in the terminal
python3 tests/fixture_mud.py 5599     # a fake MUD that checks what the client sends
```

## Releases

`.github/workflows/build.yml` runs on every push. It runs the unit tests, plays
the PC build against the fake MUD, and builds the netbin, the Saturn disc and the
Dreamcast CDI, checking each one before uploading it.

A `v*` tag also publishes a release with the Saturn zip and the Dreamcast CDI,
and tells the suin-uk repository to deploy the new netbin. Two repository secrets
are needed:

| Secret | Purpose |
| --- | --- |
| `CMUD_DC_SECRET` | The proxy secret built into the Dreamcast CDI |
| `SUINEVERE_CI_PAT` | Starts the netbin deployment in `suinevere/suin-uk` |

## Credits

- The Saturn Link server runs on the CoffeeMUD engine by Bo Zimmerman and
  contributors, under the Apache License 2.0.
- The world is from CoffeeMUD's area archive: DikuMUD's Midgaard, Haon Dor and
  Sewers, by the DikuMUD authors at the University of Copenhagen, and the native,
  CircleMUD and SMAUG areas of the builders each area file names.
- The sound effects are CoffeeMUD's MSP pack, `sounds.zip`, as published at
  coffeemud.net and in the CoffeeMUD repository; the discs carry converted copies.
- SaturnRingLib by ReyeMe and contributors, the Saturn SDK. SEGA's SGL library
  and the SH-2 compiler come with it under their own terms.
- KallistiOS, the Dreamcast SDK. The 480p text uses its 8x16 "Naomi" font.
- The 240p text uses font8x8 by Daniel Hepper, from the public-domain IBM VGA
  font.

## License

MIT, for the code in this repository. See [LICENSE](LICENSE). The SDKs, SGL and
the compilers keep their own licences, and a built Saturn disc contains SGL code.
