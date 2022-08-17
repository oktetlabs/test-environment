# SPDX-License-Identifier: Apache-2.0
# Copyright (C) 2006-2022 OKTET Labs Ltd. All rights reserved.

ifneq ($(KERNELRELEASE),)
obj-m  := dl2k.o
# dl2k-y := dl2k_mod.o
else
default: modules
modules:
	$(MAKE) -C $(KERNEL_DIR) M=`pwd` modules
endif
