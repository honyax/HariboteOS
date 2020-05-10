#include "bootpack.h"
#include <stdio.h>

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) 0x0ff0;
	
	init_gdtidt();
	init_palette();
	init_screen(binfo->vram, binfo->scrnx, binfo->scrny);
	
	putfont8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "ABC 123");
	putfont8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "HonyaOS.");
	putfont8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "HonyaOS.");
	
	char s[32];
	sprintf(s, "screen = (%d, %d)", binfo->scrnx, binfo->scrny);
	putfont8_asc(binfo->vram, binfo->scrnx, 16, 64, COL8_FFFFFF, s);
	
	char mcursor[256];
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, binfo->scrnx / 2, binfo->scrny / 2, mcursor, 16);
	
	for (;;) {
		io_hlt();
	}
}
