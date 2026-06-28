#ifndef PROBLEMSPJ_H
#define PROBLEMSPJ_H

#include "ojproblemset.h"

#include <QString>

namespace ProblemSpj {

QString dataDirectory(const POJProblem &problem, const QString &problemSetFile = QString());
QString sourceFile(const POJProblem &problem, const QString &problemSetFile = QString());
QString executableFile(const QString &sourceFile);
QString metadataFile(const QString &sourceFile);
QString displayPath(const QString &path);

bool ensureSourceFile(const POJProblem &problem,
                      const QString &problemSetFile,
                      QString *sourceFile,
                      QString *errorMessage);
bool writeMetadata(const POJProblem &problem,
                   const QString &sourceFile,
                   QString *errorMessage);
bool compile(const QString &sourceFile,
             QString *executableFile,
             QString *errorMessage,
             QString *compilerOutput = nullptr);

}

#endif // PROBLEMSPJ_H
