OUTPUT_ARCH(arm)

PROVIDE(printf = 0x107f0c84);

SECTIONS {
    .text (0x10700000 + 0x000F8200) : {
        _fs_text_start = .;
        *(.text*);
        *(.rodata*);
    }
    _fs_text_end = .;

    .bss (0x10835000 + 0x1406554) : {
        _fs_bss_start = .;
        *(.bss*);
        *(COMMON);
    }
    .io_buffer : ALIGN(0x40) {
        *(.io_buffer*);
    }
    _fs_bss_end = .;

    /DISCARD/ : {
        *(*);
    }
}

ASSERT((SIZEOF(.text)) < 0x7E00, "FS text section is too big");
ASSERT((SIZEOF(.bss)) < 0x2C4AAC, "FS bss section is too big");