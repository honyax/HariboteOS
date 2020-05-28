#include "bootpack.h"
#include <stdio.h>

#define MEMMAN_FREES		4090		// これで約32KB
#define MEMMAN_ADDR			0x003c0000

// 空き情報
struct FREEINFO {
	unsigned int addr;
	unsigned int size;
};

// メモリ管理
struct MEMMAN {
	int frees;
	int maxfrees;
	int lostsize;
	int losts;
	struct FREEINFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], mcursor[256], keybuf[32], mousebuf[128];
	int mx, my, i;
	unsigned int memtotal;
	struct MOUSE_DEC mdec;
	struct MEMMAN *memman = (struct MEMMAN *)MEMMAN_ADDR;

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
	enable_mouse(&mdec);
	memtotal = memtest(0x00400000, 0xbfffffff);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000);	// 0x00001000 - 0x0009efff
	memman_free(memman, 0x00400000, memtotal - 0x00400000);

	init_palette();
	init_screen8(binfo->vram, binfo->scrnx, binfo->scrny);
	mx = (binfo->scrnx - 16) / 2;
	my = (binfo->scrny - 28 - 16) / 2;
	init_mouse_cursor8(mcursor, COL8_008484);
	putblock8_8(binfo->vram, binfo->scrnx, 16, 16, mx, my, mcursor, 16);
	sprintf(s, "(%3d, %3d)", mx, my);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, s);

	sprintf(s, "memory %dMB   free : %dKB", memtotal / (1024 * 1024), memman_total(memman) / 1024);
	
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	
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

#define EFLAGS_AC_BIT			0x00040000
#define CR0_CACHE_DISABLE		0x60000000

unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflg, cr0, i;

	// 386 or 486以降の確認
	eflg = io_load_eflags();
	// AC-bit = 1
	eflg |= EFLAGS_AC_BIT;
	io_store_eflags(eflg);
	eflg = io_load_eflags();
	// 386では、AC-bitを立てても自動でOFFになる
	if ((eflg & EFLAGS_AC_BIT) != 0) {
		flg486 = 1;
	}
	// AC-bit = 0
	eflg &= ~EFLAGS_AC_BIT;
	io_store_eflags(eflg);

	if (flg486 != 0) {
		cr0 = load_cr0();
		// キャッシュ禁止
		cr0 |= CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);

	if (flg486 != 0) {
		cr0 = load_cr0();
		// キャッシュ許可
		cr0 &= ~CR0_CACHE_DISABLE;
		store_cr0(cr0);
	}

	return i;
}

void memman_init(struct MEMMAN *man)
{
	man->frees = 0;		// 空き情報の個数
	man->maxfrees = 0;	// 状況観察用：freesの最大値
	man->lostsize = 0;	// 解放に失敗した合計サイズ
	man->losts = 0;		// 解放に失敗した回数
}

// 空きサイズの合計を報告
unsigned int memman_total(struct MEMMAN *man)
{
	unsigned int i, t = 0;
	for (i = 0; i < man->frees; i++) {
		t += man->free[i].size;
	}
	return t;
}

// 確保
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)
{
	unsigned int i, a;
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].size >= size) {
			// 十分な広さのある空きを発見
			a = man->free[i].addr;
			man->free[i].addr += size;
			man->free[i].size -= size;
			if (man->free[i].size == 0) {
				// free[i]がなくなったので前へつめる
				man->frees--;
				for (; i < man->frees; i++) {
					man->free[i] = man->free[i + 1];	// 構造体の代入
				}
			}
			return a;
		}
	}
	return 0;	// 空きがない
}

// 解放
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
	int i, j;
	int mergefront = 0;
	int mergeback = 0;
	// まとめやすさを考えると、free[]がaddr順に並んでいる方が良い
	// だからまず、どこに入れるべきかを決める
	for (i = 0; i < man->frees; i++) {
		if (man->free[i].addr > addr) {
			break;
		}
	}

	// free[i - 1].addr < addr < free[i].addr
	// 前にまとめられるか？
	if (i > 0 && man->free[i - 1].addr + man->free[i - 1].size == addr) {
		mergefront = 1;
	}
	// 後ろにまとめられるか？
	if (i < man->frees && addr + size == man->free[i].addr) {
		mergeback = 1;
	}

	if (mergefront == 1) {
		// 前とまとめる
		man->free[i - 1].size += size;

		if (mergeback == 1) {
			// さらに後ろとまとめる
			man->free[i - 1].size += man->free[i].size;

			// i 以降を前につめる
			man->frees--;
			for (; i < man->frees; i++) {
				man->free[i] = man->free[i + 1];	// 構造体の代入
			}
		}
		return 0;		// 成功終了
	} else if (mergeback == 1) {
		// 後ろとまとめる
		man->free[i].addr = addr;
		man->free[i].size += size;
		return 0;		// 成功終了
	} else if (man->frees < MEMMAN_FREES) {
		// 前とも後ろともまとめられないので、i 以降を後ろにズラす
		for (j = man->frees; j > i; j--) {
			man->free[j] = man->free[j -  1];
		}
		man->frees++;
		if (man->maxfrees < man->frees) {
			man->maxfrees = man->frees;		// 最大値を更新
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;		// 成功終了
	} else {
		// 前とも後ろともまとめられない上に後ろにズラせない。ひとまずロストさせる
		man->losts++;
		man->lostsize += size;
		return -1;		// 失敗終了
	}
}
