Another World (Acorn RISC OS port)
==================================

This is a port of Eric Chahi's 1991 Amiga/ST game Another World to Acorn RISC OS machines.

This is a port of NEO-RAW, Fabien Sanglard's updated & cleaned up version of Gregory Montoir's raw.

Requirements:
-------------

ARM2 or better, 2 MB RAM, RISC OS 3, hard drive or high density floppies.

Building requires [GCCSDK](http://www.riscos.info/index.php/GCCSDK).

A copy of the DOS version of Another World / Out Of This World is required.

Compiling:
----------

```
cd src
CPP=<path to arm-unknown-riscos-g++> ELF2AIF=<path to elf2aif> make -f Makefile.gccsdk
cp \!RunImage\,ff8 ../\!AW/
```

Installation:
-------------

Copy !AW to the target machine, ensuring filetypes are preserved.

Copy the data files from the DOS version of the game (BANK* and MEMLIST.BIN) to !AW.data. MEMLIST.BIN may be truncated, this is fine.

Controls:
---------
- Arrow keys to move
- SPACE to fire/perform action
- C to enter level code
- Escape to quit

TODO:
-----

- Restore save game support (zlib)
- Support for 32-bit machines
- Support for VGA displays
- High resolution rendering
- Support running from 800k floppies


Original readme follows:

NEO-RAW:
====

This is an Another World VM implementation.  Based on Gregory Montoir's original work, the codebase has been cleaned up with legibility and readability in mind.

Architecture:
=============

http://fabiensanglard.net/anotherWorld_code_review/index.php
http://fabiensanglard.net/another_world_polygons/index.html
http://fabiensanglard.net/another_world_polygons_PC_DOS/index.html

Fabien Sanglard


About:
------

raw is a re-implementation of the engine used in the game Another World. This 
game, released under the name Out Of This World in non-European countries, was 
written by Eric Chahi at the beginning of the '90s. More information can be 
found on [MobyGames](https://www.mobygames.com/game/564/out-of-this-world/).

Supported Versions:
-------------------

English PC DOS version is supported ("Out of this World").

Compiling:
----------
```
cmake .
make
```
Running:
--------

You will need the original files, here is the required list :
- BANK*
- MEMLIST.BIN
	
To start the game, you can either :
- put the game's datafiles in the same directory as the executable
- use the --datapath command line option to specify the datafiles directory

Here are the various in game hotkeys :
-   Arrow Keys      allow you to move Lester
-   Enter/Space     allow you run/shoot with your gun
-   C               allow to enter a code to jump at a specific level
-   P               pause the game
-   Alt X           exit the game
-   Ctrl S          save game state
-   Ctrl L          load game state
-   Ctrl + and -    change game state slot
-   Ctrl F          toggle fast mode
-   TAB             change window scale factor

Credits:
--------

Eric Chahi, obviously, for making this great game.

Contact:
--------

Gregory Montoir, cyx@users.sourceforge.net
Fabien Sanglard, fabiensanglard.net@gmail.com
