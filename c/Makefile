CC 	   := clang
DEBUGFLAGS := -fdiagnostics-color=always -g -O0
CFLAGS := $(DEBUGFLAGS) -std=c11 -Wall -Wextra -Wshadow -pedantic

EXE := clox
CC_SRC := $(wildcard *.c)
CC_OBJ := $(patsubst %.c, obj/%.o, $(CC_SRC))

all: bin/$(EXE)
	
# bin obj:
# 	$(MKDIR) $@
	
bin/$(EXE): $(CC_OBJ) | bin
	$(CC) $(CFLAGS) -o $@ $^

obj/%.o: %.c | obj
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(CC_OBJ)
	
uninstall: clean
	$(RM) $(EXE)
	
.PHONY: all clean uninstall
