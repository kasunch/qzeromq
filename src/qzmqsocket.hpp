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

#ifndef __QZMQ_SOCKET_H__
#define __QZMQ_SOCKET_H__

#include "qzmqcommon.hpp"
#include <zmq.h>
#include <QObject>

QZMQ_BEGIN_NAMESPACE

class QSocketNotifier;
class QZmqMessage;
class QZMQ_API QZmqSocket : public QObject
{
    Q_OBJECT
public:
    static QZmqSocket* create(int type, QObject* parent=nullptr);
    virtual ~QZmqSocket();
    bool setOption(int option, const void *value, size_t len);
    bool getOption(int option, void *value, size_t *len);
    bool bind(const char *address);
    bool unbind(const char *address);
    bool connect(const char *address);
    bool disconnect(const char *address);
    bool send(QZmqMessage *msg, int flags=ZMQ_DONTWAIT);
    bool receive(QZmqMessage *msg, int flags=ZMQ_DONTWAIT);
    bool hasMoreParts();
    void* zmqSocket();

signals:
    void onMessage(QZmqSocket *socket, QZmqMessage *msg);
    void onReadyToSend(QZmqSocket *socket);
    void onError(QZmqSocket *socket, int error);

protected slots:
    void readActivated(int socket);
    void writeActivated(int socket);

protected:
    friend class QZmqEventLoopHook;
    QZmqSocket(QObject* parent=nullptr);
    int events();
    void receiveAll();
    void checkReadyToSend();

    void *socket;
    QSocketNotifier *readNotifier;
    QSocketNotifier *writeNotifier;
};

QZMQ_END_NAMESPACE

#endif // __QT_ZMQ_SOCKET_H__