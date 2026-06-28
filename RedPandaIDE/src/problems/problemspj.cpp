#include "problemspj.h"

#ifndef PROBLEMSPJ_NO_COMPILER_SUPPORT
#include "../settings.h"
#include "../settings/dirsettings.h"
#endif
#include "../systemconsts.h"
#include "../utils.h"
#include "../utils/file.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>
#include <QRegularExpression>

namespace {

constexpr char RedPandaProblemDir[] = ".redpd";
constexpr char SpjSourceName[] = "spj.cpp";
constexpr char SpjExecutableBaseName[] = "spj";
constexpr char SpjMetadataName[] = "spj.json";
constexpr char TestlibHeaderName[] = "testlib.h";

bool isCppSourceFilename(const QString &filename)
{
    const QString suffix = QFileInfo(filename).suffix().toLower();
    return suffix == "cpp" || suffix == "cc" || suffix == "cxx" || suffix == "c++";
}

bool isRedPandaSpjSourceFile(const QString &filename)
{
    QFileInfo info(filename);
    if (!isCppSourceFilename(filename)
            || info.fileName().compare(SpjSourceName, PATH_SENSITIVITY) != 0)
        return false;

    QDir dir(info.absolutePath());
    while (!dir.isRoot()) {
        if (dir.dirName().compare(RedPandaProblemDir, PATH_SENSITIVITY) == 0)
            return true;
        if (!dir.cdUp())
            break;
    }
    return false;
}

QString problemBaseDirectory(const POJProblem &problem, const QString &problemSetFile)
{
    if (problem && !problem->answerProgram().isEmpty())
        return QFileInfo(problem->answerProgram()).absolutePath();
    if (!problemSetFile.isEmpty())
        return QFileInfo(problemSetFile).absolutePath();
    return QDir::currentPath();
}

QString redPandaDataRootDirectory(const POJProblem &problem, const QString &problemSetFile)
{
    return QDir(problemBaseDirectory(problem, problemSetFile)).absoluteFilePath(RedPandaProblemDir);
}

QString testlibIncludeDir()
{
    QString includeDir;
#ifndef PROBLEMSPJ_NO_COMPILER_SUPPORT
    includeDir = getFilePath(DirSettings::appResourceDir(), "include");
    if (fileExists(getFilePath(includeDir, TestlibHeaderName)))
        return includeDir;
#endif

    includeDir = QFileInfo(QStringLiteral(__FILE__))
            .absoluteDir()
            .absoluteFilePath("../../resources/include");
    includeDir = QDir::cleanPath(includeDir);
    if (fileExists(getFilePath(includeDir, TestlibHeaderName)))
        return includeDir;

    return QString();
}

QString normalizePathSegment(const QString &value)
{
    QString result = value.trimmed();
    if (result.startsWith('{') && result.endsWith('}') && result.length() > 2)
        result = result.mid(1, result.length() - 2);
    result.replace(QRegularExpression(QStringLiteral("[\\\\/:*?\"<>|\\s]+")), QStringLiteral("_"));
    result.replace(QRegularExpression(QStringLiteral("_+")), QStringLiteral("_"));
    result = result.trimmed();
    while (result.startsWith('.'))
        result.remove(0, 1);
    while (result.endsWith('.'))
        result.chop(1);
    if (result.isEmpty())
        result = QStringLiteral("problem");
    return result;
}

QString problemDataSubdirectoryName(const POJProblem &problem)
{
    if (problem && !problem->id().isEmpty())
        return normalizePathSegment(problem->id());
    if (problem && !problem->name().isEmpty())
        return normalizePathSegment(problem->name());
    return QStringLiteral("problem");
}

QString testlibHeaderFile()
{
    const QString includeDir = testlibIncludeDir();
    if (includeDir.isEmpty())
        return QString();
    return getFilePath(includeDir, TestlibHeaderName);
}

bool ensureLocalTestlib(const QString &dirPath, QString *errorMessage)
{
    const QString sourceFile = testlibHeaderFile();
    if (sourceFile.isEmpty()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't find testlib.h.");
        return false;
    }

    const QString targetFile = QDir(dirPath).absoluteFilePath(TestlibHeaderName);
    if (fileExists(targetFile)
            && compareFileModifiedTime(sourceFile, targetFile) <= 0)
        return true;

    if (!copyFile(sourceFile, targetFile, true)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't copy testlib.h to \"%1\".").arg(dirPath);
        return false;
    }
    return true;
}

QString defaultSpjSource()
{
    return QStringLiteral(
R"(#include "../testlib.h"

int main(int argc, char *argv[])
{
    registerTestlibCmd(argc, argv);

    // Read from inf, ouf and ans. Call quitf(_ok, ...) or quitf(_wa, ...).
    // Example:
    // int participant = ouf.readInt();
    // int expected = ans.readInt();
    // if (participant == expected)
    //     quitf(_ok, "accepted");
    // quitf(_wa, "expected %d, found %d", expected, participant);

    quitf(_ok, "accepted");
}
)");
}

void setProcessPath(QProcess &process, const QString &compiler)
{
#ifndef PROBLEMSPJ_NO_COMPILER_SUPPORT
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList pathDirs;
    const QString compilerDir = QFileInfo(compiler).absolutePath();
    if (!compilerDir.isEmpty())
        pathDirs << compilerDir;
    if (pSettings && pSettings->compilerSets().defaultSet())
        pathDirs << pSettings->compilerSets().defaultSet()->binDirs();
#ifdef Q_OS_WIN
    const QString winDir = env.value("windir");
    if (!winDir.isEmpty())
        pathDirs << winDir + "\\system32" << winDir;
#endif
    QString path = env.value("PATH");
    if (!pathDirs.isEmpty()) {
        path = pathDirs.join(PATH_SEPARATOR) + (path.isEmpty() ? QString() : PATH_SEPARATOR + path);
        env.insert("PATH", path);
    }
    process.setProcessEnvironment(env);
#else
    Q_UNUSED(process);
    Q_UNUSED(compiler);
#endif
}

}

