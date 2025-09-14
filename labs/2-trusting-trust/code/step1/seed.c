// can put any payload here.
#include <stdio.h>

// the C code for thompson's replicating program, more-or-less.
int main() { 
	int i;

 	// Q: why can't we just print prog twice?
	// A: printint program twice and you will get
	// two char prog[] = {...}. It cannot be compiled as
	// it is not a valid C code. The printf("%s", prog)
	// translate the char array to C code.
	// No. I think the means
	// prinf("%s", prog);
	// prinf("%s", prog);
	// That will make your program invalid as it has 2 main()
	// quine.c is not compilable then.

	printf("char prog[] = {\n");
	for(i = 0; prog[i]; i++)
		printf("\t%d,%c", prog[i], (i+1)%8==0 ? '\n' : ' ');
	printf("0 };\n");
	printf("%s", prog);
	return 0;
}