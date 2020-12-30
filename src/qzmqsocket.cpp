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

#include "qzmqsocket.hpp"
#include "qzmqmessage.hpp"
#include "qzmqcontext.hpp"
#include "qzmqeventloophook.hpp"
#include "qzmqerror.hpp"
#include <QSocketNotifier>
#include <QMetaMethod>

QZMQ_BEGIN_NAMESPACE

QZmqSocket::QZmqSocket(QObject* parent) : QObject(parent)
{
    this->socket = NULL;
    this->readNotifier = NULL;
    this->writeNotifier = NULL;
}

QZmqSocket::~QZmqSocket()
{
    QZmqEventLoopHook::instance()->detach(this);

    if (this->readNotifier != NULL) {
        this->readNotifier->setEnabled(false);
        delete this->readNotifier;
        this->readNotifier = NULL;            
    }

    if (this->writeNotifier != NULL) {
        this->writeNotifier->setEnabled(false);
        delete this->writeNotifier;
        this->writeNotifier = NULL;            
    }

    if (this->socket != NULL) {
        int rc = zmq_close(this->socket);
        Q_ASSERT(rc == 0);
        this->socket = NULL;
    }
}

QZmqSocket* QZmqSocket::create(int type, QObject* parent)
{
    QZmqContext *context = QZmqContext::instance();
    void *socket = zmq_socket(context->context, type);
    if (socket == NULL) {
        return NULL;
    }

    qintptr fd;
    size_t fd_size = sizeof(fd);
    int rc = zmq_getsockopt(socket, ZMQ_FD, &fd, &fd_size);
    if (rc != 0) {
        return NULL;
    }

    QZmqSocket* qsocket = new QZmqSocket(parent);
    qsocket->socket = socket;
    qsocket->readNotifier = new QSocketNotifier(fd, QSocketNotifier::Read, qsocket);
    QObject::connect(qsocket->readNotifier, &QSocketNotifier::activated, qsocket, &QZmqSocket::readActivated);
    qsocket->readNotifier->setEnabled(true);
    qsocket->writeNotifier = new QSocketNotifier(fd, QSocketNotifier::Write, qsocket);
    QObject::connect(qsocket->writeNotifier, &QSocketNotifier::activated, qsocket, &QZmqSocket::writeActivated);
    qsocket->writeNotifier->setEnabled(false);
    QZmqEventLoopHook::instance()->attach(qsocket);

    return qsocket;
}

void QZmqSocket::readActivated(int socket)
{
    receiveAll();
}

void QZmqSocket::writeActivated(int socket)
{
    checkReadyToSend();
}

bool QZmqSocket::getOption(int option, void *value, size_t *len)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_getsockopt(this->socket, option, value, len);
    if (rc != 0) {
        return false;
    }
    return true;
}

bool QZmqSocket::setOption(int option, const void *value, size_t len)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_setsockopt(this->socket, option, value, len);
    if (rc != 0) {
        return false;
    }
    return true;
}

bool QZmqSocket::bind(const char *address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_bind(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

bool QZmqSocket::unbind(const char* address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_unbind(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

bool QZmqSocket::connect(const char* address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_connect(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

bool QZmqSocket::disconnect(const char* address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_disconnect(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

int QZmqSocket::events()
{
    int events = 0;
    size_t events_size = sizeof(events);
    getOption(ZMQ_EVENTS, &events, &events_size);
    return events;
}

bool QZmqSocket::receive(QZmqMessage *msg, int flags)
{
    Q_ASSERT(msg != NULL);
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_msg_recv(msg->msg, this->socket, flags);
    if (rc < 0) {
        return false;
    }
    return true;
}

void QZmqSocket::receiveAll()
{
    this->readNotifier->setEnabled(false);
    while (events() & ZMQ_POLLIN) {
        QZmqMessage *msg = QZmqMessage::create(this);
        if (receive(msg)) {
            static const QMetaMethod signal = QMetaMethod::fromSignal(&QZmqSocket::onMessage);
            if (QObject::isSignalConnected(signal)) {
                emit onMessage(this, msg);
            } else {
                delete msg; 
            }
        } else {
            emit onError(this, QZmqError::getLastError());
            delete msg;
        }
    }
    this->readNotifier->setEnabled(true);
}

bool QZmqSocket::send(QZmqMessage *msg, int flags)
{
    Q_ASSERT(msg != NULL);
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_msg_send(msg->msg, this->socket, flags);
    if (rc < 0) {
        if (QZmqError::getLastError() == EAGAIN) {
            // Non-blocking mode was requested and the message cannot be sent at the moment.
            // Enable the write notifier to get notification when the socket is ready to send again.
            this->writeNotifier->setEnabled(true);
        }
        return false;
    } 

    delete msg;
    return true;
}

void QZmqSocket::checkReadyToSend()
{
    if (events() & ZMQ_POLLOUT) {
        this->writeNotifier->setEnabled(false);
        emit onReadyToSend(this);
    }
}

bool QZmqSocket::hasMoreParts()
{
    Q_ASSERT(this->socket != NULL);
    
    int value = 0;
    size_t size = sizeof(value);
    getOption(ZMQ_RCVMORE, &value, &size);
    return value == 1;
}

void* QZmqSocket::zmqSocket()
{
    return this->socket;
}

QZMQ_END_NAMESPACE