namespace ProblemSpj {

QString dataDirectory(const POJProblem &problem, const QString &problemSetFile)
{
    if (problem && isRedPandaSpjSourceFile(problem->customSpjProgram()))
        return QFileInfo(problem->customSpjProgram()).absolutePath();
    QDir dir(redPandaDataRootDirectory(problem, problemSetFile));
    return dir.absoluteFilePath(problemDataSubdirectoryName(problem));
}

QString sourceFile(const POJProblem &problem, const QString &problemSetFile)
{
    if (problem && isCppSourceFilename(problem->customSpjProgram()))
        return problem->customSpjProgram();
    return QDir(dataDirectory(problem, problemSetFile)).absoluteFilePath(SpjSourceName);
}

QString executableFile(const QString &sourceFile)
{
    QString suffix = DEFAULT_EXECUTABLE_SUFFIX;
#ifndef PROBLEMSPJ_NO_COMPILER_SUPPORT
    if (pSettings && pSettings->compilerSets().defaultSet())
        suffix = pSettings->compilerSets().defaultSet()->executableSuffix();
#endif
    return QDir(QFileInfo(sourceFile).absolutePath()).absoluteFilePath(SpjExecutableBaseName + suffix);
}

QString metadataFile(const QString &sourceFile)
{
    return QDir(QFileInfo(sourceFile).absolutePath()).absoluteFilePath(SpjMetadataName);
}

QString displayPath(const QString &path)
{
    if (path.isEmpty())
        return QString();
    return QDir::toNativeSeparators(path);
}

bool ensureSourceFile(const POJProblem &problem,
                      const QString &problemSetFile,
                      QString *outSourceFile,
                      QString *errorMessage)
{
    const QString filename = sourceFile(problem, problemSetFile);
    QDir rootDir(redPandaDataRootDirectory(problem, problemSetFile));
    if (!rootDir.exists() && !rootDir.mkpath(QStringLiteral("."))) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't create folder \"%1\".").arg(rootDir.absolutePath());
        return false;
    }
    if (!ensureLocalTestlib(rootDir.absolutePath(), errorMessage))
        return false;

    QDir dir(QFileInfo(filename).absolutePath());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't create folder \"%1\".").arg(dir.absolutePath());
        return false;
    }
    if (!fileExists(filename) && !stringToFile(defaultSpjSource(), filename)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't create file \"%1\".").arg(filename);
        return false;
    }
    if (problem && problem->customSpjProgram().compare(filename, PATH_SENSITIVITY) != 0)
        problem->setCustomSpjProgram(filename);
    if (!writeMetadata(problem, filename, errorMessage))
        return false;
    if (outSourceFile)
        *outSourceFile = filename;
    return true;
}

