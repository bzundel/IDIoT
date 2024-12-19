APPLICATION = read_saul_values

BOARD ?= esp32-wroom-32

RIOTBASE ?= $(HOME)/RIOT

CFLAGS += -DDHT_PARAM_PIN=21

USEMODULE += saul_default
USEMODULE += dht # requires driver for dht sensor
USEMODULE += posix_sleep

DEVELHELP ?= 1

QUIET ?= 1

include $(RIOTBASE)/Makefile.include
