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

#include <cstdint>
#include <QTimer>
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
    uint32_t msg_size;
};

class InProcLatApp : public QCoreApplication
{
    Q_OBJECT
public:
    InProcLatApp(int &argc, char **argv);
    virtual ~InProcLatApp();

private slots:
    void onMessage(QZmqSocket *socket, QZmqMessage *msg);
    void onReadyToSend(QZmqSocket *socket);
    void onError(QZmqSocket *socket, int error);
    void started();

private:
    QZmqSocket* socket;
    QZmqMessage* msgQueued;
    WorkerThread* worker;
    uint32_t msg_cnt;
    uint32_t msg_size;
    uint32_t max_msgs;
    bool dontExit;
    void* watch;
};

#endif // __INPROC_LAT_H__