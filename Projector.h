#ifndef PROJECTOR_H
#define PROJECTOR_H

#include <string>
#include <vector>

#ifdef USE_QT
class QString;
#endif

class Projector {
private:
	std::vector<std::string> srcwords_;
	std::vector<std::string> dstwords_;
	static std::vector<char> internalReplaceWords(const std::string_view &srctext, const std::vector<std::string> &srcwords, const std::vector<std::string> &dstwords);
	static void convertFile(const std::string &srcpath, const std::string &dstpath, const std::vector<std::string> &srcwords, const std::vector<std::string> &dstwords);
public:
	Projector(const std::string_view &srcname, const std::string_view &dstname);
	std::string replaceWords(const std::string &s);
	bool perform(const std::string &srcpath, const std::string &dstpath);
#ifdef USE_QT
private:
	static void convertFile(const QString &srcpath, const QString &dstpath, const std::vector<std::string> &srcwords, const std::vector<std::string> &dstwords);
public:
	QString replaceWords(const QString &s);
	bool perform(QString const &srcpath, QString const &dstpath);
#endif // USE_QT
};
#endif // PROJECTOR_H
