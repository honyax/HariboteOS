#include "bootpack.h"
#include <stdio.h>

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256];
	int mx, my;
	
	init_gdtidt();
	init_pic();
	// IDT/PICの初期化が終わったのでCPUの割込み禁止を解除
	io_sti();
	
	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	//putfonts8_asc(binfo->vram, binfo->scrnx, 8, 8, COL8_FFFFFF, "ABC 123");
	//putfonts8_asc(binfo->vram, binfo->scrnx, 31, 31, COL8_000000, "HonyaOS.");
	//putfonts8_asc(binfo->vram, binfo->scrnx, 30, 30, COL8_FFFFFF, "HonyaOS.");
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);
	
	// 11111001 PIC1とキーボード(IRQ1)を許可
	io_out8(PIC0_IMR, 0xf9);
	// 11101111 マウス(IRQ12)を許可
	io_out8(PIC1_IMR, 0xef);
	
	for (;;) {
		io_hlt();
	}
}
