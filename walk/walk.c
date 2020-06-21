#include "apilib.h"

void HariMain(void)
{
    char *buf;
    int win, i;
    int x, y;
    api_initmalloc();
    buf = api_malloc(160 * 100);
    win = api_openwin(buf, 160, 100, -1, "walk");
    api_boxfilwin(win, 4, 24, 155, 95, 0 /* black */);
    x = 76;
    y = 56;
    api_putstrwin(win, x, y, 3 /* yellow */, 1, "*");
    for (;;) {
        i = api_getkey(1);
        // 黒で消す
        api_putstrwin(win, x, y, 0 /* yellow */, 1, "*");
        if (i == 0x0a) {
            // Enterで終了
            break;
        }
        switch (i) {
        case '4':
            if (x > 4) {
                x -= 8;
            }
            break;
        case '6':
            if (x < 148) {
                x += 8;
            }
            break;
        case '8':
            if (y > 24) {
                y -= 8;
            }
            break;
        case '2':
            if (y < 80) {
                y += 8;
            }
            break;
        }
        api_putstrwin(win, x, y, 3 /* yellow */, 1, "*");
    }
    api_closewin(win);
    api_end();
}
