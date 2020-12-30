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

#include "qzmqeventloophook.hpp"
#include "qzmqsocket.hpp"
#include <QVector>
#include <QThreadStorage>
#include <QTimer>
#include <QSocketNotifier>
#include <QAbstractEventDispatcher>

QZMQ_BEGIN_NAMESPACE

QThreadStorage <QZmqEventLoopHook*> QZmqEventLoopHook::localInstance;

QZmqEventLoopHook::QZmqEventLoopHook(QObject *parent) : QObject(parent)
{

}

QZmqEventLoopHook::~QZmqEventLoopHook()
{
    this->pollItems.clear();
    this->sockets.clear();
}

QZmqEventLoopHook* QZmqEventLoopHook::instance()
{
    if (!QZmqEventLoopHook::localInstance.hasLocalData()) {
        // The destructor will be called when the QApplication is destroyed
        // Refer QThreadStorage documentation for more details.
        QZmqEventLoopHook *hook = new QZmqEventLoopHook();
        // Get current disptacher of the current thread.
        auto dispatcher = QAbstractEventDispatcher::instance(nullptr); 
        Q_ASSERT(dispatcher != NULL);
        QObject::connect(dispatcher, &QAbstractEventDispatcher::aboutToBlock, hook, &QZmqEventLoopHook::onAboutToBlock);
        QObject::connect(dispatcher, &QAbstractEventDispatcher::awake, hook, &QZmqEventLoopHook::onAwake);
        hook->eventPending = false;
        QZmqEventLoopHook::localInstance.setLocalData(hook);
    }

    return QZmqEventLoopHook::localInstance.localData();
}

void QZmqEventLoopHook::attach(QZmqSocket *socket)
{
    Q_ASSERT(socket != NULL);
    zmq_pollitem_t pollItem;
    pollItem.socket = socket->socket;
    pollItem.events = ZMQ_POLLIN;
    this->pollItems.append(pollItem);
    this->sockets.append(socket);
}

void QZmqEventLoopHook::detach(QZmqSocket *socket)
{
    Q_ASSERT(socket != NULL);
    
    int index = this->sockets.indexOf(socket);
    if (index < 0) {
        return;
    }
    this->pollItems.remove(index);
    this->sockets.remove(index);
}

void QZmqEventLoopHook::onAboutToBlock()
{
    // Check all sockets and set appropriate polling flags.
    auto it = this->sockets.begin();
    auto it_poll = this->pollItems.begin();
    for ( ; it != this->sockets.end() && it_poll != this->pollItems.end(); it++, it_poll++) {
        (*it_poll).events = ZMQ_POLLIN;
        if ((*it)->writeNotifier->isEnabled()) {
            // Only set this if write notifier is already enabled.
            // Otherwise, this event loop will run without blocking resulting high CPU usage. 
            (*it_poll).events |= ZMQ_POLLOUT;
        }
    }

    int count = zmq_poll(this->pollItems.data(), this->pollItems.size(), 0);
    if (count > 0) {
        // There is activity in some sockets. 
        // So schedule a single shot timer with zero timeout to make Qt's event dispatcher exit
        // from the blocking state immediately. 
        // Then read from the sockets when the event dispatcher is awake i.e. in the onAwake() slot.
        this->eventPending = true;
        QTimer::singleShot(0, this, &QZmqEventLoopHook::onTimer);
    } else {
        // No socket activity yet.
        // Allow the QSocketNotifiers that are associated with the ZMQ sockets to monitor during the
        // event dispatcher's blocking state.
        this->eventPending = false; 
    }
}

void QZmqEventLoopHook::onAwake()
{
    if (this->eventPending) {
        for (auto it = this->sockets.begin(); it != this->sockets.end(); it++) {
            (*it)->receiveAll();
            (*it)->checkReadyToSend();
        }
        this->eventPending = false;
    }
}

void QZmqEventLoopHook::onTimer()
{
    // Just a dummy event handler
}

QZMQ_END_NAMESPACE