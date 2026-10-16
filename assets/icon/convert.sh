#!/bin/bash

# Requirements: rsvg, magick

rsvg-convert -w 256 -h 256 icon.svg -o icon.png

magick icon.png -define icon:auto-resize=256,128,64,48,32,16 icon.ico