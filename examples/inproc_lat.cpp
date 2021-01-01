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

#include "inproc_lat.hpp"
#include <qzmq.hpp>
#include <zmq.h>
#include <cstdio>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QCommandLineParser>

App::App(int &argc, char **argv) : QCoreApplication(argc, argv)
{
    QCommandLineParser parser;
    parser.addHelpOption();
    parser.addPositionalArgument("size", "message size");
    parser.addPositionalArgument("count", "roundtrip count");
    parser.process(*this);

    const QStringList args = parser.positionalArguments();
    if (args.length() != 2) {
        parser.showHelp(-1);
        return;
    }

    this->msgCount = 0;
    this->msgSize = args[0].toInt();
    this->maxMsgs = args[1].toInt();
    this->socket = NULL;
    this->msgQueued = NULL;
    this->watch = NULL;

    qInfo() << "Message size :" << this->msgSize;
    qInfo() << "Message count:" << this->maxMsgs;

    this->worker = new WorkerThread(this->msgSize, this);
    this->worker->start();

    QTimer::singleShot(0, this, &App::started); 
}

App::~App()
{
    if (this->worker != NULL) {
        delete this->worker;
        this->worker = NULL;
    }

    if (this->socket != NULL) {
        delete this->socket;
        this->socket = NULL;
    }
}

void App::onMessage(QZmqSocket *socket, QZmqMessage *msg)
{
    if (msg->size() != this->msgSize) {
        delete msg;
        qCritical() << "Message of incorrect size received";
        this->worker->quit();
        this->worker->wait();
        App::exit(-1);
        return;
    }

    this->msgCount++;
    if (this->msgCount < this->maxMsgs) {
        if (!this->socket->send(msg)) {
            int error = QZmqError::getLastError();
            const char *errStr = QZmqError::getLastError(error);
            qCritical() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
            this->worker->quit();
            this->worker->wait();
            App::exit(-1);
            return;
        }
    } else {
        uint64_t elapsed = zmq_stopwatch_stop(this->watch);
        double latency = (double)elapsed / (this->msgCount * 2);
        qInfo() << "Average latency:" << latency << "us";
        delete msg;
        this->worker->quit();
        this->worker->wait();
        App::exit();
    }
}

void App::onReadyToSend(QZmqSocket *socket)
{
    if (this->msgQueued != NULL) {
        if (!this->socket->send(this->msgQueued)) {
            int error = QZmqError::getLastError();
            const char *errStr = QZmqError::getLastError(error);
            qCritical() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
            this->worker->quit();
            this->worker->wait();
            App::exit(-1);
            return;
        }
        this->msgQueued = NULL;
    }
}

void App::onError(QZmqSocket *socket, int error)
{
    qCritical() << "Socket error:" << QZmqError::getLastError(error);
}

void App::started()
{
    this->socket = QZmqSocket::create(ZMQ_REQ);
    Q_ASSERT(this->socket != NULL);
    connect(this->socket, &QZmqSocket::onMessage, this, &App::onMessage);
    connect(this->socket, &QZmqSocket::onReadyToSend, this, &App::onReadyToSend);
    connect(this->socket, &QZmqSocket::onError, this, &App::onError);

    if (!this->socket->bind("inproc://lat_test")) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qCritical() << "Binding failed:" << error << "-" << errStr;
        App::exit(-1);
        return;
    }

    this->watch = zmq_stopwatch_start();

    QZmqMessage *msg = QZmqMessage::create(this->msgSize);
    if (!this->socket->send(msg)) {
        int error = QZmqError::getLastError();
        if (error != EAGAIN) {
            const char *errStr = QZmqError::getLastError(error);
            qCritical() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
            delete msg;
            this->worker->quit();
            this->worker->wait();
            App::exit(-1);
            return;
        } else {
            this->msgQueued = msg;
        }
    } 
}

WorkerThread::WorkerThread(uint32_t msgSize, QObject *parent) : QThread(parent)
{
    this->msgSize = msgSize;
    socket = NULL;
    connect(this, &QThread::started, this, &WorkerThread::started);
}

WorkerThread::~WorkerThread()
{
    if (this->socket != NULL) {
        delete this->socket;
        this->socket = NULL;
    }
}

void WorkerThread::onMessage(QZmqSocket *socket, QZmqMessage *msg)
{
    if (msg->size() != this->msgSize) {
        qCritical() << "Message of incorrect size received";
        return;
    }

    if (!this->socket->send(msg)) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qCritical() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
    } 
}

void WorkerThread::onReadyToSend(QZmqSocket *socket)
{

}

void WorkerThread::onError(QZmqSocket* socket, int error)
{
    qCritical() << "Socket error:" << QZmqError::getLastError(error);
}

void WorkerThread::started()
{
    this->socket = QZmqSocket::create(ZMQ_REP);
    Q_ASSERT(this->socket != NULL);
    connect(this->socket, &QZmqSocket::onMessage, this, &WorkerThread::onMessage);
    connect(this->socket, &QZmqSocket::onReadyToSend, this, &WorkerThread::onReadyToSend);
    connect(this->socket, &QZmqSocket::onError, this, &WorkerThread::onError);

    if(!this->socket->connect("inproc://lat_test")) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qCritical() << "Cannot connect:" << QZmqError::getLastError() << "-" << errStr;
    } 
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