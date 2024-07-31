#ifndef PROJECTOR_H
#define PROJECTOR_H

#include <string>
#include <vector>

#ifdef USE_QT
class QString;
#endif

class Projector {
private:
	typedef std::vector<std::string> strlist_t;

	struct WordsPair {
		strlist_t srcwords;
		strlist_t dstwords;
	};
	std::vector<WordsPair> words_;

	static std::vector<char> internalReplaceWords(const std::string_view &srctext, const strlist_t &srcwords, const strlist_t &dstwords);
	static std::vector<char> internalReplaceWords(const std::string_view &srctext, const std::vector<WordsPair> &words);
	static void convert_file(const std::string &srcpath, const std::string &dstpath, const std::vector<WordsPair> &words, bool implace_edit);
	static strlist_t split(const std::string_view &s);
	bool perform(const std::string &srcpath, const std::string &dstpath, bool implace_edit);
public:
	Projector(std::vector<std::pair<std::string_view, std::string_view>> &&rules);
	std::string replaceWords(const std::string &s);
	bool perform(const std::string &srcpath, const std::string &dstpath);
	bool perform_implace(const std::string &path);
#ifdef USE_QT
private:
	static void convertFile(const QString &srcpath, const QString &dstpath, const strlist_t &srcwords, const strlist_t &dstwords);
public:
	QString replaceWords(const QString &s);
	bool perform(QString const &srcpath, QString const &dstpath);
#endif // USE_QT
};
#endif // PROJECTOR_H