bool writeMetadata(const POJProblem &problem,
                   const QString &sourceFile,
                   QString *errorMessage)
{
    QJsonObject obj;
    obj["version"] = 1;
    obj["spj_source"] = QFileInfo(sourceFile).fileName();
    obj["spj_executable"] = QFileInfo(executableFile(sourceFile)).fileName();
    if (problem) {
        obj["problem_id"] = problem->id();
        obj["problem_name"] = problem->name();
        obj["problem_url"] = problem->url();
        obj["answer_program"] = problem->answerProgram();
    }

    QFile file(metadataFile(sourceFile));
    if (!file.open(QFile::WriteOnly | QFile::Truncate)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't write file \"%1\".").arg(file.fileName());
        return false;
    }
    file.write(QJsonDocument(obj).toJson());
    return true;
}

bool compile(const QString &sourceFile,
             QString *outExecutableFile,
             QString *errorMessage,
             QString *compilerOutput)
{
#ifdef PROBLEMSPJ_NO_COMPILER_SUPPORT
    Q_UNUSED(sourceFile);
    Q_UNUSED(outExecutableFile);
    Q_UNUSED(compilerOutput);
    if (errorMessage)
        *errorMessage = QObject::tr("SPJ compilation is not available in this build.");
    return false;
#else
    if (sourceFile.isEmpty() || !fileExists(sourceFile)) {
        if (errorMessage)
            *errorMessage = QObject::tr("SPJ source file doesn't exist.");
        return false;
    }
    if (!pSettings || !pSettings->compilerSets().defaultSet()) {
        if (errorMessage)
            *errorMessage = QObject::tr("No compiler set is available.");
        return false;
    }

    PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    const QString compiler = compilerSet->cppCompiler();
    if (compiler.isEmpty() || !fileExists(compiler)) {
        if (errorMessage)
            *errorMessage = QObject::tr("The C++ compiler \"%1\" doesn't exist.").arg(compiler);
        return false;
    }

    QString outputFile = executableFile(sourceFile);
    QFile oldOutput(outputFile);
    if (oldOutput.exists() && !oldOutput.remove()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't delete the old SPJ executable \"%1\".").arg(outputFile);
        return false;
    }

    QStringList arguments;
    const QString includeDir = testlibIncludeDir();
    if (includeDir.isEmpty()) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't find testlib.h for compiling SPJ.");
        return false;
    }
    arguments << "-I" + includeDir;
    QDir sourceDir(QFileInfo(sourceFile).absolutePath());
    if (fileExists(sourceDir.absoluteFilePath(TestlibHeaderName)))
        arguments << "-I" + sourceDir.absolutePath();
    QDir sourceParentDir(sourceDir);
    if (sourceParentDir.cdUp() && fileExists(sourceParentDir.absoluteFilePath(TestlibHeaderName)))
        arguments << "-I" + sourceParentDir.absolutePath();
    arguments << "-include" << "testlib.h";
    arguments << sourceFile << "-o" << outputFile;

    QProcess process;
    process.setProgram(compiler);
    process.setArguments(arguments);
    process.setWorkingDirectory(QFileInfo(sourceFile).absolutePath());
    setProcessPath(process, compiler);
    process.start();
    if (!process.waitForStarted(5000)) {
        if (errorMessage)
            *errorMessage = QObject::tr("Can't start the C++ compiler for SPJ.");
        return false;
    }
    if (!process.waitForFinished(30000)) {
        process.kill();
        process.waitForFinished(1000);
        if (errorMessage)
            *errorMessage = QObject::tr("Compiling SPJ timed out.");
        return false;
    }

    const QString standardOutput = fromByteArray(process.readAllStandardOutput());
    const QString standardError = fromByteArray(process.readAllStandardError());
    if (compilerOutput)
        *compilerOutput = standardOutput + standardError;

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0 || !fileExists(outputFile)) {
        if (errorMessage) {
            QString message = QObject::tr("Failed to compile SPJ.");
            const QString output = (standardOutput + standardError).trimmed();
            if (!output.isEmpty())
                message += "\n" + output;
            *errorMessage = message;
        }
        return false;
    }

    if (outExecutableFile)
        *outExecutableFile = outputFile;
    return true;
#endif
}

}
