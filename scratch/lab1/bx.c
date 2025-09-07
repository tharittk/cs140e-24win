
typedef void (*func_t)(void);

void target(void) {
    // do nothing
}

void call_indirect(void) {
    func_t f = target;
    f();  // Indirect call forces compiler to generate BX <reg>
}

