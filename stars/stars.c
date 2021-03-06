#include "apilib.h"

// 0~32767の範囲で乱数を発生
int rand(void);

void HariMain(void)
{
    char *buf;
    int win, i;
    int x, y;
    api_initmalloc();
    buf = api_malloc(150 * 100);
    win = api_openwin(buf, 150, 100, -1, "stars");
    api_boxfilwin(win,  6, 26, 143, 93, 0 /* black */);
    for (i = 0; i < 50; i++) {
        x = (rand() % 137) +  6;
        y = (rand() %  67) + 26;
        api_point(win, x, y, 3 /* yellow */);
    }
    api_end();
}
