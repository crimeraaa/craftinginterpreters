CC 	   := clang
CFLAGS := -std=c11 -Wall -Wextra -Wshadow -pedantic
DEBUGFLAGS 	 := -fdiagnostics-color=always -g -O0
RELEASEFLAGS := -O2 -s

EXE := clox
CC_SRC := $(wildcard *.c)
CC_OBJ := $(patsubst %.c, obj/%.o, $(CC_SRC))

all: debug
	
build: bin/$(EXE)
	
debug: CFLAGS += $(DEBUGFLAGS) -DDEBUG_PRINT_CODE
debug: build
	
trace: CFLAGS += -DDEBUG_TRACE_EXECUTION
trace: debug
	
release: CFLAGS += $(RELEASEFLAGS)
release: build
	
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
	
.PHONY: all build clean debug release trace uninstall
