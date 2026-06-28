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
#include "../src/utils.h"

#include <QDir>
#include <QFile>
#include <QTextStream>

QStringList textToLines(const QString &text)
{
    QString copy = text;
    QTextStream stream(&copy, QIODevice::ReadOnly);
    QStringList result;
    QString line;
    while (stream.readLineInto(&line))
        result.append(line);
    return result;
}

void textToLines(const QString &text, LineProcessFunc lineFunc)
{
    QString copy = text;
    QTextStream stream(&copy, QIODevice::ReadOnly);
    QString line;
    while (stream.readLineInto(&line))
        lineFunc(line);
}

QStringList readFileToLines(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QFile::ReadOnly | QFile::Text))
        return {};

    QTextStream stream(&file);
    QStringList result;
    QString line;
    while (stream.readLineInto(&line))
        result.append(line);
    return result;
}

bool stringToFile(const QString &str, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
        return false;

    QTextStream stream(&file);
    stream << str;
    return true;
}

bool fileExists(const QString &file)
{
    return !file.isEmpty() && QFile::exists(file);
}

bool fileExists(const QString &dir, const QString &fileName)
{
    return !dir.isEmpty() && !fileName.isEmpty() && QDir(dir).exists(fileName);
}

const QChar *getNullTerminatedStringData(const QString &str)
{
    const QChar *result = str.constData();
    if (result[str.size()] != QChar(0))
        result = str.data();
    return result;
}
