// Copyright 2019 Kasun Hewage
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "remote_thr.hpp"
#include <cstdint>
#include <qzmq.hpp>
#include <zmq.h>
#include <cstdio>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QCommandLineParser>
#include <QString>

App::App(int &argc, char **argv) : QCoreApplication(argc, argv)
{

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("connect_to", "connect to");
    parser.addPositionalArgument("size", "message size");
    parser.addPositionalArgument("count", "roundtrip count");
    parser.process(*this);

    const QStringList args = parser.positionalArguments();

    if (args.length() != 3) {
        parser.showHelp(-1);
        return;
    }

    this->msgCount = 0;
    this->connectTo = args[0];
    this->msgSize = args[1].toInt();
    this->maxMsgs = args[2].toInt();
    this->socket = NULL;
    this->msgQueued = NULL;
    this->watch = NULL;

    qInfo() << "Message size :" << this->msgSize;
    qInfo() << "Message count:" << this->maxMsgs;

    QTimer::singleShot(0, this, &App::started);
}

App::~App()
{
    if (this->socket != NULL) {
        delete this->socket;
        this->socket = NULL;
    }
}

void App::started()
{
    this->socket = QZmqSocket::create(ZMQ_PUSH);
    Q_ASSERT(this->socket != NULL);
    connect(this->socket, &QZmqSocket::onMessage, this, &App::onMessage);
    connect(this->socket, &QZmqSocket::onReadyToSend, this, &App::onReadyToSend);
    connect(this->socket, &QZmqSocket::onError, this, &App::onError);

    if(!this->socket->connect(this->connectTo.toStdString().c_str())) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qCritical() << "Cannot connect:" << QZmqError::getLastError() << "-" << errStr;
        App::exit(-1);
        return;
    }

    this->msgCount = 0;
    while(this->msgCount < this->maxMsgs) {
        QZmqMessage *msg = QZmqMessage::create(this->msgSize);
        if (!this->socket->send(msg)) {
            int error = QZmqError::getLastError();
            if (error != EAGAIN) {
                const char *errStr = QZmqError::getLastError(error);
                qCritical() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
                delete msg;
            } else {
                this->msgQueued = msg;
            }
            break;
        } else {
            delete msg;
            this->msgCount++;
        }
    }
}

void App::onMessage(QZmqSocket *socket, QZmqMessage *msg)
{
    // Worker thread should not receive messages.
}

void App::onReadyToSend(QZmqSocket *socket)
{
    while(this->msgCount < this->maxMsgs) {
        QZmqMessage *msg = NULL;
        if (this->msgQueued != NULL) {
            msg = this->msgQueued;
            this->msgQueued = NULL;
        } else {
            msg = QZmqMessage::create(this->msgSize);
        }
        if (!this->socket->send(msg)) {
            int error = QZmqError::getLastError();
            if (error != EAGAIN) {
                const char *errStr = QZmqError::getLastError(error);
                qCritical() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
                delete msg;
            } else {
                this->msgQueued = msg;
            }
            break;
        } else {
            delete msg;
            this->msgCount++;
        }
    }

    if (this->msgCount == this->maxMsgs) {
        App::exit();
    }
}

void App::onError(QZmqSocket *socket, int error)
{
    qCritical() << "Socket error:" << QZmqError::getLastError(error);
}

void customMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    //QByteArray localMsg = msg.toLocal8Bit();
    //const char* file = context.file ? context.file : "";
    //const char* function = context.function ? context.function : "";
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyyMMdd-hh:mm:ss.zzz");
    switch (type) {
        case QtDebugMsg:
            fprintf(stdout, "%s|DEBUG|%s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtInfoMsg:
            fprintf(stdout, "%s|INFO |%s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtWarningMsg:
            fprintf(stderr, "%s|WARN |%s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtCriticalMsg:
            fprintf(stderr, "%s|CRTCL|%s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtFatalMsg:
            fprintf(stderr, "%s|FATAL|%s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageOutput);
    App app(argc, argv);

    return app.exec();
}