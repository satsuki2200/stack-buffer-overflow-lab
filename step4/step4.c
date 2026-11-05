#include <stdio.h>
#include <string.h>

void vulnerable(char *input) {
    char buf[16];
    strcpy(buf, input);
    printf("bufに入った: %s\n", buf);
}

int main(int argc, char **argv) {
    vulnerable(argv[1]);
    printf("main: 無事に戻ってこられた\n");
    return 0;
}
