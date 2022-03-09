/* A program to embed programming languages into HTML to be used for
 * server-side scripting. Like PHP, but usable. */

#include <filesystem>
#include <getopt.h>
#include <stdio.h>
#include <string.h>

using std::filesystem::path;

struct language {
	// The tag associated with this language, usually the file extension.
	const char * tag;
	// An opening snippet of code to insert into the output file.
	const char * opening;
	// A closing snippet of code to insert into the output file.
	const char * closing;
	// The system() call to make before executeing the program.
	// This is formatted using printf, with the path of the generated script as
	// the first and only argument.
	const char * compile;
	// The system() call to execute the program.
	// This is formatted using printf, with the path of the generated script as
	// the first and only argument.
	const char * execute;
};

static const language languages[] = {
	{
		.tag = "c",
		.opening = "",
		.closing = "",
		.compile = "gcc -include stdio.h -o %1$s.out %1$s",
		.execute = "./%s.out"
	},
	{
		.tag = "cpp",
		.opening = "",
		.closing = "",
		.compile = "g++ -include stdio.h -include iostream -o %1$s.out %1$s",
		.execute = "./%s.out"
	},
	{
		.tag = "py",
		.opening = "",
		.closing = "",
		.compile = "",
		.execute = "python %s"
	},
	{
		.tag = "rs",
		.opening = "",
		.closing = "",
		.compile = "rustc -o %1$s.out --crate-name csp_rs %1$s",
		.execute = "./%s.out"
	}
};

static void show_help(const char * exe_name) {
	fprintf(stderr,
	        "Compiled Scripting Program\n"
	        "usage:\n"
	        "  %1$s -i <infile>\n"
	        "  %1$s -i <infile> -o <outfile> -d bin/\n"
	        "flags:\n"
	        "  -d --out-directory Provide a directory for producing temporary files.\n"
	        "  -h --help          Show this message.\n"
	        "  -i --input         Provide an input .csp file.\n"
	        "  -o --output        Provide an output file.\n",
	        exe_name);
}

static void error(char const* fmt, ...) {
	va_list ap;
	if (isatty(2)) fputs("\033[1m\033[31mfatal: \033[0m", stderr);
	else fputs("fatal: ", stderr);

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	putc('\n', stderr);

	exit(1);
}

static FILE * open_path(path& p, const char * s) {
	FILE * result = fopen(p.c_str(), s);
	if (!result) error("Failed to open %s", p.c_str());
	return result;
}

// Check if `str` is the next sequence in a file. Returns true if the sring is
// found and passes over it.
static bool matches_string(FILE * f, const char * str) {
	int pos = ftell(f);
	for (; *str; str++) {
		int next = fgetc(f);
		if (next == EOF || next != *str) {
			fseek(f, pos, SEEK_SET);
			return false;
		}
	}
	return true;
}

static const language * get_lang(FILE * csp) {
	char * tag = NULL;
	size_t tag_cap = 0;
	size_t tag_len = getdelim(&tag, &tag_cap, '>', csp);
	tag[tag_len - 1] = 0;
	for (size_t i = 0; i < sizeof(languages) / sizeof(*languages); i++) {
		if (strcmp(languages[i].tag, tag) == 0) {
			free(tag);
			return languages + i;
		}
	}
	fprintf(stderr, "Unrecognized language tag: %s\n", tag);
	free(tag);
	return NULL;
}

void compile_script(path& csp_path, path& outdir, FILE * out) {
	FILE * csp = open_path(csp_path, "r");

	// Execute the program, inserting its output into the CSP file.
	while (!feof(csp)) {
		if (matches_string(csp, "<?")) {
			// Determine the language of this code block.
			const language * lang = get_lang(csp);
			if (!lang) continue;
			path outpath = outdir / csp_path.stem().replace_extension(lang->tag);

			{ // Extract the script from the CSP file.
				FILE * script = open_path(outpath, "w");

				fputs(lang->opening, script);
				while (!feof(csp)) {
					if (matches_string(csp, "<?>")) break;
					int next = fgetc(csp);
					if (next == EOF) break;
					fputc(next, script);
				}
				fputs(lang->closing, script);
				fclose(script);
			}

			// Compile the script, if needed.
			if (lang->compile) {
				char * command;
				asprintf(&command, lang->compile, outpath.c_str());
				system(command);
				free(command);
			}

			FILE * script_output;
			{ // Execute the script, dumping its stdout into the output.
				char * command;
				asprintf(&command, lang->execute, outpath.c_str());
				script_output = popen(command, "r");
				free(command);
			}

			// Copy the script output.
			for (int next = fgetc(script_output); next != EOF;) {
				fputc(next, out);
				next = fgetc(script_output);
			}

			pclose(script_output);
		} else {
			// If not running code, echo the input.
			int next = fgetc(csp);
			if (next == EOF) break;
			fputc(next, out);
		}
	}
	fclose(csp);
}

static struct option const longopts[] = {
	{"out-directory", required_argument, NULL, 'd'},
	{"help",          no_argument,       NULL, 'h'},
	{"infile",        required_argument, NULL, 'i'},
	{"outfile",       required_argument, NULL, 'o'},
	{NULL}
};
static const char shortopts[] = "d:hi:o:";

int main(int argc, char ** argv) {
	if (argc < 2) {
		show_help(argv[0]);
		return 1;
	}

	path outdir;
	path infile;
	FILE * outfile = stdout;

	char option_char;
	while ((option_char = getopt_long_only(argc, argv, shortopts, longopts, NULL)) != -1) {
		switch (option_char) {
			case 'd':
				outdir = optarg;
				break;
			case 'h':
				show_help(argv[0]);
				return 0;
			case 'i':
				infile = optarg;
				break;
			case 'o':
				outfile = fopen(optarg, "w");
				if (!outfile) error("Failed to open %s", optarg);
				break;
		}
	}

	if (infile.empty()) error("Missing input file");
	if (outdir.empty()) outdir = infile.parent_path();

	compile_script(infile, outdir, outfile);
	return 0;
}
