OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = 0x0510E570, LENGTH = 0x00015BC
	RAMRW (rw!i) : ORIGIN = 0x05089780, LENGTH = 0x00001F00
}

SECTIONS
{
	.text : {
		build/crt0.o(.init);
		*(.text);
		*(.rodata);
	}

	.bss : {
		_bss_start = .;
		*(.bss);
	}
	_bss_end = .;
}
