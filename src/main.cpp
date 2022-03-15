/* A program to embed programming languages into HTML to be used for
 * server-side scripting. Like PHP, but usable. */

#include "fmt/format.h"
#include <filesystem>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <string>
#include <string.h>
#include <vector>
#include <unistd.h>

using std::filesystem::path;
using std::string;
using std::vector;

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
	// The extension of the file produced and executed by the program. This is
	// re-used if it already exists, speeding up processing.
	const char * output_extension;
	void (* mode_opening) (FILE * f, string mode);
	void (* mode_closing) (FILE * f, string mode);
};

void c_mode_opening(FILE * f, string mode) {
	if (mode == "main") fputs("int main() {\n", f);
}
void c_mode_closing(FILE * f, string mode) {
	if (mode == "main") fputs("\n}\n", f);
}
void rs_mode_opening(FILE * f, string mode) {
	if (mode == "main") fputs("fn main() {\n", f);
}
void rs_mode_closing(FILE * f, string mode) {
	if (mode == "main") fputs("\n}\n", f);
}

static const language languages[] = {
	{
		.tag = "c",
		.compile = "gcc -include stdio.h -o {0}.out {0}",
		.execute = "./{0}.out {1}",
		.output_extension = ".out",
		.mode_opening = &c_mode_opening,
		.mode_closing = &c_mode_closing,
	},
	{
		.tag = "cpp",
		.compile = "g++ -include stdio.h -include iostream -o {0}.out {0}",
		.execute = "./{0}.out {1}",
		.output_extension = ".out",
		.mode_opening = &c_mode_opening,
		.mode_closing = &c_mode_closing,
	},
	{
		.tag = "py",
		.execute = "python {0} {1}",
		.output_extension = "",
	},
	{
		.tag = "rs",
		.compile = "rustc -o {0}.out --crate-name csp_rs {0}",
		.execute = "./{0}.out {1}",
		.output_extension = ".out",
		.mode_opening = &rs_mode_opening,
		.mode_closing = &rs_mode_closing,
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

static void error(const char * fmt, ...) {
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

// Check if `str` is the next sequence in a file. Returns true if the string is
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

static int fgets_delims(string& result, FILE * f, const char * delims) {
	while (1) {
		int next = fgetc(f);
		if (next == EOF || strchr(delims, next)) return next;
		result += next;
	}
}

static const language * get_lang(FILE * csp, vector<string>& modes) {
	string tag;
	if (fgets_delims(tag, csp, " >") == ' ') {
		while (1) {
			string new_mode;
			int result = fgets_delims(new_mode, csp, " >");
			modes.push_back(new_mode);
			if (result != ' ') break;
		}
	}
	for (size_t i = 0; i < sizeof(languages) / sizeof(*languages); i++) {
		if (tag == languages[i].tag) {
			return languages + i;
		}
	}
	error("Unrecognized language tag %s", tag.c_str());
	return NULL;
}

void compile_script(path& csp_path, path& outdir, FILE * out) {
	FILE * csp = open_path(csp_path, "r");
	int iterations = 0;

	// Execute the program, inserting its output into the CSP file.
	while (!feof(csp)) {
		if (matches_string(csp, "<?")) {
			// Determine the language of this code block.
			vector<string> tag_modes;
			const language * lang = get_lang(csp, tag_modes);
			if (!lang) continue;
			path outpath = outdir / (string(csp_path.stem()) + std::to_string(iterations++));
			outpath.replace_extension(lang->tag);

			if (access((string(outpath) + lang->output_extension).c_str(), F_OK)) {
				FILE * script = open_path(outpath, "w");

				if (lang->opening) fputs(lang->opening, script);
				if (lang->mode_opening) for (auto& i : tag_modes) lang->mode_opening(script, i);
				while (!feof(csp)) {
					if (matches_string(csp, "<?>")) break;
					int next = fgetc(csp);
					if (next == EOF) break;
					fputc(next, script);
				}
				if (lang->closing) fputs(lang->closing, script);
				if (lang->mode_closing) for (auto& i : tag_modes) lang->mode_closing(script, i);
				fclose(script);

				// Compile the script, if needed.
				if (lang->compile) system(fmt::format(lang->compile, string(outpath)).c_str());
			} else {
				while (!feof(csp)) {
					if (matches_string(csp, "<?>")) break;
					else fgetc(csp);
				}
			}

			FILE * script_output = popen(fmt::format(lang->execute, string(outpath), "hello=world&foo=bar").c_str(), "r");

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
