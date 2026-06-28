/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "terminalprocess.h"

#include "../../systemconsts.h"

#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>

namespace {

#ifdef Q_OS_WIN
QString windowsShellProgram()
{
    const QString shell = QProcessEnvironment::systemEnvironment().value("SHELL");
    if (!shell.isEmpty()
            && (QFileInfo(shell).exists()
                || !QStandardPaths::findExecutable(shell).isEmpty()))
        return shell;
    const QString comspec = QProcessEnvironment::systemEnvironment().value("COMSPEC");
    return comspec.isEmpty() ? QStringLiteral("cmd.exe") : comspec;
}
#else
QString unixShellProgram()
{
    const QString shell = QProcessEnvironment::systemEnvironment().value("SHELL");
    return shell.isEmpty() ? QStringLiteral("/bin/sh") : shell;
}
#endif

QString shellBaseName(const QString &program)
{
    return QFileInfo(program).fileName().toLower();
}

bool isCmdShell(const QString &program)
{
    const QString name = shellBaseName(program);
    return name == QLatin1String("cmd") || name == QLatin1String("cmd.exe");
}

bool isPowerShell(const QString &program)
{
    const QString name = shellBaseName(program);
    return name == QLatin1String("powershell")
            || name == QLatin1String("powershell.exe")
            || name == QLatin1String("pwsh")
            || name == QLatin1String("pwsh.exe");
}

bool isBourneShell(const QString &program)
{
    const QString name = shellBaseName(program);
    return name == QLatin1String("bash")
            || name == QLatin1String("bash.exe")
            || name == QLatin1String("zsh")
            || name == QLatin1String("zsh.exe")
            || name == QLatin1String("sh")
            || name == QLatin1String("sh.exe")
            || name == QLatin1String("dash")
            || name == QLatin1String("dash.exe")
            || name == QLatin1String("ksh")
            || name == QLatin1String("ksh.exe")
            || name == QLatin1String("fish")
            || name == QLatin1String("fish.exe");
}

QString defaultUtf8Locale()
{
#if defined(Q_OS_MACOS) || defined(Q_OS_FREEBSD) || defined(Q_OS_OPENBSD) || defined(Q_OS_NETBSD)
    return QStringLiteral("en_US.UTF-8");
#else
    return QStringLiteral("C.UTF-8");
#endif
}

QString quoteBournePath(QString path)
{
    path.replace('\'', "'\\''");
    return QStringLiteral("'%1'").arg(path);
}

QString quotePowerShellPath(QString path)
{
    path.replace('\'', "''");
    return QStringLiteral("'%1'").arg(path);
}

}

TerminalProcess::TerminalProcess(QObject *parent)
    : QObject(parent)
    , mCurrentDir(QDir::current())
{
}

TerminalProcess::~TerminalProcess()
{
    stop();
}

bool TerminalProcess::execute(const QString &commandLine)
{
    QString trimmed = commandLine.trimmed();
    if (trimmed.isEmpty())
        return false;

    mHistory.append(commandLine);
    mHistoryIndex = mHistory.size();

    if (handleBuiltin(trimmed))
        return true;

    ensureStarted();
    if (!mProcess || mProcess->state() == QProcess::NotRunning)
        return false;

    writeCommand(commandLine);
    return true;
}

void TerminalProcess::stop()
{
    if (mProcess && mProcess->state() != QProcess::NotRunning) {
        writeCommand(QStringLiteral("exit"));
        mProcess->closeWriteChannel();
        if (!mProcess->waitForFinished(1000))
            mProcess->kill();
        mProcess->waitForFinished(2000);
    }
    if (mProcess) {
        mProcess->deleteLater();
        mProcess = nullptr;
    }
    mRunning = false;
}

QString TerminalProcess::workingDirectory() const
{
    return mCurrentDir.absolutePath();
}

void TerminalProcess::setWorkingDirectory(const QString &path)
{
    QDir d(path);
    if (d.exists()) {
        mCurrentDir = d;
        emit workingDirectoryChanged(mCurrentDir.absolutePath());
        if (mProcess && mProcess->state() != QProcess::NotRunning) {
            writeCommand(changeDirectoryCommand(mCurrentDir.absolutePath()));
        }
    }
}

