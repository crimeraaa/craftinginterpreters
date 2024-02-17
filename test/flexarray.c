#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Stupid but I just want to see how this works. */
#define FLEXARRAY_SIZE 8

typedef struct {
    int len;
    // Instead of allocating a separate pointer, you can use flexible array 
    // members to allocate the memory needed all at once.
    int data[];
} FlexArray;

#define ga_new(n)   malloc(sizeof(FlexArray) * sizeof(int[n]))

void ga_init(FlexArray *inst, int n) {
    inst->len = n;
    // runtime sizeof is allowed since C99.
    memset(inst->data, 0, sizeof(int[n]));
}

int main() {
    FlexArray *inst = ga_new(FLEXARRAY_SIZE);
    ga_init(inst, FLEXARRAY_SIZE);
    // for (int i = 0; i < inst->len; i++) {
    //     inst->data[i] = rand();
    // }
    for (int i = 0; i < inst->len; i++) {
        printf("inst->data[%i] = %i\n", i, inst->data[i]);
    }
    free(inst);
    return 0;
}
