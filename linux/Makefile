libudis86-objs := libudis86/decode.o libudis86/input.o libudis86/itab.o libudis86/syn-att.o \
	          libudis86/syn.o libudis86/syn-intel.o libudis86/udis86.o

obj-m := vmxice.o
vmxice-objs := $(libudis86-objs) \
		main.o vmx-x86.o vmx.o vmm.o video.o dbgprint.o console.o apic.o keyboard.o scancode.o dbg.o codeview.o mmu.o \
		sym.o exp.o vmmstring.o

CURRENT_PATH := $(shell pwd)
LINUX_KERNEL := $(shell uname -r)
LINUX_KERNEL_PATH := /usr/src/linux-headers-$(LINUX_KERNEL)


all:
	yasm -felf vmx-x86.asm
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules

clean:
	make -C $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean

