OUTPUT_ARCH(arm)

SECTIONS {
    . = 0x101312D0;

    .text : ALIGN(0x04) {
        build/crt0.o(.init);
        *(.text);
    }
    .rodata : {
        *(.rodata*);
    }
}

