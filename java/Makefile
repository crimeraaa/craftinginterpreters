EXE = Lox
CLASSPATH := com/craftinginterpreters/lox
JAVA_SRC := $(wildcard $(CLASSPATH)/*.java)
JAVA_OUT := $(patsubst %.java, %.class, $(JAVA_SRC))
JAVAC_FLAGS := --class-path . -d . -Werror -implicit:none

all: $(JAVA_OUT)
	
# Due to the package system, we can ONLY run from here with the command:
# java com/craftinginterpreters/lox/Lox [args..]
run: $(JAVA_OUT)
	java --class-path . $(CLASSPATH)/$(EXE)

# -d indicates the destination directory is our CWD.
$(JAVA_OUT): $(JAVA_SRC)
	javac $(JAVAC_FLAGS) $(JAVA_SRC)
	
clean:
	$(RM) $(JAVA_OUT)

.PHONY: all clean
