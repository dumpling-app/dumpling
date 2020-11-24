OUTPUT_ARCH(arm)

MEMORY
{
	RAMX (rx) : ORIGIN = 0x107F81C4, LENGTH = 0x0000100
}

SECTIONS
{
	.text : ALIGN(0x100) {
		build/crt0.o(.init);
		*(.text);
		*(.rodata);
	}
}