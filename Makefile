# Toolchain
CC      := i686-elf-gcc
LD      := i686-elf-ld
AS      := nasm
QEMU    := qemu-system-i386

# Tous les dossiers "include" de src/ sont ajoutés au chemin de recherche (-I)
INCDIRS := $(shell find src -type d -name include)
INCLUDES := $(addprefix -I,$(INCDIRS))

# -MMD -MP : génère les .d (dépendances vers les .h) pour recompiler si un header change
CFLAGS  := -std=gnu11 -ffreestanding -O2 -Wall -Wextra -m32 $(INCLUDES) -MMD -MP
ASFLAGS := -f elf32
LDFLAGS := -T linker.ld -nostdlib

BUILD   := build
ISODIR  := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf
ISO     := $(BUILD)/anoyau.iso

C_SRCS   := $(shell find src -name '*.c')
ASM_SRCS := $(shell find src -name '*.asm')
OBJS     := $(C_SRCS:src/%.c=$(BUILD)/%.o) $(ASM_SRCS:src/%.asm=$(BUILD)/%.o)

.PHONY: all check iso run debug verify clean

all: $(KERNEL)

# Étape 0 : vérifie que tous les outils sont présents
check:
	@for t in $(CC) $(LD) $(AS) $(QEMU) grub-mkrescue xorriso gdb; do \
		command -v $$t >/dev/null && echo "ok      $$t" || echo "MANQUE  $$t"; \
	done
	@$(CC) --version | head -1

-include $(OBJS:.o=.d)

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -m elf_i386 $(OBJS) -o $@

# ISO bootable avec GRUB
$(ISO): $(KERNEL) grub.cfg
	@mkdir -p $(ISODIR)/boot/grub
	cp $(KERNEL) $(ISODIR)/boot/kernel.elf
	cp grub.cfg $(ISODIR)/boot/grub/grub.cfg
	grub-mkrescue -o $@ $(ISODIR)

iso: $(ISO)

run: $(ISO)
	$(QEMU) -cdrom $(ISO)

# QEMU en pause, serveur GDB sur :1234 -> dans gdb : target remote :1234
debug: $(ISO)
	$(QEMU) -cdrom $(ISO) -s -S

# Vérifie l'ELF : header Multiboot valide + adresse de chargement
verify: $(KERNEL)
	grub-file --is-x86-multiboot $(KERNEL) && echo "multiboot: ok"
	i686-elf-readelf -l $(KERNEL)

clean:
	rm -rf $(BUILD)
