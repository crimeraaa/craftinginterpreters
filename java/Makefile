EXE := Lox
CLASSPATH 	:= com/craftinginterpreters/lox
JAVA_SRC 	:= $(wildcard $(CLASSPATH)/*.java)
JAVA_OUT 	:= $(patsubst %.java, %.class, $(JAVA_SRC))
JAVA_FLAGS  := --class-path .
JAVAC_FLAGS := $(JAVA_FLAGS) -d . -Werror -implicit:none

TOOL 		:= GenerateAst
TOOLPATH 	:= com/craftinginterpreters/tool
TOOL_SRC	:= $(wildcard $(TOOLPATH)/*.java)
TOOL_OUT 	:= $(patsubst %.java, %.class, $(TOOL_SRC))

all: $(JAVA_OUT) $(TOOL_OUT)
	
# Due to the package system, we can ONLY run from here with the command:
# java com/craftinginterpreters/lox/Lox [args..]
run: $(JAVA_OUT)
	java $(JAVA_FLAGS) $(CLASSPATH)/$(EXE)
	
# Generate the AST datatypes and visitor pattern methods.
# $(CLASSPATH) is the first and only argument to GenerateAst.
gen: $(TOOL_OUT)
	java $(JAVA_FLAGS) $(TOOLPATH)/$(TOOL) $(CLASSPATH)

# -d indicates the destination directory is our CWD.
$(JAVA_OUT): $(JAVA_SRC)
	javac $(JAVAC_FLAGS) $(JAVA_SRC)
	
$(TOOL_OUT): $(TOOL_SRC)
	javac $(JAVAC_FLAGS) $(TOOL_SRC)
	
clean:
	$(RM) $(JAVA_OUT) $(TOOL_OUT)

.PHONY: all clean
