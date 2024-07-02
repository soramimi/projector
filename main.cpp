
#include "Projector.h"
#include "joinpath.h"
#include <cctype>
#include <cstring>
#include <sys/stat.h>
#include "../FileType/src/FileType.h"
#include "../FileType/src/magic_mgc.h"

FileType ft;

bool isbinary(int fd)
{
	auto r = ft.file(fd);
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
	
	char const *rule = nullptr;
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
			if (!rule) {
				rule = arg;
			} else if (!srcpath) {
				srcpath = arg;
			} else if (!dstpath) {
				dstpath = arg;
			} else {
				fprintf(stderr, "syntax error: %s\n", arg);
			}
		}

	}
	std::string_view srcsym;
	std::string_view dstsym;
	if (!usage) {
		if (rule) {
			char const *p1 = strchr(rule, ':');
			if (p1) {
				srcsym = {rule, size_t(p1 - rule)};
				p1++;
				char const *p2 = strchr(p1, ':');
				if (!p2) {
					dstsym = p1;
				}
			}
			auto issymbol = [](std::string_view const &s){
				int n = s.size();
				if (n > 0) {
					if (isalnum((unsigned char)s[0])) {
						for (int i = 1; i < n; i++) {
							if (!isalnum((unsigned char)s[i]) && s[i] != '_') {
								return false;
							}
						}
						return true;
					}
				}
				return false;
			};
			if (!issymbol(srcsym)) {
				fprintf(stderr, "source symbol is not specified\n");
				exit(1);
			}
			if (!issymbol(dstsym)) {
				fprintf(stderr, "destination symbol is not specified\n");
				exit(1);
			}
		} else {
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
	
	std::string s = to_string(srcsym);
	std::string d = to_string(dstsym);
	Projector gen(s, d);
	
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

