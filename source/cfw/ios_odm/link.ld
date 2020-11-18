OUTPUT_ARCH(arm)

SECTIONS
{
	. = 0x107F81C4;

	.text : ALIGN(0x100) {
		build/crt0.o(.init);
		*(.text);
		*(.rodata);
	}
}