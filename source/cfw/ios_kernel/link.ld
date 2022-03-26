OUTPUT_ARCH(arm)

MEMORY {
    RAMX (rx) : ORIGIN = 0x08135000, LENGTH = 0x000B000
}

SECTIONS {
    . = 0x08135000;

    .text : ALIGN(0x100) {
        build/crt0.o(.init);
        *(.text);
    }
    .rodata : {
        *(.rodata*);
    }

    /DISCARD/ : {
        *(*);
    }
}