\# Mini Calculator Compiler



A small compiler \*\*front-end\*\* for a tiny calculator language, written in C, with a matching browser demo. It runs three phases in order and shows the output of each:



```

Source -> \[Lexer] -> tokens -> \[Parser] -> AST -> \[Semantic analyzer] -> result

```



A phase runs only if the previous one succeeded. It does \*\*not\*\* generate code or execute the program.



\## Project structure



| File | Purpose |

|------|---------|

| `lexer.c` / `lexer.h` | Lexical analysis: turns source text into tokens |

| `sample\_input.txt` | Example program |



\## Language



Programs are a list of statements, each ending with `;`.



```

a = 10;

b = 20;

c = a + b \* 2;

print(c);

print((a + b) / 2);

```



Operators: `+ - \* /` and parentheses. `\*` and `/` bind tighter than `+` and `-`.



\## Phases



1\. \*\*Lexical analysis\*\* splits the source into tokens (identifiers, numbers, operators, `=`, `(`, `)`, `;`) and records line numbers.



