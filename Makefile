# Top level make file for building PandA socket server and associated device
# drivers for interfacing to the FPGA resources.

TOP := $(CURDIR)

# Build defaults that can be overwritten by the CONFIG file if present

BUILD_DIR = $(TOP)/build
PYTHON = python2
SPHINX_BUILD = sphinx-build
ARCH = arm
CROSS_COMPILE = arm-xilinx-linux-gnueabi-
BINUTILS_DIR = /dls_sw/FPGA/Xilinx/SDK/2015.1/gnu/arm/lin/bin
KERNEL_DIR = $(error Define KERNEL_DIR before building driver)
SIM_SERVER_DIR = $(TOP)

DEFAULT_TARGETS = driver server sim_server docs zpkg

-include CONFIG


CC = $(CROSS_COMPILE)gcc

DRIVER_BUILD_DIR = $(BUILD_DIR)/driver
SERVER_BUILD_DIR = $(BUILD_DIR)/server
SIM_SERVER_BUILD_DIR = $(BUILD_DIR)/sim_server
DOCS_BUILD_DIR = $(BUILD_DIR)/html

DRIVER_FILES := $(wildcard driver/*)
SERVER_FILES := $(wildcard server/*)

ifdef BINUTILS_DIR
PATH := $(BINUTILS_DIR):$(PATH)
endif

default: $(DEFAULT_TARGETS)
.PHONY: default


export GIT_VERSION := $(shell git describe --abbrev=7 --dirty --always --tags)

# ------------------------------------------------------------------------------
# Kernel driver building

PANDA_KO = $(DRIVER_BUILD_DIR)/panda.ko

# Building kernel modules out of tree is a headache.  The best workaround is to
# link all the source files into the build directory.
DRIVER_BUILD_FILES := $(DRIVER_FILES:driver/%=$(DRIVER_BUILD_DIR)/%)
$(DRIVER_BUILD_FILES): $(DRIVER_BUILD_DIR)/%: driver/%
	ln -s $$(readlink -e $<) $@

# The driver register header file needs to be built.
DRIVER_HEADER = $(DRIVER_BUILD_DIR)/panda_drv.h
$(DRIVER_HEADER): driver/panda_drv.py config_d/registers
	$(PYTHON) $< >$@

$(PANDA_KO): $(DRIVER_BUILD_DIR) $(DRIVER_BUILD_FILES) $(DRIVER_HEADER)
	$(MAKE) -C $(KERNEL_DIR) M=$< modules \
            ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	touch $@


driver: $(PANDA_KO)
.PHONY: driver


# ------------------------------------------------------------------------------
# Socket server

SERVER = $(SERVER_BUILD_DIR)/server
SIM_SERVER = $(SIM_SERVER_BUILD_DIR)/sim_server
SERVER_FILES := $(wildcard server/*)

$(SERVER): $(SERVER_BUILD_DIR) $(SERVER_FILES)
	$(MAKE) -C $< -f $(TOP)/server/Makefile \
            VPATH=$(TOP)/server TOP=$(TOP) CC=$(CC)

# Two differences with building sim_server: we use the native compiler, not the
# cross-compiler, and we only build the sim_server target.
$(SIM_SERVER): $(SIM_SERVER_BUILD_DIR) $(SERVER_FILES)
	$(MAKE) -C $< -f $(TOP)/server/Makefile \
            VPATH=$(TOP)/server TOP=$(TOP) sim_server

# Construction of simserver launch script.
SIMSERVER_SUBSTS += s:@@PYTHON@@:$(PYTHON):;
SIMSERVER_SUBSTS += s:@@BUILD_DIR@@:$(BUILD_DIR):;
SIMSERVER_SUBSTS += s:@@SIM_SERVER_DIR@@:$(SIM_SERVER_DIR):;

simserver: simserver.in
	sed '$(SIMSERVER_SUBSTS)' $< >$@
	chmod +x $@

server: $(SERVER)
sim_server: $(SIM_SERVER) simserver

.PHONY: server sim_server


# ------------------------------------------------------------------------------
# Documentation

$(DOCS_BUILD_DIR)/index.html: $(wildcard docs/*.rst docs/*/*.rst docs/conf.py)
	$(SPHINX_BUILD) -b html docs $(DOCS_BUILD_DIR)

docs: $(DOCS_BUILD_DIR)/index.html

docs-clean:
	rm -rf $(DOCS_BUILD_DIR)

.PHONY: docs docs-clean


# ------------------------------------------------------------------------------
# Build installation package

SERVER_ZPKG = $(BUILD_DIR)/panda-server@$(GIT_VERSION).zpg
CONFIG_ZPKG = $(BUILD_DIR)/panda-config@$(GIT_VERSION).zpg

$(SERVER_ZPKG): $(PANDA_KO) $(SERVER) $(wildcard etc/*)

$(CONFIG_ZPKG): $(wildcard config_d/*)

$(BUILD_DIR)/%.zpg:
	etc/make-zpkg $(TOP) $(BUILD_DIR) $@

zpkg: $(SERVER_ZPKG) $(CONFIG_ZPKG)
.PHONY: zpkg


# ------------------------------------------------------------------------------

# This needs to go more or less last to avoid conflict with other targets.
$(BUILD_DIR)/%:
	mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f simserver
	find -name '*.pyc' -delete

.PHONY: clean


DEPLOY += $(PANDA_KO)
DEPLOY += $(SERVER)

deploy: $(DEPLOY)
	scp $^ root@172.23.252.202:/opt

.PHONY: deploy
