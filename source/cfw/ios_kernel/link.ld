OUTPUT_ARCH(arm)

SECTIONS
{
	. = 0x08134100;

	.text : ALIGN(0x100) {
		build/crt0.o(.init);
		*(.text);
	}
	.rodata : {
		*(.rodata*);
	}
}