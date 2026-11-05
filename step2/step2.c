#include <stdio.h>

int main(void) {
    char c = 'A';
    printf("中身=%c  番地=%p\n", c, &c);
    return 0;
}
