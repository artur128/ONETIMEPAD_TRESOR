TARGET = main
MCU    = atmega88
CC     = avr-gcc
F_CPU = 8000000
OPT = s
 
CDEFS = -DF_CPU=$(F_CPU)UL
ADEFS = -DF_CPU=$(F_CPU)UL
CPPDEFS = -DF_CPU=$(F_CPU)UL
CFLAGS  =-mmcu=$(MCU) -Wall -Os $(CDEFS) -std=gnu99
LDFLAGS =-mmcu=$(MCU)

CFLAGS += -funsigned-bitfields
CFLAGS += -fshort-enums
CFLAGS += -Wall



$(TARGET): $(TARGET).o sha1.o hmac.o time.o

$(TARGET).hex : $(TARGET)
	 avr-objcopy -j .data -j .text -O ihex $< $@

program: $(TARGET).hex
	avrdude -p $(MCU) -c usbasp -U flash:w:$(TARGET).hex -v

clean:
	rm -f *.o  *.hex $(TARGET)

