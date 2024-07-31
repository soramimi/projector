#include "Projector.h"
#include "joinpath.h"
#include <algorithm>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

bool isbinary(char const *data, size_t size, int st_mode);

/**
 * @brief 大文字かどうかを判定する
 * @param c
 * @return 
 */
static inline bool is_upper(char c)
{
	return std::isupper((unsigned char)c);
}

/**
 * @brief 小文字かどうかを判定する
 * @param c
 * @return 
 */
static inline bool is_lower(char c)
{
	return std::islower((unsigned char)c) || std::isdigit((unsigned char)c); // 数字も小文字扱い
}

enum CaseStyle {
	DefaultCase,
	TitleCase,
	UpperCase,
	LowerCase,
};

struct JoinPolicy {
	CaseStyle case_style = DefaultCase;
	char separator = 0;
	JoinPolicy(CaseStyle cs = DefaultCase, char sep = 0)
		: case_style(cs)
		, separator(sep)
	{
	}
};

/**
 * @brief 指定された文字列を単語に分割してリストとして返す
 * @param s 分割する文字列
 * @return 単語のリスト
 */
Projector::strlist_t Projector::split(std::string_view const &s)
{
	strlist_t words;
	char const *ptr = s.data();
	char const  *end = ptr + s.size();
	while (ptr < end) {
		int i = 1;
		while (1) {
			char c = ptr[i];
			if (ptr + i == end || (is_upper(c) && is_lower(ptr[i + 1]))) {
				break;
			}
			i++;
		}
		std::string word(ptr, i);
		words.push_back(word);
		ptr += i;
	}
	return words;
}

/**
 * @brief 大文字小文字変換
 * @param s
 * @param cp
 * @return
 */
std::string convert(std::string_view const &s, CaseStyle cp)
{
	std::string ret(s);
	switch (cp) {
	case LowerCase:
		std::transform(ret.cbegin(), ret.cend(), ret.begin(), tolower);
		break;
	case UpperCase:
		std::transform(ret.cbegin(), ret.cend(), ret.begin(), toupper);
		break;
	case TitleCase:
		if (s.size() > 0) {
			std::transform(ret.cbegin(), ret.cend(), ret.begin(), tolower);
			*ret.begin() = toupper(*ret.begin());
		}
		break;
	}
	return ret;
}

#ifdef _WIN32
int strncasecmp(char const *l, const char *r, int n)
{
	return strnicmp(l, r, n);
}
#endif

Projector::Projector(std::vector<std::pair<std::string_view, std::string_view>> &&rules)
{
	for (auto const &rule : rules) {
		auto srcname = std::get<0>(rule);
		auto dstname = std::get<1>(rule);
		auto srcwords = split(srcname);
		auto dstwords = split(dstname);
		words_.push_back({srcwords, dstwords});
	}
}

/**
 * @brief 文字列中の単語を置換する
 * @param srctext
 * @param srcwords
 * @param dstwords
 * @return
 *
 * 例: srctext = "HelloWorld", srcwords = ["Hello", "World"], dstwords = ["Good", "Bye"]
 */
std::vector<char> Projector::internalReplaceWords(std::string_view const &srctext, strlist_t const &srcwords, strlist_t const &dstwords)
{
	std::vector<char> newtext;

	auto AppendS = [&](std::string_view const &s){
		newtext.insert(newtext.end(), s.begin(), s.end());
	};

	auto AppendC = [&](char c){
		newtext.push_back(c);
	};

	auto CaseFind = [](std::string_view const &source, std::string_view const &pattern, int pos){
		int n = source.size();
		int m = pattern.size();
		if (m <= n) {
			n -= m;
			for (int i = pos; i <= n; i++) {
				if (strncasecmp(source.data() + i, pattern.data(), m) == 0) {
					return i;
				}
			}
		}
		return -1;
	};

	int pos = 0;
	while (1) {
		int next = CaseFind(srctext, srcwords[0], pos);
		if (next >= pos) {
			int match = 1;
			std::vector<int> vec;
			vec.push_back(next);
			int s = next;
			for (int i = 1; i < srcwords.size(); i++) {
				int end = next + srcwords[i - 1].size();
				int next2 = CaseFind(srctext, srcwords[i], next);
				if (next2 == end || next2 == end + 1) {
					vec.push_back(next2);
					next = next2;
					match++;
				} else {
					next += srcwords[i - 1].size();
					AppendS(srctext.substr(pos, next - pos));
					break;
				}
			}
			if (match == srcwords.size()) {
				AppendS(srctext.substr(pos, s - pos));
				char sep = 0;
				CaseStyle cp = TitleCase;
				for (int i = 0; i < vec.size(); i++) {
					int n = srcwords[i].size();
					int p = vec[i];
					char c = srctext[p];
					char d = srctext[p + 1];
					if (is_upper(c)) {
						if (is_upper(d)) {
							cp = UpperCase;
						}
						if (is_lower(d)) {
							cp = TitleCase;
						}
					} else if (is_lower(c)) {
						if (is_lower(d)) {
							cp = LowerCase;
						}
					}
					if (i < dstwords.size()) {
						AppendS(convert(dstwords[i], cp));
					}
					next = vec[i] + n;
					if (i + 1 < vec.size()) {
						int r = p + n;
						if (r + 1 == vec[i + 1]) {
							sep = srctext[r];
							switch (sep) {
							case ' ':
							case '-':
							case '_':
								break; // ok
							default:
								sep = 0; // invalidate
								break;
							}
							if (sep != 0 && i + 1 < dstwords.size()) {
								AppendC(sep);
							}
						}
						next++;
					}
				}
				for (int i = vec.size(); i < dstwords.size(); i++) {
					if (sep != 0) {
						AppendC(sep);
					}
					AppendS(convert(dstwords[i], cp));
				}
			}
			pos = next;
		} else {
			AppendS(srctext.substr(pos));
			break;
		}
	}

	return newtext;
}

