#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h> 

int main(int argc, char** argv) {

   

    FILE* dbg = fopen("aoutran", "w+");

    fprintf(stderr, "%s ran\n", argv[0]);
    printf("Exec test binary\nUsage: %s fd-child-read fd-child-write\n", argv[0]);
    for (int i = 0; i < argc; ++i) printf("Arguments[%d]: %s\n", i,  argv[i]);

    if (argc != 3) return 1;

    int childread = 0, childwrite = 0;
    
    if (!sscanf(argv[1], "%d", &childread) || !sscanf(argv[2], "%d", &childwrite)) return 2;

    printf("Arguments parsed, attempting to open fds read=%d, write=%d\n", childread, childwrite);

    FILE* fr = fdopen(childread, "rb");
    FILE* fw = fdopen(childwrite, "wb");

    // attempt to read a line from read and write it back to write

    char buf[1024];

    if (fgets(buf, 1023, fr)) {
        int len = strlen(buf);
        if (len > 1023) len = 1023;
        for (int i = 0; i < len; ++i) buf[i] = (char)toupper((int)buf[i]);
        buf[len] = '\0';
        fprintf(fw, "%s\n", buf);
        fprintf(dbg, "%s\n", buf);
    }

    fflush(fw);
    fclose(fr);
    fclose(fw);

    fclose(dbg);
   
    printf("\n");
    fflush(stdout);
    return 0;
}
