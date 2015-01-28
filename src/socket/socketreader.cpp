/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2015 Nathan Osman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 **/

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTcpSocket>

#include "../util/settings.h"
#include "socketreader.h"
#include "socketstream.h"

SocketReader::SocketReader(qintptr socketDescriptor)
    : mSocketDescriptor(socketDescriptor)
{
}

void SocketReader::start()
{
    QTcpSocket socket;
    SocketStream stream(socket);

    try {
        if(!socket.setSocketDescriptor(mSocketDescriptor)) {
            throw tr("Invalid socket descriptor.");
        }

        if(stream.readInt<qint32>() != 1) {
            throw tr("Protocol version mismatch.");
        }

        emit deviceName(stream.readQByteArray());

        // TODO: use this for determining progress
        stream.readInt<qint64>();

        QDir root(Settings::get<QString>(Settings::TransferDirectory));

        for(int count(stream.readInt<qint32>()); count; --count) {
            QString filename(stream.readQByteArray());
            FileInfo info(root, filename);

            QDir path(QFileInfo(info.absoluteFilename()).path());
            if(!path.exists()) {
                if(!path.mkpath(".")) {
                    throw tr("Unable to create %1.").arg(path.absolutePath());
                }
            }

            QFile file(info.absoluteFilename());
            if(!file.open(QIODevice::WriteOnly)) {
                throw tr("Unable to open %1 for writing.").arg(info.absoluteFilename());
            }

            qint64 fileSize(stream.readInt<qint64>());

            while(fileSize) {
                QByteArray data(qUncompress(stream.readQByteArray()));

                file.write(data);
                fileSize -= data.length();
            }
        }

        emit completed();
    } catch(const QString &exception) {
        emit error(exception);
    }
}