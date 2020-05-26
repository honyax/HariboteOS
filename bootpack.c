#include "bootpack.h"
#include <stdio.h>

struct MOUSE_DEC {
	unsigned char buf[3];
	unsigned char phase;
	int x, y;
	int btn;
};

extern struct FIFO8 keyfifo;
extern struct FIFO8 mousefifo;
void enable_mouse(struct MOUSE_DEC *mdec);
void init_keyboard(void);
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	struct MOUSE_DEC mdec;

	int xmin = 0;
	int ymin = 0;
	int xmax = binfo->scrnx - 16;
	int ymax = binfo->scrny - 16;
	
	init_gdtidt();
	init_pic();
	// IDT/PICの初期化が終わったのでCPUの割込み禁止を解除
	io_sti();
	
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9);	// 11111001 PIC1とキーボード(IRQ1)を許可
	io_out8(PIC1_IMR, 0xef);	// 11101111 マウス(IRQ12)を許可
	
	init_keyboard();

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	enable_mouse(&mdec);
	
	for (;;) {
		io_cli();
		if (fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0) {
			io_stihlt();
		} else {
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 16, 15, 31);
				putfonts8_asc(binfo->vram, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if (mouse_decode(&mdec, i) != 0) {
					// データが3バイト揃ったので表示
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					if ((mdec.btn & 0x01) != 0) {
						s[1] = 'L';
					}
					if ((mdec.btn & 0x02) != 0) {
						s[3] = 'R';
					}
					if ((mdec.btn & 0x04) != 0) {
						s[2] = 'C';
					}
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(binfo->vram, binfo->scrnx, 32, 16, COL8_FFFFFF, s);

					// マウスカーソルの移動
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);	// マウスを消す
					mx += mdec.x;
					my += mdec.y;
					if (mx < xmin) { mx = xmin; }
					if (my < ymin) { my = ymin; }
					if (mx > xmax) { mx = xmax; }
					if (my > ymax) { my = ymax; }

					sprintf(s, "(%3d, %3d)", mx, my);
					boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 0, 0, 79, 15);	// 前回の座標を消す
					putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);	// 座標を書く
					putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);	// マウス描画
				}
			}
		}
	}
}

#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void wait_KBC_sendready(void)
{
	// キーボードコントローラがデータ送信可能になるのを待つ
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(void)
{
	// キーボードコントローラの初期化
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
}

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4

void enable_mouse(struct MOUSE_DEC *mdec)
{
	// マウス有効
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	// うまくいくとACK(0xfa)が送信されてくる
	mdec->phase = 0; // マウスの0xfaを待っている段階
}

int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	switch (mdec->phase) {
		case 0:
			// マウスの0xfaを待っている段階
			if (dat == 0xfa) {
				mdec->phase = 1;
			}
			return 0;
		case 1:
			// マウスの1バイト目を待っている段階
			if ((dat & 0xc8) == 0x08) {
				// 正しい1バイト目だった
				mdec->buf[0] = dat;
				mdec->phase = 2;
			}
			return 0;
		case 2:
			// マウスの2バイト目を待っている段階
			mdec->buf[1] = dat;
			mdec->phase = 3;
			return 0;
		case 3:
			// マウスの3バイト目を待っている段階
			mdec->buf[2] = dat;
			mdec->phase = 1;

			mdec->btn = mdec->buf[0] & 0x07;
			mdec->x = mdec->buf[1];
			mdec->y = mdec->buf[2];
			if ((mdec->buf[0] & 0x10) != 0) {
				mdec->x |= 0xffffff00;
			}
			if ((mdec->buf[0] & 0x20) != 0) {
				mdec->y |= 0xffffff00;
			}
			mdec->y = -(mdec->y); // マウスではy方向の符号が画面と反対

			// データが3バイト揃ったので表示
			return 1;
	}
	// ここに来ることはないはず
	return -1;
}
