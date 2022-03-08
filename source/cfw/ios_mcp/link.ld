OUTPUT_ARCH(arm)

SECTIONS
{
	.text 0x05116000 : {
		_text_start = .;
		_mcp_start = .;
		*(.text*)
		*(.rodata*)
	}
    _text_end = .;

	.bss 0x050BD000 : {
		_bss_start = .;
		*(.bss*);
	}
	_bss_end = .;

	/DISCARD/ : {
		*(*);
	}
}

ASSERT((SIZEOF(.text)) < 0x3F00, "text section is too big");
ASSERT((SIZEOF(.bss)) < 0x3000, "bss section is too big");
