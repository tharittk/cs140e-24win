unsigned get32(void* addr) {
	return *(unsigned *)addr;
}

void put32(void *addr, unsigned val){
	*(unsigned*)addr = val;
}

char get8(void* addr){
	return *(char *)addr;
}

void put8(void* addr, char val){
	*(char*)addr = val;
}
