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
#include <cstdint>
#include <cstdio>
#include <QDebug>
#include <QDateTime>
#include <QCommandLineParser>

InProcLatApp::InProcLatApp(int &argc, char **argv) : QCoreApplication(argc, argv)
{

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption dontExitOpt(QStringList() << "d" << "dont-exit", "Don't exit.");
    parser.addOption(dontExitOpt);
    parser.addPositionalArgument("size", "message size");
    parser.addPositionalArgument("count", "roundtrip count");
    parser.process(*this);

    const QStringList args = parser.positionalArguments();

    if (args.length() != 2) {
        parser.showHelp(-1);
        return;
    }

    msg_cnt = 0;
    msg_size = args[0].toInt();
    max_msgs = args[1].toInt();
    dontExit = parser.isSet(dontExitOpt) ? true : false;

    socket = QZmqSocket::create(ZMQ_REQ);
    Q_ASSERT(socket != NULL);
    connect(socket, &QZmqSocket::onMessage, this, &InProcLatApp::onMessage);
    connect(socket, &QZmqSocket::onError, this, &InProcLatApp::onError);

    if (!socket->bind("inproc://lat_test")) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qCritical() << "Binding failed:" << error << "-" << errStr;
        InProcLatApp::exit(-1);
        return;
    }

    worker = new WorkerThread(msg_size,this);
    worker->start();
    QTimer::singleShot(10, this, &InProcLatApp::started); 
}

InProcLatApp::~InProcLatApp()
{
    if (worker != NULL) {
        delete worker;
    }

    if (socket != NULL) {
        delete socket;
    }
}

void InProcLatApp::onMessage(QZmqSocket *socket, QZmqMessage *msg)
{
    if (msg_cnt < max_msgs) {
        if (msg->size() != msg_size) {
            qCritical() << "Message of incorrect size received";
            worker->quit();
            worker->wait();
            InProcLatApp::exit(-1);
            return;
        }

        if (!socket->send(msg)) {
            int error = QZmqError::getLastError();
            const char *errStr = QZmqError::getLastError(error);
            qWarning() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
        }
        msg_cnt++;
    } else {
        uint64_t elapsed = zmq_stopwatch_stop(watch);
        double latency = (double)elapsed / (msg_cnt * 2);
        qInfo() << "Sending stopped";
        qInfo() << "Average latency:" << latency << "us";
        delete msg;
        worker->quit();
        worker->wait();
        if (!dontExit) {
            InProcLatApp::quit();
        }
    }
}

void InProcLatApp::onError(QZmqSocket *socket, int error)
{
    qCritical() << "Socket error:" << QZmqError::getLastError(error);
}


void InProcLatApp::started()
{
    qInfo() << "Start sending";
    watch = zmq_stopwatch_start();
    QZmqMessage *msg = QZmqMessage::create(msg_size);
    if (!socket->send(msg)) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qWarning() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
    } 
}

WorkerThread::WorkerThread(uint32_t msg_size, QObject *parent) : QThread(parent)
{
    this->msg_size = msg_size;
    socket = NULL;
    connect(this, &QThread::started, this, &WorkerThread::started);
}

WorkerThread::~WorkerThread()
{
    if (socket != NULL) {
        delete socket;
    }
}

void WorkerThread::onMessage(QZmqSocket *socket, QZmqMessage *msg)
{
    if (!socket->send(msg)) {
        int error = QZmqError::getLastError();
        const char *errStr = QZmqError::getLastError(error);
        qWarning() << "Sending failed:" << QZmqError::getLastError() << "-" << errStr;
    } 
}

void WorkerThread::onError(QZmqSocket* socket, int error)
{
    qCritical() << "Socket error:" << QZmqError::getLastError(error);
}

void WorkerThread::started()
{
    socket = QZmqSocket::create(ZMQ_REP);
    connect(socket, &QZmqSocket::onMessage, this, &WorkerThread::onMessage);
    connect(socket, &QZmqSocket::onError, this, &WorkerThread::onError);
    socket->connect("inproc://lat_test"); 
}


void customMessageOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    const char* file = context.file ? context.file : "";
    const char* function = context.function ? context.function : "";
    QString dateTimeStr = QDateTime::currentDateTime().toString("yyyyMMdd-hh:mm:ss.zzz");
    switch (type) {
        case QtDebugMsg:
            fprintf(stdout, "%s|DEBUG| %s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtInfoMsg:
            fprintf(stdout, "%s|INFO | %s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtWarningMsg:
            fprintf(stderr, "%s|WARN | %s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtCriticalMsg:
            fprintf(stderr, "%s|CRTCL| %s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
        case QtFatalMsg:
            fprintf(stderr, "%s|FATAL| %s\n", dateTimeStr.toStdString().c_str(), msg.toStdString().c_str());
            break;
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(customMessageOutput);
    InProcLatApp app(argc, argv);

    return app.exec();
}