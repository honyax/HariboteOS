// 割り込み関係

#include <stdio.h>
#include "bootpack.h"

// PICの初期化
void init_pic(void)
{
	io_out8(PIC0_IMR,  0xff  ); // 全ての割り込みを受け付けない
	io_out8(PIC1_IMR,  0xff  ); // 全ての割り込みを受け付けない
	
	io_out8(PIC0_ICW1, 0x11  ); // エッジトリガモード
	io_out8(PIC0_ICW2, 0x20  ); // IRQ0-7は、INT20-27で受ける
	io_out8(PIC0_ICW3, 1 << 2); // PIC1はIRQ2にて接続
	io_out8(PIC0_ICW4, 0x01  ); // ノンバッファモード
	
	io_out8(PIC1_ICW1, 0x11  ); // エッジトリガモード
	io_out8(PIC1_ICW2, 0x28  ); // IRQ8-15は、INT28-2fで受ける
	io_out8(PIC1_ICW3, 2     ); // PIC1はIRQ2にて接続
	io_out8(PIC1_ICW4, 0x01  ); // ノンバッファモード
	
	io_out8(PIC0_IMR,  0xfb  ); // 11111011 PIC1以外は全て禁止
	io_out8(PIC1_IMR,  0xff  ); // 11111111 全ての割り込みを受け付けない
}

struct KEYBUF keybuf;

// PS/2キーボードからの割り込み
void inthandler21(int *esp)
{
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);	// IRQ-01受付完了をPICに通知
	data = io_in8(PORT_KEYDAT);
	if (keybuf.flag == 0) {
		keybuf.data = data;
		keybuf.flag = 1;
	}
}

// PS/2マウスからの割り込み
void inthandler2c(int *esp)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
	for (;;) {
		io_hlt();
	}
}

// PIC0からの不完全割り込み対策
void inthandler27(int *esp)
{
	// Athlon64X2機などではチップセットの都合によりPICの初期化時にこの割り込みが1度だけおこる
	// この割り込み処理関数は、その割り込みに対して何もしないでやり過ごす
	// なぜ何もしなくていいの？
	//	→  この割り込みはPIC初期化時の電気的なノイズによって発生したものなので、
	//		まじめに何か処理してやる必要がない。
	
	// IRQ-07受付完了をPICに通知
	io_out8(PIC0_OCW2, 0x67); 
}