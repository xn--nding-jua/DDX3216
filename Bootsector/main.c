void print_char(char c) {
    asm volatile (
        "mov $0x0E, %%ah\n\t"  // select type-function in text-mode
        "mov %0, %%al\n\t"     // load char in AL
        "int $0x10\n\t"        // call BIOS-interrupt 10h
        :
        : "r"(c)
        : "eax"
    );
}

void print_string(const char* str) {
    while (*str) {
        print_char(*str);
        str++;
    }
}

__attribute__((noreturn)) void main() {
    print_string("Hello World from C-Code!");

    while(1) {
        asm volatile("hlt");
    }
}
