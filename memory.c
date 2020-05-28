#include "bootpack.h"

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

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)
{
    unsigned int a;
    size = (size + 0xfff) & 0xfffff000;
    a = memman_alloc(man, size);
    return a;
}

int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)
{
    int i;
    size = (size + 0xfff) & 0xfffff000;
    i = memman_free(man, addr, size);
    return i;
}
