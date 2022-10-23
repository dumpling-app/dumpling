<div style="text-align: center;">
    <img src="assets/dumpling-logo.png" alt="Dumpling Logo" style="width: 80px;" />
    <h3>Dumpling</h3>
    <p>A simple, all-in-one Wii U file dumper! Developed with the intent of making dumping games and other files (for emulators like Cemu) faster and easier.</p>
</div>

## How to install
**Method 1:**  
Use the Wii U App Store to download and install it in the homebrew launcher. See Dumpling's page [here](https://apps.fortheusers.org/wiiu/dumpling).

**Method 2:**  
Download the [latest release from GitHub](https://github.com/emiyl/dumpling/releases), and extract the `dumpling.zip` file to the root of your SD card.

**Method 3:**  
Use [dumpingapp.com](https://dumplingapp.com) on your Wii U to launch Dumpling without any setup or SD card.

## How to use

For an always up-to-date guide to dump your games for Cemu using Dumpling, see [cemu.cfw.guide](https://cemu.cfw.guide/dumping-games)!  

If you want to fully homebrew your Wii U too, we recommend using [wiiu.hacks.guide](https://wiiu.hacks.guide) to install Tiramisu and installing Dumpling using the first two methods mentioned above!

You don't need to run/have Mocha CFW or Haxchi, just launch Dumpling from the Homebrew Launcher.

## How to compile
 - Install [DevkitPro](https://devkitpro.org/wiki/Getting_Started) for your platform.
 - Install freetype2 using DevkitPro's pacman (e.g. `(dkp-)pacman -Sy ppc-pkg-config ppc-freetype`).
 - Install [wut](https://github.com/devkitpro/wut) through DevkitPro's pacman or compile (and install) the latest source yourself.
 - Compile [libmocha](https://github.com/wiiu-env/libmocha).
 - Then, with all those dependencies installed, you can just run `make` to get the .rpx file that you can run on your Wii U.


## Features
 - Dumps everything related to your games! Game, updates, DLC and saves are all dumped through one simple GUI!
 - Dumps both disc and digital games in an extracted format, making for easy modding and usage with Cemu.
 - Creates 1:1 copies of data with proper meta data.
 - Allows dumping to an SD or USB stick/drive (must be formatted as fat32).
 - Allows you to dump system applications too.
 - Feature to quickly dump everything needed to play online with Cemu, including the otp.bin and seeprom.bin!
 - Also dumps extra compatibility files for Cemu when dumping online files.
 - Has features to dump the base game, update, DLC and save files separately.
 - Now also supports easily dumping vWii games (requires [nfs2iso2nfs](https://github.com/FIX94/nfs2iso2nfs/releases/tag/v0.5.6) for converting vWii games to .iso).

## Credits
 - [Crementif](https://github.com/Crementif) for [dumpling-rework](https://github.com/emiyl/dumpling)
 - [emiyl](https://github.com/emiyl) for [dumpling-classic](https://github.com/emiyl/dumpling-classic)
 - chriz, Tomk007 and Jaimie for testing
 - [wut](https://github.com/devkitpro/wut) for providing the Wii U toolchain that Dumpling is built with
 - FIX94, Maschell, Quarky, GaryOderNichts and koolkdev for making and maintaining homebrew (libraries)
 - smea, plutoo, yellows8, naehrwert, derrek, dimok and kanye_west for making the exploits and CFW possible

## License
Dumpling is licensed under [MIT](https://github.com/emiyl/dumpling/blob/master/LICENSE.md).  
Dumpling uses [fatfs](http://elm-chan.org/fsw/ff/00index_e.html), see its BSD-styled license [here](https://github.com/emiyl/dumpling/blob/master/source/utils/fatfs/LICENSE.txt).
