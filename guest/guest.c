#include <stddef.h>
#include <stdint.h>

void
__attribute__((noreturn))
_start(void) {
	const char *p;

	uint16_t port = 0xE9;
	for (p = "Hello, world!"; *p; ++p) {
		uint8_t value = *p;
		asm("outb %0,%1" : /* empty */ : "a" (value), "Nd" (port) : "memory");
	}
	
	asm("hlt" : /* empty */ : "a" (42) : "memory");
}
