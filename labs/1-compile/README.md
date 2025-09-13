# Compilation lab

Unlike all subsequent labs, our first two don't use hardware.  They should
give a good feel for whether the class works for you without requiring
a hardware investment.

Today's short lab focuses on what happens when you compile code.  How to
see how the compiler translated your C code by examining the machine code
it produced.  And some some of the subtle rules for how the compiler can
do this translation.  What we cover will apply to every lab this quarter.
   - The `docs` directory has useful, further readings.

The next lab will dive into subtle inductive magic tricks that
arise when you compile a compiler using itself.  FWIW: in the past,
this lab was by far the favorite in the class post mortem.

What to do:

  1. Check out the class repository from github and setup the gcc
     cross-compiler.  See below.

  2. Read through the note on 
     [using gcc to figure out assembly code](../../notes/using-gcc-for-asm/README.md) and work through the examples.

     The ability to look at assembly to see how the compiler transformed
     your code (more on this in the next bullet) or to see how to do an
     act in assembly is a skill you'll use all quarter.  Not doing it
     was a pretty common initial mistake we saw last year.

     Many people get stuck really easily when they have to figure out
     how to write assembly code (reasonable since its new).  A simple
     hack to answering "how do I do X in ARM assembly?" is to do X
     in C code, use `gcc` to compile the C code to ARM machine code,
     and look at the result.  Using the compiler with active aggressive
     versus passively sitting and thinking will save you a bunch of time
     (not just in this class.)

  3. Read through the note on observability and compiler optimization:
     [observability](../../notes/observability/README.md) and work
     through the examples in `examples-pointer` and `examples-volatile`
     and then answer a few questions below.

     You should re-read this note carefully outside of class (several
     times!)  The compiler has very strong ideas about when and who
     can observe changes it makes to code.  Hardware devices typically
     wildly violate these assumptions.  Some of the hardest bugs that
     arise in the class (and IRL) come from ignorance of how and why
     these violations cause problems.

  
Checkoff:

  - Sections 2 and 3 below have some questions you should answer and check
    off with a CA.

  - Feel free to work with another person and checkoff as a team, but
    you should both have code written on your own laptops and be able
    to answer each on your own.

  - Simple extension: write some C examples that show the compiler
    doing something interesting / surprising.  E.g., write C code
    that shows off fancy optimizations the compiler does.

-------------------------------------------------------------------
## 1. install the gcc tool chain

### macOS

