# Name of the binaries.
PROJ_NAME=jocho-arm

######################################################################
#                         SETUP SOURCES                              #
######################################################################


# This is the directory containing the firmware package,
# the unzipped folder downloaded from here:
# http://www.st.com/web/en/catalog/tools/PF257904
STM_DIR=/home/niklas/code/STM32F4-Discovery_FW_V1.1.0

# This is where the source files are located,
# which are not in the current directory
# (the sources of the standard peripheral library, which we use)
# see also "info:/make/Selective Search" in Konqueror
STM_SRC = $(STM_DIR)/Libraries/STM32F4xx_StdPeriph_Driver/src

SRC_DIRS += $(STM_SRC)

# Tell make to look in that folder if it cannot find a source
# in the current directory
vpath %.c $(SRC_DIRS)

# My source file
SRCS = main.c counter.c 
SRCS += hw/audio.c
SRCS += synth/env.c synth/op.c synth/algorithms.c synth/voice.c synth/wt.c


# Contains initialisation code and must be compiled into
# our project. This file is in the current directory and
# was writen by ST.
SRCS  += system_stm32f4xx.c

# These source files implement the functions we use.
# make finds them by searching the vpath defined above.
SRCS  += stm32f4xx_rcc.c 
SRCS  += stm32f4xx_gpio.c
SRCS  += stm32f4xx_dma.c
SRCS  += stm32f4xx_dac.c
SRCS  += stm32f4xx_tim.c
SRCS  += stm32f4xx_syscfg.c
SRCS  += misc.c

# Startup file written by ST
# The assembly code in this file is the first one to be
# executed. Normally you do not change this file.
SRCS += $(STM_DIR)/Libraries/CMSIS/ST/STM32F4xx/Source/Templates/TrueSTUDIO/startup_stm32f4xx.s

# The header files we use are located here
INC_DIRS  = $(STM_DIR)/Libraries/CMSIS/Include
INC_DIRS += $(STM_DIR)/Libraries/CMSIS/ST/STM32F4xx/Include
INC_DIRS += $(STM_DIR)/Libraries/STM32F4xx_StdPeriph_Driver/inc
INC_DIRS += $(STM_DIR)/Utilities/STM32F4-Discovery
INC_DIRS += $(STM_DIR)/Utilities/STM32_EVAL/Common
INC_DIRS += .
INC_DIRS += synth

# in case we have to many sources and don't want 
# to compile all sources every time
OBJS = $(SRCS:.c=.o)

######################################################################
#                         SETUP TOOLS                                #
######################################################################


# This is the path to the toolchain
# (we don't put our toolchain on $PATH to keep the system clean)
TOOLS_DIR = /usr/bin

# The tool we use
CC      = $(TOOLS_DIR)/arm-none-eabi-gcc
OBJCOPY = $(TOOLS_DIR)/arm-none-eabi-objcopy
GDB     = $(TOOLS_DIR)/arm-none-eabi-gdb

## Preprocessor options

# directories to be searched for header files
INCLUDE = $(addprefix -I,$(INC_DIRS))

# #defines needed when working with the STM library
DEFS    = -DUSE_STDPERIPH_DRIVER #-DUSE_FULL_ASSERT
# if you use the following option, you must implement the function 
#    assert_failed(uint8_t* file, uint32_t line)
# because it is conditionally used in the library
# DEFS   += -DUSE_FULL_ASSERT
DEFS    += -DSTM32F40XX
#DEFS    += -DSD_POLLING_MODE

## Compiler options
CFLAGS  = -ggdb
CFLAGS += -O3
CFLAGS += -Wall -Wextra -Warray-bounds
CFLAGS += -mlittle-endian -mthumb -mcpu=cortex-m4 -mthumb-interwork
CFLAGS += -mfloat-abi=hard -mfpu=fpv4-sp-d16
CFLAGS += -ffunction-sections -fdata-sections -std=c99

## Linker options
# tell ld which linker file to use
# (this file is in the current directory)
LFLAGS  = -Tstm32_flash.ld
#LFLAGS += --gc-sections


######################################################################
#                         SETUP TARGETS                              #
######################################################################

.PHONY: $(PROJ_NAME) all
all: $(PROJ_NAME)

$(PROJ_NAME): $(PROJ_NAME).elf

$(PROJ_NAME).elf: $(SRCS)
	$(CC) $(INCLUDE) $(DEFS) $(CFLAGS) $(LFLAGS) $^ -o $@ 
	$(OBJCOPY) -O ihex $(PROJ_NAME).elf   $(PROJ_NAME).hex
	$(OBJCOPY) -O binary $(PROJ_NAME).elf $(PROJ_NAME).bin

clean:
	rm -f *.o $(PROJ_NAME).elf $(PROJ_NAME).hex $(PROJ_NAME).bin

# Flash the STM32F4
flash: 
	$(TOOLS_DIR)/st-flash write $(PROJ_NAME).bin 0x8000000

.PHONY: debug
debug:
# before you start gdb, you must start st-util
	$(GDB) $(PROJ_NAME).elf
