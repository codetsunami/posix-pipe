#include <stdio.h>

int main() {
    char buf[1024];

    while (!feof(stdin)) {
        if (!fgets(buf, 1023, stdin)) return 1;

        for (char* x = buf; *x; ++x) if (*x != '\0' && *x != '\n') *x+=1;

        printf("%s", buf);
        fflush(stdout);
    }

    return 0;
}
