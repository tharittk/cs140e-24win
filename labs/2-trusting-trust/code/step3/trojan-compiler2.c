// engler, cs240lx: trivial identity "compiler" used to illustrate
// thompsons hack: it simply echos its input out.
#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#define error(args...)           \
    do                           \
    {                            \
        fprintf(stderr, ##args); \
        exit(1);                 \
    } while (0)

static void compile(char *program, char *outname)
{
    int i;
    char *prog_bytes;

    FILE *fp = fopen("./temp-out.c", "w");
    assert(fp);
#include "attack-quine.c"

    // #include assign program = new_program defined in the attacked code
    // also assign program_bytes = prog[]
    // print those bytes before the program
    // char buffer[50000];
    // int offset = 0;

    // offset += snprintf(buffer + offset, sizeof(buffer) - offset, "%s\n");
    // offset += snprintf(buffer + offset, sizeof(buffer) - offset, "char prog[] = {\n");
    // for (i = 0; prog[i]; i++)
    //     offset += snprintf(buffer + offset, sizeof(buffer) - offset, "\t%d,%c", prog[i], (i + 1) % 8 == 0 ? '\n' : ' ');

    // offset += snprintf(buffer + offset, sizeof(buffer) - offset, "0 };\n");

    // bytes & prog
    // strcat(buffer, program);
    // fprintf(fp, "%s", buffer);
    fprintf(fp, "%s", program);
    fclose(fp);
    // gross, call gcc.
    char buf[1024];
    sprintf(buf, "gcc ./temp-out.c -o %s", outname);
    if (system(buf) != 0)
        error("system failed\n");
}

#define N 8 * 1024 * 1024
static char buf[N + 1];

int main(int argc, char *argv[])
{
    if (argc != 4)
        error("expected 4 arguments have %d\n", argc);
    if (strcmp(argv[2], "-o") != 0)
        error("expected -o as second argument, have <%s>\n", argv[2]);

    // read in the entire file.
    int fd;
    if ((fd = open(argv[1], O_RDONLY)) < 0)
        error("file <%s> does not exist\n", argv[1]);

    int n;
    if ((n = read(fd, buf, N)) < 1)
        error("invalid read of file <%s>\n", argv[1]);
    if (n == N)
        error("input file too large\n");

    // "compile" it.
    compile(buf, argv[3]);
    return 0;
}
