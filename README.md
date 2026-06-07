# Titan Souls Keyboard Fix

It's an SDL2 library proxy to fix the terrible keyboard controls Titan Souls ships with.

Yes, AI (Claude) was used as a guide and for C++ debugging. Working with JS and React 5 days a week causes C++ muscles to atrophy, which doesn't align with 11PM-1AM sessions poking at this.

It's ugly and all in one file. But it works.

**Work in Progress** - There's debug and logging code in there plus other code for things I'm still poking at.

## Updated Controls

Both the original controls and updated controls can be used with the mod in place.

It's all hard coded - I made it for myself.

**Movement**
* New: WASD
* Old: Arrow keys

**Rolling/Running**
* New: Space
* Old: X

**Arrow (Fire, Recall)**
* New: Left Mouse, Enter
* Old: C

## Installation steps

See `Building the Binary` below.

## Building the Binary

> I use Visual Studio 2026 Community edition.

1. Grab whatever version of SDL2 header files you want to use and put them in `sdl2-proxy\SDL2`
  * I used `SDL2-devel-2.0.8-VC.zip` from the [SDL 2.0.8](https://github.com/libsdl-org/SDL/releases/tag/release-2.0.8) release.
2. Disable or adjust the auto-copy path:
  * Right click sdl2-proxy project > Properties > Build Eevents > Post-Build Event
3. In the game files, rename `SDL2.dll` to `SDL2_original.dll`
4. Build, place the built binary in the game folders, open the game.

## You should do X instead of Y

If the game supports it, feel free to fix it. (If you fork this and improve it, ping me so I can learn!)

As far as I could tell, the game uses SDL2 events for nearly everything, which is why all the logic ended up in that proxy function.

## Mouse Aiming

I haven't been able to get proper mouse aiming implemented. I *could* implement the 8-cardinal directions (North, North-East, East, etc) based on cursor position, but I'd prefer to add the full range of aiming.

The game doesn't support keyboard _and_ controllers being active simultaneously. If a controller is detected, it only listens to that.

## Will you add ...?

Feel free to ask, but probably not.

I met the original goal of using WASD and other reasonable keyboard + mouse controls. My only other wanted feature is some form of mouse aiming. Anything else is sugar coating.
