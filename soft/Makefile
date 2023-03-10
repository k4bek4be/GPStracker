### Project name (also used for output file name)
PROJECT	= glg

### Source files and search directory
CSRC    = main.c uart0.c uart1.c ff.c mmc.c 1wire.c ds18b20.c expander.c HD44780-I2C.c I2C.c xprintf.c gpx.c ffunicode.c display.c working_modes.o timec.o nmea.o settings.o menu.o
ASRC    = stime.S
VPATH   =

### Target device
DEVICE  = atmega644p

### Bootloader port
PROGPORT ?= /dev/ttyUSB0

### Optimization level (0, 1, 2, 3, 4 or s)
OPTIMIZE = s

### C Standard level (c89, gnu89, c99 or gnu99)
#CSTD = c99

### Include dirs, library dirs and definitions
LIBS	=
LIBDIRS	=
INCDIRS	=
DEFS	= F_CPU=7372800
# DEFS	+= LEDR_UART
ADEFS	=

### Warning contorls
WARNINGS = all extra

### Output directory
OBJDIR = obj

### Output file format (hex, bin or both) and debugger type
OUTPUT	= hex
HEXFMT  = ihex
DEBUG	= dwarf-2


### Programs to build porject
CC      = avr-gcc
OBJCOPY = avr-objcopy
OBJDUMP = avr-objdump
SIZE    = avr-size
NM      = avr-nm
AVRDUDE	= avrdude -c arduino -b 230400 -P $(PROGPORT) -p $(DEVICE)


# Define all object files
COBJ      = $(CSRC:.c=.o) 
AOBJ      = $(ASRC:.S=.o)
COBJ      := $(addprefix $(OBJDIR)/,$(COBJ))
AOBJ      := $(addprefix $(OBJDIR)/,$(AOBJ))
PROJECT   := $(OBJDIR)/$(PROJECT)


# Flags for C files
#CFLAGS += -std=$(CSTD)
CFLAGS += -g$(DEBUG)
CFLAGS += -mmcu=$(DEVICE)
CFLAGS += -O$(OPTIMIZE) -mcall-prologues
CFLAGS += $(addprefix -W,$(WARNINGS)) --param=min-pagesize=0
CFLAGS += $(addprefix -I,$(INCDIRS))
CFLAGS += $(addprefix -D,$(DEFS))
CFLAGS += -Wp,-MM,-MP,-MT,$(OBJDIR)/$(*F).o,-MF,$(OBJDIR)/$(*F).d


# Assembler flags
ASFLAGS += $(addprefix -D,$(ADEFS)) -Wa,-gstabs,-g$(DEBUG)
ALL_ASFLAGS = -mmcu=$(DEVICE) -I. -x assembler-with-cpp $(ASFLAGS)


# Linker flags
LDFLAGS += -Wl,-Map,$(PROJECT).map


# Default target.
all: version build size

ifeq ($(OUTPUT),hex)
build: elf hex lst sym
hex: $(PROJECT).hex
else
ifeq ($(OUTPUT),bin)
build: elf bin lst sym
bin: $(PROJECT).bin
else
ifeq ($(OUTPUT),both)
build: elf hex bin lst sym
hex: $(PROJECT).hex
bin: $(PROJECT).bin
else
$(error "Invalid format: $(OUTPUT)")
endif
endif
endif

elf: $(PROJECT).elf
lst: $(PROJECT).lst 
sym: $(PROJECT).sym


# Display compiler version information.
version :
	@$(CC) --version

# Create final output file from ELF output file.
%.hex: %.elf
	@echo
#	$(OBJCOPY) -j .text -j .data -j .eeprom -j .fuse -O $(HEXFMT) $< $@
	$(OBJCOPY) -j .text -j .data -O $(HEXFMT) $< $@

%.bin: %.elf
	@echo
	$(OBJCOPY) -j .text -j .data -O binary $< $@

# Create extended listing file from ELF output file.
%.lst: %.elf
	@echo
	$(OBJDUMP) -h -S -C $< > $@

# Create a symbol table from ELF output file.
%.sym: %.elf
	@echo
	$(NM) -n $< > $@

# Display size of file.
size:
	@echo
	$(SIZE) $(PROJECT).elf
	$(SIZE) $(PROJECT).hex


# Link: create ELF output file from object files.
%.elf:  $(AOBJ) $(COBJ)
	@echo
	@echo Linking...
	$(CC) $(CFLAGS) $(AOBJ) $(COBJ) --output $@

# Compile: create object files from C source files. ARM or Thumb(-2)
$(COBJ) : $(OBJDIR)/%.o : %.c
	@echo
	@echo $< :
	$(CC) -c $(CFLAGS) $< -o $@

# Assemble: create object files from assembler source files. ARM or Thumb(-2)
$(AOBJ) : $(OBJDIR)/%.o : %.S
	@echo
	@echo $< :
	$(CC) -c $(ALL_ASFLAGS) $< -o $@


# Target: clean project.
clean:
	@echo
	rm -f -r $(OBJDIR) | exit 0


# Include the dependency files.
-include $(shell mkdir $(OBJDIR) 2>/dev/null) $(wildcard $(OBJDIR)/*.d)

install: all
	$(AVRDUDE) -U flash:w:$(PROJECT).hex
reset:
	$(AVRDUDE)

