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

#ifndef __QZMQ_EVENTLOOP_HOOK_H__
#define __QZMQ_EVENTLOOP_HOOK_H__

#include "qzmqcommon.hpp"
#include <QObject>

QZMQ_BEGIN_NAMESPACE

template <typename T> class QVector;
template <typename T> class QThreadStorage;
class zmq_pollitem_t;
class QZmqSocket;

class QZMQ_LOCAL QZmqEventLoopHook : public QObject
{
    Q_OBJECT
public:
    virtual ~QZmqEventLoopHook();
    static QZmqEventLoopHook* instance();
    void attach(QZmqSocket *socket);
    void detach(QZmqSocket *socket);

private slots:
    void onAboutToBlock();
    void onAwake();
    void onTimer();

protected:
    QZmqEventLoopHook(QObject *parent=nullptr);
    Q_DISABLE_COPY(QZmqEventLoopHook);

    bool eventPending;
    QVector<zmq_pollitem_t> pollItems;
    QVector<QZmqSocket*> sockets;
    static QThreadStorage <QZmqEventLoopHook*> localInstance;
};

QZMQ_END_NAMESPACE

#endif // __QZMQ_EVENTLOOP_HOOK_H__