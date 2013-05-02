# Compilation options
PROGRAM = firefly
MCU = attiny13a
CC = avr-gcc
OBJCOPY = avr-objcopy
CFLAGS += -Wall -g -Os -mmcu=$(MCU)
LDFLAGS += 
OBJS = $(PROGRAM).o

# Flashing options
TOOL = usbtiny
TARGET = t13

# Be silent per default, but 'make V=1' will show all compiler calls.
ifneq ($(V),1)
Q := @
endif

all: $(PROGRAM).hex

$(PROGRAM).elf: $(PROGRAM).o
	@printf "  LD      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(PROGRAM).hex: $(PROGRAM).elf
	@printf "  OBJCOPY $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(OBJCOPY) -O ihex $< $@

%.o: %.c
	@printf "  CC      $(subst $(shell pwd)/,,$(@))\n"
	$(Q)$(CC) $(CFLAGS) -o $@ -c $<

flash: $(PROGRAM).hex
	@printf "  FLASH   $(PROGRAM).hex\n"
	@printf "  SIZE\n"
	@avr-size $(PROGRAM).hex
	$(Q)avrdude -c $(TOOL) -P usb -p $(TARGET) -U flash:w:$(PROGRAM).hex

clean:
	@printf "  CLEAN   $(subst $(shell pwd)/,,$(OBJS))\n"
	$(Q)rm -f $(OBJS)
	@printf "  CLEAN   $(PROGRAM).elf\n"
	$(Q)rm -f *.elf
	@printf "  CLEAN   $(PROGRAM).hex\n"
	$(Q)rm -f *.hex
