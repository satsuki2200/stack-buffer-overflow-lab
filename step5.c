#include <stdio.h>
#include <unistd.h>

void win(void) {
    printf("\n*** 乗っ取り成功！本来呼ばれないwin()が動いた ***\n");
}

void vulnerable(void) {
    char buf[16];
    read(0, buf, 200);
}

int main(void) {
    vulnerable();
    printf("main: 普通に終了\n");
    return 0;
}
