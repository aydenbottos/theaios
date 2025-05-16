CROSS = i686-elf-
AS      = nasm
CC      = $(CROSS)gcc
LD      = $(CROSS)ld
OBJCOPY = $(CROSS)objcopy

CFLAGS = -Wall -Wextra -std=gnu99 -ffreestanding -O2 \
         -fno-stack-protector -fno-exceptions -fno-pic
LDFLAGS = -T linker.ld -nostdlib

ASM_SRC = $(wildcard src/*.s)
ASM_OBJ = $(ASM_SRC:.s=.asm.o)
SRC_C   = $(filter-out src/boot.s,$(wildcard src/*.c))
C_OBJ   = $(SRC_C:.c=.o)
OBJ     = $(ASM_OBJ) $(C_OBJ)

all: kernel.bin

%.asm.o: %.s
	$(AS) -f elf32 $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

kernel.bin: $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $^
	$(OBJCOPY) -O elf32-i386 $@ $@

iso: kernel.bin
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/
	cp grub/grub.cfg isodir/boot/grub/
	grub-mkrescue -o myos.iso isodir >/dev/null 2>&1
	@echo "ISO ready: myos.iso"

run: iso
	qemu-system-i386 -cdrom myos.iso

clean:
	rm -rf isodir *.iso kernel.bin src/*.o
