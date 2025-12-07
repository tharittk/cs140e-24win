// set the exception vector base to base
void arm_vector_set (void* base){
   asm ("MCR p15, 0, %0, c12, c0, 0":: "r" (base):);
}

// get the exception vector base
void* arm_vector_get(void){
    void* addr;
    asm ("MRC p15, 0, %0, c12, c0, 0": "=r" (addr)::);
    return addr;
}