std::vector<char> Projector::internalReplaceWords(std::string_view const &srctext, std::vector<WordsPair> const &words)
{
	std::string_view view = srctext;
	std::vector<char> vec;
	for (WordsPair const &pair : words) {
		vec = internalReplaceWords(view, pair.srcwords, pair.dstwords);
		view = std::string_view(vec.data(), vec.size());
	}
	return vec;
}

/**
 * @brief mkpath ディレクトリを作成する
 * @param path 
 * @param dir
 * @param mode
 * @return 
 */
bool mkpath(std::string const &path, std::string const &dir, int mode)
{
	struct stat st;
	if (stat(dir.c_str(), &st) == 0) {
		if (S_ISDIR(st.st_mode)) return true;
		return false;
	}
	auto it = dir.find_last_of('/');
	if (it != std::string::npos) {
		if (!mkpath(path, dir.substr(0, (int)it), mode)) {
			return false;
		}
	}
	printf("mkdir: %s\n", path.c_str());
	return mkdir(dir.c_str(), mode) == 0;
}

/**
 * @brief ファイルを変換してコピーする
 * @param srcpath
 * @param dstpath
 * @param srcwords
 * @param dstwords
 */
void Projector::convert_file(std::string const &srcpath, std::string const &dstpath, std::vector<WordsPair> const &words, bool implace_edit)
{
	int openflags = O_RDONLY;
	if (implace_edit) {
		openflags = O_RDWR;
	}
	int fd = open(srcpath.c_str(), openflags);
	if (fd != -1) {
		auto const *i = strrchr(dstpath.c_str(), '/');
		if (i > dstpath.c_str()) {
			std::string basedir(dstpath.c_str(), i - dstpath.c_str());
			mkpath(basedir, basedir, 0755);
		}
		auto ReplaceAndWriteFile = [&](int fd, std::vector<char> const &vec, __mode_t st_mode){
			if (isbinary(vec.data(), vec.size(), st_mode)) {
				write(fd, vec.data(), vec.size());
			} else {
				auto vec2 = internalReplaceWords(std::string_view(vec.data(), vec.size()), words);
				write(fd, vec2.data(), vec2.size());
			}
		};

		struct stat st;
		if (fstat(fd, &st) == 0 && S_ISREG(st.st_mode)) {
			std::vector<char> vec(st.st_size);
			read(fd, vec.data(), vec.size());
			if (implace_edit) {
				printf(" file: %s\n", srcpath.c_str());
				lseek(fd, 0, SEEK_SET);
				ftruncate(fd, 0);
				ReplaceAndWriteFile(fd, vec, st.st_mode);
				close(fd);
			} else {
				close(fd);
				printf(" file: %s\n", dstpath.c_str());
				int fd2 = open(dstpath.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
				if (fd2 != -1) {
					ReplaceAndWriteFile(fd2, vec, st.st_mode);
					close(fd2);
				}
			}
		}
	}
}

struct FileItem {
	std::string srcpath;
	std::string dstpath;
	FileItem() = default;
	FileItem(std::string const &srcpath, std::string const &dstpath)
		: srcpath(srcpath)
		, dstpath(dstpath)
	{
	}
};

/**
 * @brief ディレクトリを再帰的にスキャンする
 * @param basedir
 * @param absdir
 * @param out
 */
void scandir(std::string const &basedir, std::string const &absdir, std::vector<FileItem> *out)
{
	DIR *dir = opendir((basedir / absdir).c_str());
	if (dir) {
		while (dirent *ent = readdir(dir)) {
			if (strcmp(ent->d_name, ".") == 0) continue;
			if (strcmp(ent->d_name, "..") == 0) continue;
			if (strcmp(ent->d_name, ".git") == 0) continue;
			std::string filename = ent->d_name;
			std::string path = absdir / filename;
			if (ent->d_type == DT_DIR) {
				scandir(basedir, path, out);
			} else if (ent->d_type == DT_REG) {
				auto dpath = absdir / filename;
				out->push_back({path, dpath});
			}
		}
		closedir(dir);
	}
}

/**
 * @brief 単語を置換する
 * @param s
 * @return 
 */
std::string Projector::replaceWords(std::string const &t)
{
	std::vector<char> vec = internalReplaceWords(t, words_);
	return std::string(vec.data(), vec.size());
}

/**
 * @brief プロジェクトを生成する
 * @param srcpath
 * @param dstpath
 * @return 
 */
bool Projector::perform(std::string const &srcpath, std::string const &dstpath, bool implace_edit)
{
	std::vector<FileItem> files;
	scandir(srcpath, {}, &files);
	
	struct stat stdst;
	if (stat(dstpath.c_str(), &stdst) == 0) {
		fprintf(stderr, "already existing: %s\n", dstpath.c_str());
		return false;
	}
	
	struct stat stsrc;
	if (stat(srcpath.c_str(), &stsrc) == 0) {
		if (S_ISREG(stsrc.st_mode)) {
			convert_file(srcpath, dstpath, words_, implace_edit);
		} else if (S_ISDIR(stsrc.st_mode)) {
			if (implace_edit) {
				fprintf(stderr, "implace edit is not supported for directory\n");
				return false;
			}
			for (FileItem const &item : files) {
				std::string s = srcpath / item.srcpath;
				std::string d = dstpath / replaceWords(item.dstpath);
				convert_file(s, d, words_, false);

			}
		}
	} else {
		fprintf(stderr, "Not found: %s\n", srcpath.c_str());
		return false;
	}
	
	return true;
}

bool Projector::perform(std::string const &srcpath, std::string const &dstpath)
{
	return perform(srcpath, dstpath, false);
}

bool Projector::perform_implace(std::string const &path)
{
	return perform(path, {}, true);
}

#ifdef USE_QT

#include <QDirIterator>
#include <QMessageBox>

/**
 * @brief ファイルを変換してコピーする
 * @param srcpath
 * @param dstpath
 * @param srcwords
 * @param dstwords
 */
void ProjectGenerator::convertFile(QString const &srcpath, QString const &dstpath, strlist_t const &srcwords, strlist_t const &dstwords)
{
	QFile infile(srcpath);
	if (infile.open(QFile::ReadOnly)) {
		auto i = dstpath.lastIndexOf('/');
		if (i > 0) {
			QDir().mkpath(dstpath.mid(0, i));
		}
		QByteArray ba = infile.readAll();
		QFile outfile(dstpath);
		if (outfile.open(QFile::WriteOnly)) {
			auto vec = internalReplaceWords(std::string_view(ba.begin(), ba.size()), srcwords, dstwords);
			outfile.write(vec.data(), vec.size());
		}
	}
}

struct qFileItem {
	QString srcpath;
	QString dstpath;
	qFileItem() = default;
	qFileItem(QString const &srcpath, QString const &dstpath)
		: srcpath(srcpath)
		, dstpath(dstpath)
	{
	}
};

/**
 * @brief ディレクトリを再帰的にスキャンする
 * @param basedir
 * @param absdir
 * @param out
 */
void scandir(QString const &basedir, QString const &absdir, std::vector<qFileItem> *out)
{
	QDirIterator it(basedir / absdir);
	while (it.hasNext()) {
		it.next();
		auto info = it.fileInfo();
		auto filename = info.fileName();
		if (filename.startsWith('.')) continue;
		if (info.isDir()) {
			scandir(basedir, absdir / filename, out);
		} else if (info.isFile()) {
			if (filename == ".git") continue;
			if (filename.endsWith(".user")) continue;
			QString path = absdir / filename;
			// "_."で始まるファイルは、先頭の"_"を削除
			auto dpath = absdir / (filename.startsWith("_.") ? filename.mid(1) : filename);
			//QString dpath = absdir / filename;
			out->push_back({path, dpath});
		}
	}
}

/**
 * @brief 単語を置換する
 * @param s
 * @return 
 */
QString ProjectGenerator::replaceWords(QString const &s)
{
	std::string t = s.toStdString();
	auto vec = internalReplaceWords(t, srcwords_, dstwords_);
	return QString::fromUtf8(vec.data(), vec.size());
}

/**
 * @brief プロジェクトを生成する
 * @param srcpath
 * @param dstpath
 * @return 
 */
bool ProjectGenerator::perform(QString const &srcpath, QString const &dstpath)
{
	std::vector<qFileItem> files;
	scandir(srcpath, {}, &files);
	
	QFileInfo srcinfo(srcpath);
	QFileInfo dstinfo(dstpath);
	
	if (dstinfo.exists()) {
		QMessageBox::warning(nullptr, "", "Already existing:\n" + dstpath);
		return false;
	}
	
	if (srcinfo.isFile()) {
		convertFile(srcpath, dstpath, srcwords_, dstwords_);
	} else if (srcinfo.isDir()) {
		for (qFileItem const &item : files) {
			QString s = srcpath / item.srcpath;
			QString d = dstpath / replaceWords(item.dstpath);
			convertFile(s, d, srcwords_, dstwords_);
		}
	}

	return true;
}

#endif // USE_QT
