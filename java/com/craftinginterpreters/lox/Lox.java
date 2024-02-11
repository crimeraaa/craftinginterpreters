/* Part 4.2.0: https://craftinginterpreters.com/scanning.html */
package com.craftinginterpreters.lox;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.List;

/**
 * @brief       To compile: `javac -d . com/craftinginterpreters/lox/*.java`
 *              To run:     `java com/craftinginterpreters/lox/Lox`
 */
public class Lox {
    // Make static so successive calls to `runCode` use the same interpreter.
    private static final Interpreter interpreter = new Interpreter();
    static boolean hadError = false;
    static boolean hadRuntimeError = false; // Only really matters for files

    public static void main(String[] args) throws IOException {
        if (args.length > 1) {
            System.out.println("Usage: jlox [script]");
            System.exit(64); // UNIX sysexits.h convention
        } else if (args.length == 1) {
            runFile(args[0]);
        } else {
            runPrompt();
        }
    }

    /* Lex, parse, analyze and execute a file then get outta here */
    private static void runFile(String path) throws IOException {
        byte[] bytes = Files.readAllBytes(Paths.get(path));
        String source = new String(bytes, Charset.defaultCharset());
        runCode(source);
        
        // Indicate an error in the exit code.
        if (hadError) {
            System.exit(65);
        }
        if (hadRuntimeError) {
            System.exit(70);
        }
    }

    /* REPL */
    private static void runPrompt() throws IOException {
        InputStreamReader input = new InputStreamReader(System.in);
        BufferedReader reader = new BufferedReader(input);
        // Loop infinitely in interactive prompt mode
        for (;;) {
            System.out.print("> ");
            String line = reader.readLine();
            if (line == null) {
                break; // Occurs when CTRL-D pressed, indicates we should exit.
            }
            runCode(line);
            hadError = false; // Don't kill interactive session just yet.
        }
    }

    private static void runCode(String source) {
        Scanner scanner = new Scanner(source);
        List<Token> tokens = scanner.scanTokens();
        Parser parser = new Parser(tokens);
        Expr expression = parser.startParser();

        if (hadError) {
            return;
        }
        interpreter.interpret(expression);
    }

    static void error(int line, String message) {
        report(line, "", message);
    }

    private static void report(int line, String where, String message) {
        System.err.println("[line " + line + "] Error" + where + ": " + message);
        hadError = true;
    }
    
    static void error(Token token, String message) {
        if (token.type == TokenType.TK_EOF) {
            report(token.line, " at end", message);
        } else {
            report(token.line, " at '" + token.lexeme + "'", message);
        }
    }
    
    static void runtimeError(RuntimeError error) {
        System.err.println(error.getMessage() + "\n[line " + error.token.line + "]");
        hadRuntimeError = true;
    }
}
