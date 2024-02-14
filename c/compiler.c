#include "common.h"
#include "scanner.h"
#include "compiler.h"

void compiler_compile(const char *source)
{
    scanner_init(source);
    int line = -1;
    for (;;) {
        LoxToken token = scanner_scan_token();
        // Print the line number if it's unique, else print a pipe.
        if (token.line != line) {
            printf("%4d ", token.line);
            line = token.line;
        } else {
            printf("   | ");
        }
        // For now, print the tokentype value and source code substring as is.
        printf("%2d '%.*s'\n", token.type, token.length, token.start);
        if (token.type == TOKEN_EOF) {
            break;
        }
    }
}
