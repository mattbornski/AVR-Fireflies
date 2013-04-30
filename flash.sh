#!/bin/bash

avrdude -p t13 -c usbtiny -e -U flash:w:fireflyLED.hex
