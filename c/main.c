#include "common.h"
#include "chunk.h"
#include "debug.h"
#include "vm.h"

static void run_repl(void)
{
    char line[1024];
    for (;;) {
        printf("> ");
        // Reads in newline, so interpreter sees 2 lines in here!
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        vm_interpret(line);
    }
}

/* Read as script into memory and allocate one string for all of it. */
static char *read_file(const char *path)
{
    FILE *file_ptr = fopen(path, "rb"); // readonly in binary mode
    if (file_ptr == NULL) {
        lox_dprintf("read_file", "Could not open file '%s'.\n", path);
        exit(EX_IOERR);
    }

    fseek(file_ptr, 0L, SEEK_END); 
    size_t file_size = ftell(file_ptr);
    rewind(file_ptr); // Move file pointer back to beginning of stream
    
    char *buffer = malloc(file_size + 1);
    // If this happens, all we can say is good luck!
    if (buffer == NULL) {
        lox_dprintf("read_file", "Not enough memory to read '%s'.\n", path);
        exit(EX_IOERR);
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file_ptr);
    if (bytes_read < file_size) {
        lox_dprintf("read_file", "Could not read file '%s'.\n", path);
        exit(EX_IOERR);
    }
    buffer[bytes_read] = '\0';

    fclose(file_ptr);
    return buffer;
}

static void run_file(const char *path)
{
    char *source = read_file(path);
    LoxInterpretResult result = vm_interpret(source);
    free(source);
    
    if (result == INTERPRET_COMPILE_ERROR) {
        exit(EX_DATAERR);
    }
    if (result == INTERPRET_RUNTIME_ERROR) {
        exit(EX_SOFTWARE);
    }
}

int main(int argc, char *argv[]) 
{
    vm_init();
    if (argc == 1) {
        run_repl();
    } else if (argc == 2) {
        run_file(argv[1]);
    } else {
        fprintf(stderr, "Usage: clox [path]\n");
        exit(EX_USAGE);
    }
    vm_free();
}
