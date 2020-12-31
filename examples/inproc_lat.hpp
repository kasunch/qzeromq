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

#ifndef __INPROC_LAT_H__
#define __INPROC_LAT_H__

#include <QCoreApplication>
#include <QThread>

class QZmqSocket;
class QZmqMessage;

class WorkerThread : public QThread
{
    Q_OBJECT
public:
    WorkerThread(uint32_t msg_size, QObject *parent=nullptr);
    virtual ~WorkerThread();

private slots:
    void onMessage(QZmqSocket *socket, QZmqMessage *msg);
    void onReadyToSend(QZmqSocket *socket);
    void onError(QZmqSocket *socket, int error);
    void started();

private:
    QZmqSocket* socket;
    uint32_t msgSize;
};

class App : public QCoreApplication
{
    Q_OBJECT
public:
    App(int &argc, char **argv);
    virtual ~App();

private slots:
    void onMessage(QZmqSocket *socket, QZmqMessage *msg);
    void onReadyToSend(QZmqSocket *socket);
    void onError(QZmqSocket *socket, int error);
    void started();

private:
    QZmqSocket* socket;
    QZmqMessage* msgQueued;
    WorkerThread* worker;
    int msgCount;
    int msgSize;
    int maxMsgs;
    bool dontExit;
    void* watch;
};

#endif // __INPROC_LAT_H__