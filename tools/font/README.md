# README

This directory contains a small tool to generate the 8x16 font used as big font
for small (128x32) SSD1306 displays.

## Prerequisites
To build the tool, you need Go 1.13 or later.

## Building and running to the converter
To create the header file, just execute
```
go run fontconverter.go
```

This will read the image `IBM_VGA_8x16.png` and convert it to the header file 
`font8x16.h` in Tapuino's source directory.

If you want to convert a different image, or to a different target, just add 
source and target to the command line:
```
go run fontconverter.go <image> <headerfile>
```
 
 ## Credits
The font image was taken from https://github.com/spacerace/romfont/blob/master/font-images/IBM_VGA_8x16.png