Use the [cs107e install notes](https://web.archive.org/web/20210414133806/http://cs107e.github.io/guides/install/mac/).
Note: do not install the python stuff. We will use their custom brew formula!

If you get an error that it can't find `string.h`, you want to set `CPATH`
to the empty string (see a TA for help if you need it).

### Linux

For [ubuntu/linux](https://askubuntu.com/questions/1243252/how-to-install-arm-none-eabi-gdb-on-ubuntu-20-04-lts-focal-fossa), ARM recently
changed their method for distributing the tool change. Now you
must manually install. As of this lab, the following works:

        wget https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2

        sudo tar xjf gcc-arm-none-eabi-10.3-2021.10-x86_64-linux.tar.bz2 -C /usr/opt/

We want to get these binaries on our `$PATH` so we don't have to type the
full path to them every time. There's a fast and messy option, or a slower
and cleaner option.

The fast and messy option is to add symlinks to these in your system `bin`
folder:

        sudo ln -s /usr/opt/gcc-arm-none-eabi-10.3-2021.10/bin/* /usr/bin/

The cleaner option is to add `/usr/opt/gcc-arm-none-eabi-10.3-2021.10/bin` to
your `$PATH` variable in your shell configuration file (e.g., `.zshrc` or
`.bashrc`), save it, and `source` the configuration. When you run:

        arm-none-eabi-gcc
        arm-none-eabi-ar
        arm-none-eabi-objdump

You should not get a "Command not found" error.

If gcc can't find header files, try:

       sudo apt-get install libnewlib-arm-none-eabi

-------------------------------------------------------------------
## 2. Use `gcc` to figure out assembly.


You should answer these questions for checkoff.  In each case: 
write a bit of C code, and be prepared to explain how the machine code
it produces shows your answer is correct.
  1. What register holds a pointer (not integer) return value?
	foo.c: r0 - you can see in foo.s that the return value is moved to r0
  2. What register holds the third pointer argument to a routine?
	int rptr3.c: there is a store r2, [fp -16] which is the address of 3rd pointer.
	Later, there is a ldr r3, [fp-16] which load that address back to r3 and ldr r3, [r3]
	- effectively deference to the underlying int.
	By the way, ARM leaves space one the stack = #args + 1. The +1 is the saved return adddress - like the LINK command in SaM in your compiler class. That is why the first argument is from (fp-4 ... `fp-8`]
  3. If register `r1` holds a memory address (a pointer), 
     what instruction do you use to store an 8-bit integer to that location?
	loadstore.c: The argument char 8 bit is at fp-9 while the addr is (fp-4...fp-8], you can see we load that char to r2, zero-ext it, load value of target addr to r3 and then do
	store r2, [r3] <- taking addr at r3 (which refers to memory) and store r2 at it.
  4. Same as (3) but for load?
	ldr r3, [r3] , taking addr at r3 and fetch memory from that and store at r3
  5. Load/store 16-bit?
	Same as above - with zero-extension required.
  6. Load/store 32-bit?
	Same as above but 32-bit does not require zero-extension.
  7. Write some C code that will cause the compiler to emit a `bx`
     instruction that *is not* a function return.
	bx.c: We put the function pointer (represent as the label: in assembly) to a register , says r3, i.e., assigning it a variable in c code var = f //. Now if we do var() i.e., evaluate the function, the assembly code will do bx r3
  8. What does an infinite loop look like?  Why?
	infinite.c: you have a fall through assembly, cmp and branch instruction based on the loop predicate (if true will go to the backedge. Othewise, fall through and leave).
  9. How do you call a routine whose address is in a register rather
     than a constant?  (You'll use this in the threads lab.)
	I speculate that this is about the system call in which the address of the instruction is at the "known address". I think we just ldr ri, [known addr]. and bx ri. In this case, the address is already at the register so there is no need for load. There is a difference between bx (tail call) and blx (call with link - which saves return address in lr) - in SaM, you do JUMPIND (which takes the saved PC at the top of the stack and set PC <- TOS). You did this when wanting to return from a method invocation so that you continue where you left off.

Finally implement the following routines. (Note: you should cheat by 
using the compiler as above!)

  1. Implement a an assembly routine `unsigned GET32(void *addr)` that
     does a 32-bit load of the memory pointed to by address `addr`
     and returns it.  Put your implementation in a `mem-op.S` assembly
     file and make sure it compiles with your `arm-none-eabi-gcc`.

            # compile but don't link
            % arm-none-eabi-gcc -c mem-op.S
            # disassemble.
            % arm-none-eabi-objdump -d mem-op.o
 
  2. Also implement `GET16` and `GET8`.

  3. Similarly, write an assembly routine 
     `void PUT32(void *addr, unsigned val)` 
     that stores the 32-bit quantity `val` into the memory pointed
     to by address `addr`.  Also put this in `mem-op.S` and make sure
     it compiles.

  4. Also implement `PUT16` and `PUT8`.
	Done in mem-op.c. Looks like casting from void -> unsigned is for the compiler i.e., you don't see `casting` in assembly.
	
	The 32-bit address points to 1 byte value in memory. Meaning when you do int* p and *p, you are taking 4 consecutive address (sizeof(int) = 4), and reconstruct integer based from those 8 x 4 bits. Same thing for char* but here you reconstruct only from 1 address. That is why the pointer size = register size (does not matter what underlying data is, char, int, double the addr is 32 bit per byte). 
	When you allocate a lot of char, it doesn't mean that you are wasting 32-bit address for every 8 bit store. You are wasting that much if only you ask for the `pointer` to each char (you never do). Usually you just ask for the head pointer of the consective char and do ++ptr.

-------------------------------------------------------------------
## 3. Observability.

### Questions about example-volatile.

For each of the following: make a copy of the file in question, do
the modification and be able to explain how the machine code shows
you are right:

  1. Give two different fixes for `2-wait.c` and check that they work.
	Mark mbox->status as volatile
	Add asm volatile ("" ::: "memory") to the instruction inside the while loop.
	This forces the the program to perform fresh read despite not marking status
	as volatile.
  2. Which of the two files `4-fb.c` and `5-fb.c` (or: both, neither) 
     have a problem?  What is it?  If one does and not the other, why?
     Give a fix.
		
	Actually, 4-fb.c should fail due to compiler reordering the instructions.
	If we run gcc with, says -O3, we will observe that, for example, only
	the instruction that store height=960 and width=1280 gets
	store ri, [addr] before hte mbox->write is executed (compiler reorder).
	This will cause the problem with peripheral that read the data as the
	whole `struct` is not appropriately written (offset, etc instructions come after
	mbox->write).	 

### Questions about example-pointer.

For all the files, make sure you can answer the questions in comments
using the generated machine code and explain why.


In addition, you should be able to answer the following questions from
a previous 240 exam:

  1.  Which (if any) assignments can the compiler remove from the code
      in this file?
    
            /* foo.c start */
            static int lock = 0, cnt = 0;

            void bar(void);

            void foo(int *p) {              
                cnt++;                      // line 1
                lock = 1;                   // line 2
                bar();                      // line 3
                lock = 0;                   // line 4
                return;                
            }
            /* foo.c end */
			None. lock and cnt is static and bar() may access it.
			Currrently, the bar definition is opaque so the compiler
			must be conservative. If it can prove that bar does not
			access lock or cnt. It may optimize this away (but not guarantee).
			Imagine that some parts of the program may have the access to
			the pointer of lock and/or count (not lock and count directly because
			they are static). In this case, changes at these memory locaitons are
			observable.

   2. The compiler analyzes `foo` in isolation, can it
      reorder or remove any of the following assignments?

            void foo(int *p, int *q) {
                *q = 1;
                *p = 2;
                *q = 3;
                return;j
            }
			If it can prove that q != p. *q=1 can be eliminated.
			If q=p then only *q=3 is retained.

    3. How much of this code can gcc remove?  (Give your intuition!)

            #include <stdlib.h>
            int main(void) {
                int *p = malloc(4);
                *p = 10;
                return 0;
            }
			Nobody can access int* p. So the *p=10 is deadstore.
			The compiler may get rid of malloc completely (the bonus is
			we don't have memory leak despite not calling free(p))).

-------------------------------------------------------------------
## Post-script

We did a quick drive by through massive compiler topics.  While real
compilers can seem (and often are) complicated, they also expose
their results fairly easily.  The habit of looking at what they did,
reasoning about why, and figuring out how to answer questions by feeding
them intentionally crafted inputs will serve you well in the class and
future endeavors.

In closing, heh:

<p align="center">
  <img src="docs/compile.jpg" width="450" />
</p>

