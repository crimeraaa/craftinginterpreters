# What is this?

This repo is my journey working through [*Crafting Interpreters*](https://craftinginterpreters.com/statements-and-state.html) by [Bob Nystrom (munificent)](https://github.com/munificent).

This is also my first time writing Java in literal years, and I don't really like the language :) I'm genuinely excited to start the parts in C though!

# Lox Grammar

This is the Lox grammar so far, adapted from the bits and pieces Bob gives us starting in [Chapter 5.](https://craftinginterpreters.com/representing-code.html#rules-for-grammars) I've combined productions to ways that make more sense to me while still hopefully preserving Bob's original design.

This is the Backus-Naur notation for *"metasyntax"*. It's a syntax that in itself describes a programming language's syntax.

The sequence `::=` indicates a *"production"*, which is like a set of possibilities that can resursively refer to itself.

Like in regular expressions, the following apply:

1. `*`: match 0 or more of the left operand.
2. `?`: match 0 or 1 of the left operand.
3. `+`: match 1 or more of the left operand.
4. `(<pattern>|<pattern>)`: match one of the patterns inside the parentheses separated by `|`.

Sequences enclosed by outer square brackets and inner double quotes, like `["var"]`, mean that the sequence inside the quotes (e.g. `var`) should appear as-is in the source code.

Purely uppercase sequences, like `IDENTIFIER`, represent some arbitrary sequence, like a variable name or a number/string literal. `EOF` is unique in that it's not something you can write, but rather is (or **should** be!) implicitly encoded into your text files.

```bnf
program     ::= declaration* EOF

declaration ::= function 
              | variable 
              | statement
function    ::= ["fun"] IDENTIFIER ["("] parameters? [")"] block
parameters  ::= IDENTIFIER ( [","] IDENTIFIER )*
variable    ::= ["var"] IDENTIFIER assignment? [";"]
assignment  ::= ["="] expression
statement   ::= expression ";"
              | block
              | ["if"] ifcondition statement ( ["else"] statement )?
              | ["while"] ifcondition statement
              | ["for"] loopvariant statement
              | ["return"] expression? [";"]
block       ::= ["{"] declaration* ["}"]
ifcondition ::= ["("] expression [")"]
loopvariant ::= ["("] ( variable | expression | [";"] ) 
                expression? [";"]
                expression? [")"]
                
expression  ::= primary
              | IDENTIFIER assignment
              | ( unary )* expression
              | expression ( binary expression )*
              | grouping
              | invocation
primary     ::= NUMBER
              | STRING
              | IDENTIFIER
              | ["true"]
              | ["false"]
              | ["nil"]
grouping    ::= ["("] expression [")"]
invocation  ::= IDENTIFIER ( ["("] arguments? [")"] )+
arguments   ::= expression ( [","] expression )*

unary       ::= ["!"]
              | ["-"]
binary      ::= arithmetic
              | relational
              | logical
arithmetic  ::= ["+"] 
              | ["-"] 
              | ["*"] 
              | ["/"]
relational  ::= [">"] 
              | [">="] 
              | ["<"] 
              | ["<="] 
              | ["=="] 
              | ["!="]
logical     ::= ["and"]
              | ["or"]

```

# Operator Precedence

The following BNF grammar describes the exact order of operations for Lox, this is more true to the tidbits that we're given in the book.

Note that operations farther down are actually higher precedence, because they get evaluated earlier than the operations higher up the grammar. 

So like in C, Lox treats logical `and` as being higher priority that logical `or`. 

Assignment of existing variables is higher priority than declaration of a new variable, giving it a right associativity. Like in the following:

```
var a = 13;
var b = a = 4; // Looks like `var b = (a = 4);`
```

And of course, function calls have a *very* high precedence!

```bnf
program     ::= declaration* EOF
declaration ::= fundecl
              | vardecl
              | statement
fundecl     ::= "fun" function
function    ::= IDENTIFIER "(" parameters? ")" block
parameters  ::= IDENTIFIER ( "," IDENTIFIER )*
vardecl     ::= "var" IDENTIFIER ( "=" expression )? ";"
statement   ::= exprstmt
              | block
              | printstmt
              | ifelsestmt
              | whilestmt
              | forstmt
              | returnstmt
exprstmt    ::= expression ";"
block       ::= "{" declaration* "}"
printstmt   ::= "print" expression ";"
ifelsestmt  ::= "if" "(" expression ")" statement ( "else" statement )?
whilestmt   ::= "while" "(" expression ")" statement
forstmt     ::= "for" "(" forinit forcond foriter ")" statement
forinit     ::= ( vardecl | exprstmt | ";" )
forcond     ::= expression? ";"
foriter     ::= expression? 

expression  ::= assignment
assignment  ::= IDENTIFIER "=" assignment
              | logical_or
logical_or  ::= logical_and ( "or" logical_and )*
logical_and ::= equality ( "and" equality )*
equality    ::= comparison ( ( "!=" | "==" ) comparison )*
comparison  ::= terminal ( ( ">" | ">=" | "<" | "<=" ) terminal )*
terminal    ::= factor  ( ( "-" | "+" ) factor )* 
factor      ::= unary ( ( "/" | "*" ) unary )*
unary       ::= ( "!" | "-" ) unary
              | invocation
invocation  ::= primary ( "(" arguments? ")" )*
arguments   ::= expression ( "," expression )*
primary     ::= literal | grouping | IDENTIFIER
literal     ::= NUMBER | STRING | "true" | "false" | "nil"
grouping    ::= "(" expression ")"
              
```

<!-- I suck at LaTeX lol
$$
% Use the builtin align package for, well, simple alignment.
\begin{align*} 
% Use '&' to align at this particular symbol, and '\\' for newlines.
program     &::= declaration* \mathrm{EOF} \\
declaration &::= \mathrm{function}~|\mathrm{variable}~|\mathrm{statement} \\
function    &::= \mathrm{["fun"] IDENTIFIER } ["("] parameters? [")"] block \\
parameters  &::= \mathrm{IDENTIFIER} ( [","] \mathrm{IDENTIFIER} )*
\end{align*}
$$
 -->

