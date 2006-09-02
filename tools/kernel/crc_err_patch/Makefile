
ifneq ($(KERNELRELEASE),)
obj-m  := dl2k.o
# dl2k-y := dl2k_mod.o
else
default: modules
modules:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` modules
endif
