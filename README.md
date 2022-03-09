# Compiled Scripting Program.
A PHP-like program which allows any language to be embedded into text files.
This can be extended and used as a server-side scripting language, or even just
serve as a sort of macro language to generate text files or source code.

It works by extracting the user's scripts from the input file and executing
them, piping each program's stdout into an output file.

Currently, C, C++, Rust, and Python have support within the program, but new
languages can easily be added in under a minute.

# Supported languages

- C
	- Automatically includes stdio.h
- C++
	- Automatically includes stdio.h and iostream
- Rust
- Python

# Adding a language
Each language in the program is stored as a `language` structure within the
`languages` array. For example:

```
static const language languages[] = {
	{
		.tag = "c",
		.opening = "",
		.closing = "",
		.compile = "gcc -include stdio.h -o %1$s.out %1$s",
		.execute = "./%1$s.out"
	}
}
```

To add support for a language, start by choosing a tag. This is what you'll
use to begin writing the language within a .csp file. For example, if the tag
is "c": `<?c> // C code here! <?>`. The tag is also used as the output script's
extension.

The opening and closing strings are used to insert code before and after the
user-provided script. This can be used to include common libraries before the
user's code to reduce redundancy. This could be set to `"#include <stdio.h>\n"`,
for example, but for the C language the `-include` compiler flag was used
instead.

The compile string is a system call which will be run prior to executing the
script. Before being called, it will be formatted with the script file as the
first and only input. This file will replace any instance of `%1$s` in the
string.

Finally, the execute string is a system call which will run immediately after
the compile call has finished. Just like compile, it will be formatted with the
script file as the first and only input. This file will replace any instance of
`%1$s` in the string.

# Planned features

- Language arguments
	- For example, `<?c main>` could be used to insert a main function around
	the user's code, turning `int main() {puts("Hello, world!");}` into simply
	`puts("Hello, world!");`.
- A library providing HTTPS capabilities, making CSP a viable alterntive to PHP.
- Configurable tag syntax, to allow CSP to be used in more contexts.
