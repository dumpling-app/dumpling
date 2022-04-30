OUTPUT_ARCH(arm)

SECTIONS {
    . = (0x08135000);

    .text : ALIGN(0x100) {
        build/crt0.o(.init);
        *(.text*);
    }
    .rodata : {
        *(.rodata*);
    }
    .data : {
        *(.data*);
    }
    .bss : {
        *(.bss*);
        *(COMMON*);
    }

    /DISCARD/ : {
        *(*);
    }
}

ASSERT((SIZEOF(.text) + SIZEOF(.rodata) + SIZEOF(.data) + SIZEOF(.bss)) < 0x4200, "kernel .elf is too big");