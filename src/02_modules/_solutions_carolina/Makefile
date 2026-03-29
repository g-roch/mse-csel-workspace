# Part executed when called from kernel build system:
MODULES := mymodule07 mymodule08

ifneq ($(KERNELRELEASE),)
ifeq ($(MODULE),)
obj-m += $(addsuffix .o,$(MODULES))
else
obj-m += $(MODULE).o		## build only the requested module
endif

# Part executed when called from standard make in module source directory:
else
include ../../buildroot_path
include ../../kernel_settings
PWD := $(shell pwd)

.PHONY: all clean install install-% $(MODULES)

all:
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) modules

# Build a single module: `make mymodule07` or `make mymodule08`
$(MODULES):
	$(MAKE) -C $(KDIR) M=$(PWD) ARCH=$(CPU) CROSS_COMPILE=$(TOOLS) MODULE=$@ modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) modules_install

# Install a single module: `make install-mymodule07`
install-%:
	$(MAKE) -C $(KDIR) M=$(PWD) INSTALL_MOD_PATH=$(MODPATH) MODULE=$* modules_install

endif

