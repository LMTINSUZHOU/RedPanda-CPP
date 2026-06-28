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

namespace {

#ifdef Q_OS_WIN
QString windowsShellProgram()
{
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

    mHistory.append(trimmed);
    mHistoryIndex = mHistory.size();

    if (handleBuiltin(trimmed))
        return true;

    ensureStarted();
    if (!mProcess || mProcess->state() == QProcess::NotRunning)
        return false;

    QByteArray command = commandLine.toLocal8Bit();
#ifdef Q_OS_WIN
    command.append("\r\n");
#else
    command.append('\n');
#endif
    mProcess->write(command);
    return true;
}

void TerminalProcess::stop()
{
    if (mProcess && mProcess->state() != QProcess::NotRunning) {
#ifdef Q_OS_WIN
        mProcess->write("exit\r\n");
#else
        mProcess->write("exit\n");
#endif
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
#ifdef Q_OS_WIN
            const QString command = QStringLiteral("cd /d \"%1\"\r\n").arg(QDir::toNativeSeparators(mCurrentDir.absolutePath()));
#else
            QString escaped = mCurrentDir.absolutePath();
            escaped.replace('\'', "'\\''");
            const QString command = QStringLiteral("cd '%1'\n").arg(escaped);
#endif
            mProcess->write(command.toLocal8Bit());
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
#ifdef Q_OS_WIN
    return {QStringLiteral("/Q"), QStringLiteral("/K")};
#else
    if (QFileInfo(shellProgram()).fileName() == QLatin1String("sh"))
        return QStringList();
    return {QStringLiteral("-i")};
#endif
}

void TerminalProcess::onProcessStarted()
{
    mRunning = true;
}

void TerminalProcess::onReadyReadStdout()
{
    if (!mProcess) return;
    QByteArray data = mProcess->readAll();
    emit outputReady(parseAnsiToHtml(data));
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
            emit outputReady(parseAnsiToHtml(remaining));
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
