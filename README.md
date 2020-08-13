<p align="center">
  <img src="assets/dumpling-logo.png" alt="Dumpling Logo" width="80" height="80">

  <h3 align="center">Dumpling</h3>

  <p align="center">
    A simple, all-in-one Wii U file dumper! Developed with the intent of making dumping games and other files for Cemu faster and easier.
  </p>
</p>

## How to install
**Method 1:**  
Use the Wii U App Store to download and install it in the homebrew launcher. See Dumpling's page [here](https://apps.fortheusers.org/wiiu/dumpling).

**Method 2:**  
Download the [latest release from Github](https://github.com/emiyl/dumpling/releases), and extract the `dumpling.zip` file to the root of your SD card.

Using it is as simple as running Mocha or Haxchi and launching Dumpling in the homebrew launcher.


## How to compile
- Install [DevkitPro](https://devkitpro.org/wiki/Getting_Started) for your platform.
- Install [wut](https://github.com/devkitPro/wut) through DevkitPro's pacman or compile (and install) the latest source yourself.
- Compile [libiosuhax](https://github.com/yawut/libiosuhax#using-wut---static-library) from source yourself as a static library.
- Compile [libfat](https://github.com/Crementif/libfat) from source, since it has been fixed to perform MUCH better in certain situations which would normally cripple the classic Dumpling.
- Then, with all those dependencies installed, you can just run `make` to get the .rpx file that you can run on your Wii U.


## Features
- Dumps everything related to your games! Game, updates, DLC and saves are all dumpable!
- Dumps both disc and digital games in an extracted format, making for easy modding.
- Creates 1:1 copies of data with proper meta data.
- Allows dumping to an SD or USB stick/drive (must be formatted as fat32).
- Allows you to dump system applications too.
- Quickly dump files required for online dumping
- Feature to quickly dump all the files needed for Cemu online play
  - You must dump `otp.bin` and `seeprom.bin` separately with [wiiu-nanddumper](https://github.com/koolkdev/wiiu-nanddumper) (for now!)
- Feature to quickly dump compatibility files which can be used to improve graphics and game compatibillity in Cemu.
- Has features to dump the base game files, update files and DLC files separately.

## Credits
- dimok789 for [ft2sd](https://github.com/dimok789/ft2sd/)
- dimok789 and FIX94 for [FTPiiU Everywhere](https://github.com/FIX94/ftpiiu/tree/ftpiiu_everywhere)
- GaryOderNichts for added support of Haxchi and other controllers
- shepgoba, rw-r-r-0644, luigoalma, vgmoose and Pysis for helping me with the project
- chrissie and CrafterPika for testing