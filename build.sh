#! /bin/sh

(
  cmake -S . -B ./build -DPICO_BOARD=pico_w;
  cd build || exit;
  
  make -j12;
);