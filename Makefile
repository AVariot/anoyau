# Toolchain (kernel 64 bits, avec les outils du système)
CC      := gcc
LD      := ld
AS      := nasm
QEMU    := qemu-system-x86_64

# Tous les dossiers "include" de src/ sont ajoutés au chemin de recherche (-I)
INCDIRS := $(shell find src -type d -name include)
INCLUDES := $(addprefix -I,$(INCDIRS))

# Options d'un kernel 64 bits compilé avec le gcc du système :
#   -mno-red-zone         une interruption écraserait les 128 octets sous rsp
#   -mgeneral-regs-only   pas de registres SSE (non activés dans le kernel)
#   -fno-pie -fno-pic     adresse fixe, pas de code position-indépendant
#   -fno-stack-protector -fcf-protection=none
#                         protections activées par défaut sur Arch, qui supposent une libc
ARCH_CFLAGS := -m64 -mno-red-zone -mgeneral-regs-only -fno-pie -fno-pic \
               -fno-stack-protector -fcf-protection=none

# -MMD -MP : génère les .d (dépendances vers les .h) pour recompiler si un header change
CFLAGS  := -std=gnu11 -ffreestanding -O2 -Wall -Wextra $(ARCH_CFLAGS) $(INCLUDES) -MMD -MP

# make test : build séparé (build-test/) avec -DRUN_TESTS, lance les tests au boot
ifdef TESTS
CFLAGS  += -DRUN_TESTS
endif

ASFLAGS := -f elf64
LDFLAGS := -m elf_x86_64 -z max-page-size=0x1000 -T linker.ld -nostdlib

BUILD   := build
ISODIR  := $(BUILD)/iso
KERNEL  := $(BUILD)/kernel.elf
ISO     := $(BUILD)/anoyau.iso

C_SRCS   := $(shell find src -name '*.c')
ifndef TESTS
C_SRCS   := $(filter-out src/tests/%,$(C_SRCS))
endif
ASM_SRCS := $(shell find src -name '*.asm')
OBJS     := $(C_SRCS:src/%.c=$(BUILD)/%.o) $(ASM_SRCS:src/%.asm=$(BUILD)/%.o)

.PHONY: all check iso run debug verify test test-debug clean

all: $(KERNEL)

# Étape 0 : vérifie que tous les outils sont présents
check:
	@for t in $(CC) $(LD) $(AS) $(QEMU) grub-mkrescue xorriso gdb; do \
		command -v $$t >/dev/null && echo "ok      $$t" || echo "MANQUE  $$t"; \
	done
	@$(CC) --version | head -1

-include $(OBJS:.o=.d)

# Un changement de Makefile (options) recompile tout : on ne mélange jamais du 32 et du 64 bits
$(OBJS): Makefile

$(BUILD)/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD)/%.o: src/%.asm
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(KERNEL): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) $(OBJS) -o $@

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
	readelf -l $(KERNEL)

# Kernel + tests (src/tests/), dans build-test/ pour ne pas mélanger avec build/
test:
	$(MAKE) BUILD=build-test TESTS=1 run

test-debug:
	$(MAKE) BUILD=build-test TESTS=1 debug

clean:
	rm -rf build build-test
