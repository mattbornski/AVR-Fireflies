#!/bin/bash

brew install avrdude

brew tap larsimmisch/avr
brew install avr-libc

# The avr toolchain I currently have fails to put the target definition for the attiny13a in the appropriate location
AVR_GCC_VERSION=$(avr-gcc --version | head -1)
if [ "${AVR_GCC_VERSION}" == "avr-gcc (GCC) 4.7.2" ] ; then
  pushd /usr/local/Cellar
  cp avr-gcc/4.7.2/avr/lib/avr25/crttn13a.o avr-gcc/4.7.2/avr/lib/crttn13a.o 
  popd
fi
