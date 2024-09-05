#!/bin/sh

clear

project=${PWD##*/}

echo "Uploading binary"
echo "${pwd}"

project=${PWD##*/}

pkill openocd;

openocd -c "tcl_port disabled" \
  -c "gdb_port 3333" \
  -c "telnet_port 4444" \
  -s /usr/share/openocd/scripts \
  -f board/pico-debug.cfg \
  -c "program build/${project}.elf" \
  -c "init;reset init;" \
  -c "echo (((READY)))";