# BFB Info

A command line utility for viewing information about the lesser-known BFB file
format. This format is used (to my knowledge) exclusively by the video game Zoo
Tycoon 2, developed by the now defunct BlueFang Games.

Right now, this application will display basic information about the structure
of the file given as a command line argument. See the example below.

In the future, I plan on parsing more of the file, and dumping that output to
the terminal as well. I will be using the information I have gathered by
developing this application in future endeavours.

## Why?

I used to love this game. That's pretty much the only reason.

## Usage

Right now, this application is confirmed to only run on Linux systems -- however
it should in theory run on most POSIX compliant operating systems. It also will
run on Windows 10, via the Windows Subsystem for Linux.

## Example

```bash
user$ ./bfb Stegosaurus_Adult_F.bfb

author:      BlueFang
block count: 6

block id:   8
block type: 6
block name: meshData
block end:  e12e

vertex format: BFRVertexPNT0T1
vertex stride: 40
vertex count:  1232

index stride:  2
index count:   4062


block id:   4
block type: 6
block name: meshData
block end:  19384

vertex format: BFRVertexPNT0T1
vertex stride: 40
vertex count:  941

index stride:  2
index count:   3930


block id:   5
block type: 8
block name: mesh
block end:  1f1a1


block id:   9
block type: 8
block name: mesh
block end:  241fe


block id:   d
block type: 8
block name: mesh
block end:  2853b


block id:   10
block type: 4
block name: capsule
block end:  285a5

```


