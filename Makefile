PROGRAM = tapuino
ARDUINO_DIR = /usr/share/arduino
ROOT_DIR = $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))
SOURCE = $(ROOT_DIR)
TARGET = $(ROOT_DIR)/target
BUILDER = arduino-builder \
	-compile \
	-verbose \
	-build-path $(TARGET) \
	-fqbn arduino:avr:nano:cpu=atmega328 \
	-hardware $(ARDUINO_DIR)/hardware \
	-tools $(ARDUINO_DIR)/tools \
	-tools $(ARDUINO_DIR)/tools-builder \
	-tools $(ARDUINO_DIR)/hardware/tools/avr \
	-built-in-libraries $(ARDUINO_DIR)/libraries
AVRDUDE = avrdude \
	-v \
	-patmega328p \
	-carduino \
	-P/dev/ttyUSB0 \
	-b57600 \
	-D \
	-V

all: $(TARGET)/$(PROGRAM).ino.hex

$(TARGET)/$(PROGRAM).ino.hex: $(SOURCE)/$(PROGRAM).ino $(SOURCE)/*.c $(SOURCE)/*.h
	mkdir -p $(TARGET)
	$(BUILDER) $(SOURCE)/$(PROGRAM).ino

upload: $(TARGET)/$(PROGRAM).ino.hex
	$(AVRDUDE) -v -U flash:w:$(TARGET)/$(PROGRAM).ino.hex

status:
	$(AVRDUDE) -v

clean:
	rm $(TARGET) -rf
