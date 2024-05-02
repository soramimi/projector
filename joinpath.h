
#ifndef JOINPATH_H
#define JOINPATH_H

#include <string>

std::string joinpath(char const *left, char const *right);
std::string joinpath(std::string const &left, std::string const &right);

static inline std::string operator / (std::string const &left, std::string const &right)
{
	return joinpath(left, right);
}

#ifdef USE_QT
#include <QString>
QString qjoinpath(ushort const *left, ushort const *right);
inline QString joinpath(QString const &left, QString const &right)
{
	return qjoinpath(left.utf16(), right.utf16());
}

static inline QString operator / (QString const &left, QString const &right)
{
	return joinpath(left, right);
}

#endif // USE_QT

#endif // JOINPATH_H