void TerminalProcess::setExtraBinDirs(const QStringList &dirs)
{
    if (mExtraBinDirs == dirs)
        return;
    mExtraBinDirs = dirs;
    stop();
}

bool TerminalProcess::handleBuiltin(const QString &line)
{
    if (line == "clear" || line == "cls") {
        emit outputReady("<span class='terminal-clear'></span>");
        return true;
    }
    if (line == "cwd") {
        emit outputReady(mCurrentDir.absolutePath().toHtmlEscaped() + "\n");
        return true;
    }
    return false;
}

void TerminalProcess::ensureStarted()
{
    if (!mProcess || mProcess->state() == QProcess::NotRunning)
        startShell();
}

void TerminalProcess::startShell()
{
    if (mProcess)
        mProcess->deleteLater();
    mProcess = new QProcess(this);
    mProcess->setWorkingDirectory(mCurrentDir.absolutePath());
    mProcess->setProcessChannelMode(QProcess::MergedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const QString utf8Locale = defaultUtf8Locale();
    if (env.value("LANG").isEmpty())
        env.insert("LANG", utf8Locale);
    if (env.value("LC_CTYPE").isEmpty())
        env.insert("LC_CTYPE", utf8Locale);
    QString path = env.value("PATH");
    QStringList pathAdded = mExtraBinDirs;
    if (!pathAdded.isEmpty()) {
        if (path.isEmpty())
            path = pathAdded.join(PATH_SEPARATOR);
        else
            path = pathAdded.join(PATH_SEPARATOR) + PATH_SEPARATOR + path;
        env.insert("PATH", path);
    }
    mProcess->setProcessEnvironment(env);

    connect(mProcess, &QProcess::started,
            this, &TerminalProcess::onProcessStarted);
    connect(mProcess, &QProcess::readyRead,
            this, &TerminalProcess::onReadyReadStdout);
    connect(mProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &TerminalProcess::onProcessFinished);
    connect(mProcess, &QProcess::errorOccurred,
            this, &TerminalProcess::onProcessError);

    mPendingOutput.clear();
    mProcess->start(shellProgram(), shellArguments());
}

QString TerminalProcess::shellProgram() const
{
#ifdef Q_OS_WIN
    return windowsShellProgram();
#else
    return unixShellProgram();
#endif
}

QStringList TerminalProcess::shellArguments() const
{
    const QString program = shellProgram();
#ifdef Q_OS_WIN
    if (isPowerShell(program)) {
        return {
            QStringLiteral("-NoLogo"),
            QStringLiteral("-NoExit"),
            QStringLiteral("-NoProfile"),
            QStringLiteral("-ExecutionPolicy"),
            QStringLiteral("Bypass"),
            QStringLiteral("-Command"),
            QStringLiteral("$utf8 = New-Object System.Text.UTF8Encoding $false; [Console]::InputEncoding = $utf8; [Console]::OutputEncoding = $utf8; $OutputEncoding = $utf8")
        };
    }
    if (isCmdShell(program))
        return {QStringLiteral("/Q"), QStringLiteral("/K"), QStringLiteral("chcp 65001>nul")};
    if (isBourneShell(program))
        return {QStringLiteral("-i")};
    return QStringList();
#else
    const QString name = shellBaseName(program);
    if (name == QLatin1String("sh"))
        return QStringList();
    return {QStringLiteral("-i")};
#endif
}

QByteArray TerminalProcess::lineEnding() const
{
#ifdef Q_OS_WIN
    const QString program = shellProgram();
    if (isCmdShell(program) || isPowerShell(program))
        return "\r\n";
#endif
    return "\n";
}

QString TerminalProcess::changeDirectoryCommand(const QString &path) const
{
    const QString program = shellProgram();
#ifdef Q_OS_WIN
    if (isPowerShell(program))
        return QStringLiteral("Set-Location -LiteralPath %1").arg(quotePowerShellPath(QDir::toNativeSeparators(path)));
    if (isCmdShell(program))
        return QStringLiteral("cd /d \"%1\"").arg(QDir::toNativeSeparators(path));
#endif
    return QStringLiteral("cd %1").arg(quoteBournePath(path));
}

void TerminalProcess::writeCommand(const QString &command)
{
    if (!mProcess || mProcess->state() == QProcess::NotRunning)
        return;
    QByteArray data = command.toUtf8();
    data.append(lineEnding());
    mProcess->write(data);
}

void TerminalProcess::onProcessStarted()
{
    mRunning = true;
}

void TerminalProcess::onReadyReadStdout()
{
    if (!mProcess) return;
    QByteArray data = mProcess->readAll();
    emit outputReady(QString::fromUtf8(parseAnsiToHtml(data)));
}

void TerminalProcess::onReadyReadStderr()
{
    // Merged channel, handled in stdout
}

void TerminalProcess::onProcessFinished(int exitCode, QProcess::ExitStatus)
{
    mRunning = false;
    if (mProcess) {
        // Flush any remaining output
        QByteArray remaining = mProcess->readAll();
        if (!remaining.isEmpty())
            emit outputReady(QString::fromUtf8(parseAnsiToHtml(remaining)));
        mProcess->deleteLater();
        mProcess = nullptr;
    }
    emit commandFinished(exitCode);
}

void TerminalProcess::onProcessError(QProcess::ProcessError error)
{
    mRunning = false;
    QString msg;
    switch (error) {
    case QProcess::FailedToStart:
        msg = tr("Failed to start program");
        break;
    case QProcess::Timedout:
        msg = tr("Process timed out");
        break;
    default:
        msg = tr("Process error: %1").arg(error);
        break;
    }
    emit outputReady(QString("<span style='color:#e06c75'>%1</span>\n").arg(msg.toHtmlEscaped()));
    if (mProcess) {
        mProcess->deleteLater();
        mProcess = nullptr;
    }
}

// Minimal ANSI SGR parser: converts common color/reset sequences to HTML spans.
// Tracks span nesting to properly close spans on ESC[0m reset.
QByteArray TerminalProcess::parseAnsiToHtml(const QByteArray &data)
{
    QByteArray result;
    result.reserve(data.size() * 2);

    int i = 0;
    bool inEscape = false;
    QByteArray escapeBuf;
    int openSpanCount = 0;

    while (i < data.size()) {
        unsigned char ch = static_cast<unsigned char>(data[i]);

        if (!inEscape) {
            if (ch == 0x1B && i + 1 < data.size() && data[i + 1] == '[') {
                inEscape = true;
                escapeBuf.clear();
                i += 2; // skip ESC [
                continue;
            }
            switch (ch) {
            case '<': result += "&lt;"; break;
            case '>': result += "&gt;"; break;
            case '&': result += "&amp;"; break;
            case '\r': break; // ignore CR
            case '\n': result += "<br>"; break;
            case '\t': result += "    "; break;
            default: result += ch; break;
            }
            i++;
        } else {
            if (ch >= 0x40 && ch <= 0x7E) {
                inEscape = false;
                QByteArray param = escapeBuf;
                escapeBuf.clear();

                if (ch == 'm') {
                    if (param.isEmpty() || param == "0") {
                        // Close all open spans for reset
                        while (openSpanCount > 0) {
                            result += "</span>";
                            openSpanCount--;
                        }
                    } else {
                        QByteArray style;
                        QList<QByteArray> codes = param.split(';');
                        for (const QByteArray &code : codes) {
                            int n = code.toInt();
                            switch (n) {
                            case 1:  style += "font-weight:bold;"; break;
                            case 4:  style += "text-decoration:underline;"; break;
                            case 30: style += "color:#000000;"; break;
                            case 31: style += "color:#e06c75;"; break;
                            case 32: style += "color:#98c379;"; break;
                            case 33: style += "color:#e5c07b;"; break;
                            case 34: style += "color:#61afef;"; break;
                            case 35: style += "color:#c678dd;"; break;
                            case 36: style += "color:#56b6c2;"; break;
                            case 37: style += "color:#abb2bf;"; break;
                            default: break;
                            }
                        }
                        if (!style.isEmpty()) {
                            result += "<span style='" + style + "'>";
                            openSpanCount++;
                        }
                    }
                }
                i++;
            } else {
                escapeBuf += ch;
                i++;
            }
        }
    }

    // Close any remaining open spans
    while (openSpanCount > 0) {
        result += "</span>";
        openSpanCount--;
    }

    return result;
}
