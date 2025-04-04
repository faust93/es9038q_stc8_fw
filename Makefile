####### MCU #######

MCU                 ?= STC8G1K08A
MCU_IRAM            ?= 256
MCU_XRAM            ?= 1024
MCU_CODE_SIZE       ?= 8192

##### Project #####

PROJECT             ?= es9038q_stc8_fw
# The path for generated files
BUILD_DIR           = build

TOOCHAIN_PREFIX     ?= /opt/toolc/sdcc-4.4.0/bin/

# C source folders
USER_CDIRS          := fw
# C source single files
USER_CFILES         :=
USER_INCLUDES       := fw


## STC8G1K08A
LIB_FLAGS           := __CONF_FOSC=24000000UL \
					__CONF_MCU_MODEL=MCU_MODEL_STC8G1K08 \
					__CONF_CLKDIV=0x08 \
					__CONF_IRCBAND=0x00 \
					__CONF_VRTRIM=0x1F \
					__CONF_IRTRIM=0xBA \
					__CONF_LIRTRIM=0x00

include ./rules.mk