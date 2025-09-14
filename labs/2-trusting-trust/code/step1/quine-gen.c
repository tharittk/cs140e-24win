// convert the contents of stdin to their ASCII values (e.g., 
// '\n' = 10) and spit out the <prog> array used in Figure 1 in
// Thompson's paper.
#include <stdio.h>

int main(void) { 
    int i = 0;
    // risk buffer overflow
    char prog[5000]; // enough for this seed.c

	printf("char prog[] = {\n");
    int c;
    while ((c = getchar()) != EOF){
		printf("\t%d,%c", c, (i+1)%8==0 ? '\n' : ' ');
        prog[i] = c;
        ++i;
    }
    prog[i] = '\0';
	printf("0 };\n");
    // line below makes it so that the output file
    // is compilable C file (header, main, etc.)
	printf("%s", prog);
	return 0;
}
