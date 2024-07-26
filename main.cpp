
#include "../FileType/src/FileType.h"
#include "../FileType/src/magic_mgc.h"
#include "Projector.h"
#include "joinpath.h"
#include <cctype>
#include <cstring>
#include <optional>
#include <sys/stat.h>

FileType ft;

bool isbinary(char const *data, size_t size, int st_mode)
{
	auto r = ft.file(data, size, st_mode);
	return strcmp(r.charset.c_str(), "binary") == 0;
}

std::string to_string(std::string_view const &v)
{
	return {v.data(), v.size()};
}

int main(int argc, char **argv)
{
	ft.open(magic_mgc_data, magic_mgc_size);

	bool usage = false;
	
	std::vector<std::pair<std::string_view, std::string_view>> rules;
	char const *srcpath = nullptr;
	char const *dstpath = nullptr;
	for (int argi = 1; argi < argc; argi++) {
		char const *arg = argv[argi];
		if (*arg == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0) {
				usage = true;
			} else {
				fprintf(stderr, "unknown option: %s\n", arg);
			}
		} else {
			auto ParseRule = [](char const *s)->size_t{
				size_t colon = 0;
				size_t i = 0;
				for (i = 0; s[i]; i++) {
					char c = s[i];
					if (c == ':') {
						if (i == 0) return 0;
						colon = i;
					} else {
						if (!isalnum((unsigned char)c) && c != '_') return 0;
					}
				}
				if (colon > 0 && colon + 1 < i) {
					return colon;
				}
				return 0;
			};
			size_t colon = ParseRule(arg);
			if (colon > 0) {
				std::string_view srcsym{arg, colon};
				std::string_view dstsym{arg + colon + 1};
				rules.emplace_back(srcsym, dstsym);
			} else if (!srcpath) {
				srcpath = arg;
			} else if (!dstpath) {
				dstpath = arg;
			} else {
				fprintf(stderr, "syntax error: %s\n", arg);
			}
		}
	}
	if (!usage) {
		if (rules.empty()) {
			fprintf(stderr, "rule is not specified\n");
			exit(1);
		}
		if (!srcpath) {
			fprintf(stderr, "source path is not specified\n");
			exit(1);
		}
		if (!dstpath) {
			fprintf(stderr, "destination path is not specified\n");
			exit(1);
		}
	}
	
	if (usage) {
		fprintf(stderr, "usage: projector <rule> <srcpath> <dstpath>\n");
		fprintf(stderr, "  rule: <srcsym>:<dstsym>\n");
		fprintf(stderr, "   - srcsym: source symbol\n");
		fprintf(stderr, "   - dstsym: destination symbol\n");
		fprintf(stderr, "  srcpath: source path\n");
		fprintf(stderr, "  dstpath: destination path\n");
		exit(1);
	}
	
	Projector gen(std::move(rules));
	
	struct stat srcst;
	if (stat(srcpath, &srcst) != 0) {
		fprintf(stderr, "no such file or directory: %s\n", srcpath);
		exit(1);
	}
	
	std::string srcpath2;
	{
		std::string_view v = srcpath;
		while (v.size() > 0 && v.back() == '/') {
			v = v.substr(0, v.size() - 1);
		}
		srcpath2 = v;
	}
	std::string dstpath2 = dstpath;
	struct stat dstst;
	if (stat(dstpath, &dstst) == 0) {
		if (S_ISDIR(dstst.st_mode)) {
			char const *p = strrchr(srcpath2.c_str(), '/');
			if (p) {
				p++;
			} else {
				p = srcpath2.c_str();
			}
			dstpath2 = dstpath2 / gen.replaceWords(p);
		} else {
			fprintf(stderr, "already existing: %s\n", dstpath);
			exit(1);
		}
	}

	gen.perform(srcpath2, dstpath2);

	return 0;
}

