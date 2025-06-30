// Minimal test to isolate the crash
#include <stdio.h>

static int __attribute__((constructor)) global_init() {
    printf("🧚 Global constructor working\n");
    return 0;
}

int main() {
    printf("🧚 Main function reached\n");
    return 0;
}
