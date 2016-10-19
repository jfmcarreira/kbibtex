/***************************************************************************
 *   Copyright (C) 2004-2014 by Thomas Fischer <fischer@unix-ag.uni-kl.de> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "iconvlatex.h"

#include <iconv.h>

#include <QStringList>

#include "encoderlatex.h"

QStringList IConvLaTeX::encodingList;

class IConvLaTeX::IConvLaTeXPrivate
{
private:
    // UNUSED IConvLaTeX *p;

public:
    const QString destEncoding;
    iconv_t iconvHandle;

    IConvLaTeXPrivate(IConvLaTeX */* UNUSED parent*/, const QString &_destEncoding)
            : destEncoding(_destEncoding) // , UNUSED p(parent)
    {
        iconvHandle = iconv_open(destEncoding.toLatin1().data(), "utf-8");
    }

    ~IConvLaTeXPrivate()
    {
        iconv_close(iconvHandle);
    }
};

IConvLaTeX::IConvLaTeX(const QString &destEncoding)
        : d(new IConvLaTeXPrivate(this, destEncoding))
{
    /// nothing
}

IConvLaTeX::IConvLaTeX(const IConvLaTeX &other)
        : d(new IConvLaTeXPrivate(this, other.d->destEncoding))
{
    /// nothing
}

IConvLaTeX::~IConvLaTeX()
{
    delete d;
}

IConvLaTeX &IConvLaTeX::operator=(const IConvLaTeX &other) {
    delete d;
    d = new IConvLaTeXPrivate(this, other.d->destEncoding);
    return *this;
}

QByteArray IConvLaTeX::encode(const QString &ninput)
{
    /// Perform Canonical Decomposition followed by Canonical Composition
    const QString input = ninput.normalized(QString::NormalizationForm_C);
    /// Get an UTF-8 representation of the input string
    QByteArray inputByteArray = input.toUtf8();
    /// Limit the size of the output buffer
    /// by making an educated guess of its maximum size
    size_t maxOutputByteArraySize = inputByteArray.size() * 4 + 1024;
#ifdef Q_WS_WIN
    /// iconv on Windows likes to have it as const char *
    const char *inputBuffer = inputByteArray.data();
#else
    /// iconv on Linux likes to have it as char *
    char *inputBuffer = inputByteArray.data();
#endif
    QByteArray outputByteArray(maxOutputByteArraySize, '\0');
    char *outputBuffer = outputByteArray.data();
    size_t inputBufferBytesLeft = inputByteArray.size();
    size_t ouputBufferBytesLeft = maxOutputByteArraySize;
    Encoder *laTeXEncoder = EncoderLaTeX::instance();

    while (iconv(d->iconvHandle, &inputBuffer, &inputBufferBytesLeft, &outputBuffer, &ouputBufferBytesLeft) == (size_t)(-1) && inputBufferBytesLeft > 0) {
        /// split text into character where iconv stopped and remaining text
        QString remainingString = QString::fromUtf8(inputBuffer);
        QChar problematicChar = remainingString.at(0);
        remainingString = remainingString.mid(1);

        /// setup input buffer to continue with remaining text
        inputByteArray = remainingString.toUtf8();
        inputBuffer = inputByteArray.data();
        inputBufferBytesLeft = inputByteArray.size();

        /// encode problematic character in LaTeX encoding and append to output buffer
        const QString encodedProblem = laTeXEncoder->encode(problematicChar);
        QByteArray encodedProblemByteArray = encodedProblem.toUtf8();
        qstrncpy(outputBuffer, encodedProblemByteArray.data(), ouputBufferBytesLeft);
        ouputBufferBytesLeft -= encodedProblemByteArray.size();
        outputBuffer += encodedProblemByteArray.size();
    }

    /// cut away unused space
    outputByteArray.resize(maxOutputByteArraySize - ouputBufferBytesLeft);

    return outputByteArray;
}

const QStringList IConvLaTeX::encodings()
{
    if (encodingList.isEmpty()) {
        /* FIXME this list will contain encodings that are irreversible!
        QProcess iconvProgram;
        QStringList iconvProgramArgs = QStringList() << "--list";
        iconvProgram.start(QStringLiteral("iconv"), iconvProgramArgs);
        iconvProgram.waitForStarted(10000);
        if (iconvProgram.state() == QProcess::Running) {
            iconvProgram.waitForReadyRead(10000);
            encodingList.clear();
            QString allText = "";
            while (iconvProgram.canReadLine()) {
                allText += iconvProgram.readAllStandardOutput();
                iconvProgram.waitForReadyRead(10000);
            }
            iconvProgram.waitForFinished(10000);
            iconvProgram.close();

            encodingList = allText.replace("//", "").split('\n', QString::SkipEmptyParts);
        }
        */

        /// approved encodings manually added to list
        int dosCodepages[] = {437, 720, 737, 775, 850, 852, 855, 857, 858, 860, 861, 862, 863, 864, 865, 866, 869, -1};
        int windowsCodepages[] = {1250, 1251, 1252, 1253, 1254, 1255, 1256, 1257, 1258, -1};
        for (int *cur = dosCodepages; *cur > 0; ++cur)
            encodingList << QStringLiteral("CP") + QString::number(*cur);
        for (int *cur = windowsCodepages; *cur > 0; ++cur)
            encodingList << QStringLiteral("CP") + QString::number(*cur);
        for (int i = 1; i <= 16; ++i)
            encodingList << QStringLiteral("ISO-8859-") + QString::number(i);
        encodingList << QStringLiteral("KOI8-R");
        for (int i = 1; i <= 10; ++i)
            encodingList << QStringLiteral("Latin") + QString::number(i);
        encodingList << QStringLiteral("UTF-8");
        for (int *cur = windowsCodepages; *cur > 0; ++cur)
            encodingList << QStringLiteral("Windows-") + QString::number(*cur);
    }

    return encodingList;
}
