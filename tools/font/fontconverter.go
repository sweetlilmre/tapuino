/*

Copyright (c) 2020 Andreas Signer <asigner@gmail.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
package main

*/
package main

import (
	"fmt"
	"image/png"
	"io"
	"log"
	"os"
)

const (

	firstChar = 0x20
	lastChar = 0x7e
	arrowRight = 26
)

func usage() {
	fmt.Fprintf(os.Stderr, "usage: fontconverter <font-image>\n")
	os.Exit(1)
}

func main() {
	targetFilename := "../../font8x16.h"
	sourceFilename := "IBM_VGA_8x16.png"

	if len(os.Args) >= 2 {
		sourceFilename = os.Args[1]
	}
	if len(os.Args) >= 3 {
		targetFilename = os.Args[2]
	}
	fmt.Printf("Creating header file %s from %s...\n", targetFilename, sourceFilename)
	f, err := os.Open(sourceFilename)
	if err != nil {
		log.Fatalf("Can't open file %s: %s", sourceFilename, err)
	}
	defer f.Close()

	img, err := png.Decode(f)
	if err != nil {
		log.Fatalf("Can't decode png: %s", err)
	}

	var bottomBytes, topBytes []byte
	for ch := firstChar; ch <= lastChar; ch++ {
		c := ch
		if c == lastChar {
			c = arrowRight
		}
		px, py := (c%16)*9+1, int(c/16)*17;
		for x := 0; x < 8; x++ {
			val := 0
			for y:=0; y<16;y++ {
				r,_,_,_ := img.At(px +x , py+y).RGBA()
				if r > 0 {
					val = val | (1<<y)
				}
			}
			bottomBytes = append(bottomBytes, byte(val >> 8))
			topBytes = append(topBytes, byte(val & 0xff))
		}
	}

	of, err := os.Create(targetFilename)
	if err != nil {
		log.Fatalf("Can't create target file: %s", err)
	}
	defer of.Close()

	fmt.Fprintf(of,"#include <avr/pgmspace.h>\n")
	fmt.Fprintf(of, "\n")
	dumpBytes(of, bottomBytes, "font8x16_bottom")
	dumpBytes(of, topBytes, "font8x16_top")

	fmt.Println("Done.")
}

func dumpBytes(w io.Writer, b []byte, name string) {

	fmt.Fprintf(w, "const uint8_t %s [] PROGMEM = {\n", name)
	for c := firstChar; c <= lastChar; c++ {
		cstr := string(rune(c))
		if c == lastChar {
			cstr = "->"
		}
		p := (c - firstChar) * 8
		fmt.Fprintf(w, "    0x%02X,  0x%02X,  0x%02X,  0x%02X,  0x%02X,  0x%02X,  0x%02X,  0x%02X,    // [0x%02X] '%s'\n", b[p+0], b[p+1], b[p+2], b[p+3], b[p+4], b[p+5], b[p+6], b[p+7], c, cstr)
	}
	fmt.Fprintf(w, "};\n")
}
