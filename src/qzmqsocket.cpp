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
#include "qzmqerror.hpp"
#include <QSocketNotifier>
#include <QMetaMethod>
#include <QAbstractEventDispatcher>
#include <QTimer>

QZMQ_BEGIN_NAMESPACE

constexpr int DEFAULT_MAX_THROUGHPUT = 1000;

/**
 * @brief   Construct a new QZmqSocket::QZmqSocket object
 * 
 * @param parent Parent object of the socket.
 */
QZmqSocket::QZmqSocket(QObject* parent) : QObject(parent)
{
    this->socket = NULL;
    this->readNotifier = NULL;
    this->writeNotifier = NULL;
    this->wakeUpTimer = NULL;
    this->maxThroughput = DEFAULT_MAX_THROUGHPUT;
}

/**
 * @brief   Destroy the QZmqSocket::QZmqSocket object
 *          Use delete to destory/close the socket and free the used resources.
 */
QZmqSocket::~QZmqSocket()
{
    if (this->wakeUpTimer != NULL) {
        delete this->wakeUpTimer;
        this->wakeUpTimer = NULL;
    }

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

/**
 * @brief   Create 0MQ socket.
 * 
 * @param type      Refer to the documentation of zmq_socket().
 * @param parent    Parent object of the created socket object.
 * @return QZmqSocket*  A pointer to the created socket. 
 *                      NULL is returned if the socket creation is failed.
 *                      Use QZmqError::getLastError() to get the error code.
 */
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

    qsocket->wakeUpTimer = new QTimer(qsocket);
    qsocket->wakeUpTimer->setSingleShot(true);
    QObject::connect(qsocket->wakeUpTimer, &QTimer::timeout, qsocket, &QZmqSocket::onWakeUpTimer);

    auto dispatcher = QAbstractEventDispatcher::instance(nullptr); 
    Q_ASSERT(dispatcher != NULL);
    QObject::connect(dispatcher, &QAbstractEventDispatcher::aboutToBlock, qsocket, &QZmqSocket::onAboutToBlock);
    QObject::connect(dispatcher, &QAbstractEventDispatcher::awake, qsocket, &QZmqSocket::onAwake);

    return qsocket;
}

/**
 * @brief   Slot for  activated signal of the read socket notifier.
 * 
 * @param socket Associated file descriptor of the socket
 */
void QZmqSocket::readActivated(int socket)
{
    receiveAll();
}

/**
 * @brief   Slot for  activated signal of the write socket notifier.
 * 
 * @param socket Associated file descriptor of the socket
 */
void QZmqSocket::writeActivated(int socket)
{
    checkReadyToSend();
}

/**
 * @brief   This is the slot (function) for the signal that is emitted just before the
 *          event dispatcher is blocked. No messages are read in this function.
 */
void QZmqSocket::onAboutToBlock()
{
    bool eventPending = false;
    int events = this->events();
    if (events & ZMQ_POLLIN) {
        eventPending = true;
    }

    if (this->writeNotifier->isEnabled() && (events & ZMQ_POLLOUT)) {
        eventPending = true;
    }

    if (eventPending) {
        // There is activity in the socket. 
        // Schedule a single shot timer just to wakeup the event dispatcher.
        this->wakeUpTimer->start(0);
    }
}

/**
 * @brief   This is the slot (function) for the signal that is emitted just after the
 *          event dispatcher is awaken. Messages are read and ready send signal emitted if needed.
 */
void QZmqSocket::onAwake()
{
    receiveAll();
    checkReadyToSend();
}

/**
 * @brief   This is just a dummy handler for the event loop wake-up timer.
 * 
 */
void QZmqSocket::onWakeUpTimer()
{
    // Nothing to be done here.
}

/**
 * @brief   Get ØMQ socket options. 
 *          Refer to the documentation of zmq_getsockopt().
 * 
 * @param option    Refer to the documentation of zmq_getsockopt().
 * @param value     Refer to the documentation of zmq_getsockopt().
 * @param len       Refer to the documentation of zmq_getsockopt().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful. 
 *                  Use QZmqError::getLastError() to get the error code.
 */
bool QZmqSocket::getOption(int option, void *value, size_t *len)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_getsockopt(this->socket, option, value, len);
    if (rc != 0) {
        return false;
    }
    return true;
}

/**
 * @brief   Set ØMQ socket options. 
 *          Refer to the documentation of zmq_setsockopt().
 * 
 * @param option    Refer to the documentation of zmq_setsockopt().
 * @param value     Refer to the documentation of zmq_setsockopt().
 * @param len       Refer to the documentation of zmq_setsockopt().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful. 
 *                  Use QZmqError::getLastError() to get the error code.
 */
bool QZmqSocket::setOption(int option, const void *value, size_t len)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_setsockopt(this->socket, option, value, len);
    if (rc != 0) {
        return false;
    }
    return true;
}

/**
 * @brief   Accept incoming connections on a socket.
 *          Refer to the documentation of zmq_bind().
 * 
 * @param address   Refer to the documentation of zmq_bind().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful.
 *                  Use QZmqError::getLastError() to get the error code.
 */
bool QZmqSocket::bind(const char *address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_bind(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

/**
 * @brief   Stop accepting connections on a socket.
 *          Refer to the documentation of zmq_unbind().

 * @param address   Refer to the documentation of zmq_unbind().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful.
 *                  Use QZmqError::getLastError() to get the error code.
 */
bool QZmqSocket::unbind(const char* address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_unbind(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

/**
 * @brief   Create outgoing connection from socket.
 *          Refer to the documentation of zmq_connect().
 * 
 * @param address   Refer to the documentation of zmq_connect().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful.
 *                  Use QZmqError::getLastError() to get the error code.
 */
bool QZmqSocket::connect(const char* address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_connect(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

/**
 * @brief   Disconnect a socket.
 *          Refer to the documentation of zmq_disconnect().
 * 
 * @param address   Refer to the documentation of zmq_disconnect().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful.
 *                  Use QZmqError::getLastError() to get the error code.
 */
bool QZmqSocket::disconnect(const char* address)
{
    Q_ASSERT(this->socket != NULL);

    int rc = zmq_disconnect(this->socket, address);
    if (rc != 0) {
        return false;
    }
    return true;
}

/**
 * @brief   Retrieve socket event state.
 *          Operation of this function is similar to QZmqSocket::getOption() with 
 *          ZMQ_EVENTS set as the option.
 * 
 * @return int  Socket event state.
 *              Refer to the documentation of zmq_getsockopt() with ZMQ_EVENTS as the option.
 */
int QZmqSocket::events()
{
    int events = 0;
    size_t events_size = sizeof(events);
    getOption(ZMQ_EVENTS, &events, &events_size);
    return events;
}

/**
 * @brief   Receive a message from the socket.
 * 
 * @param msg   A pointer to a valid message object that stores the incoming message.
 *              @sa QZmqMessage.
 * @param flags Refer to the documentation of zmq_msg_recv().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful.
 *                  Use QZmqError::getLastError() to get the error code.
 */
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

/**
 * @brief   Receive all available messages from the socket and emit onMessage() signal.
 *          The maximum number of massages received in one call to this function is limited
 *          by QZmqSocket::maxThroughput.
 *          @sa QZmqSocket::setMaximumThroughput()
 */
void QZmqSocket::receiveAll()
{
    int i = 0;
    while ((events() & ZMQ_POLLIN) && i < this->maxThroughput) {
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
        i++;
    }
}

/**
 * @brief   Send a message through the socket.
 *          If the massage cannot be sent at the moment, onReadyToSend() signal will be emitted 
 *          later when the socket is ready to send messages.
 *          @sa QZmqSocket::onReadyToSend()
 *          @note This function does not deallocate the given message.
 * 
 * @param msg   A pointer to the massages to be sent.
 * @param flags Refer to the documentation of zmq_msg_send().
 * @return true     If the operation is successful.
 * @return false    If the operation is not successful.
 *                  Use QZmqError::getLastError() to get the error code.
 */
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
    } else {
        // For the moment, we can still send data over the socket.
        // So, we do not need to worry about the ready-to-send event.
        this->writeNotifier->setEnabled(false);
    }

    return true;
}

/**
 * @brief   Check if the socket is ready to send massages.
 *          If the socket is ready, QZmqSocket::onReadyToSend() signal will be emitted.
 */
void QZmqSocket::checkReadyToSend()
{
    if (this->writeNotifier->isEnabled()) {
        if (events() & ZMQ_POLLOUT) {
            this->writeNotifier->setEnabled(false);
            emit onReadyToSend(this);
        }
    }
}

/**
 * @brief   See if the last received message has more parts to be received. 
 * 
 * @return true     If the last received message has more parts.
 * @return false    If the last received message does not have more parts.
 */
bool QZmqSocket::hasMoreParts()
{    
    int value = 0;
    size_t size = sizeof(value);
    getOption(ZMQ_RCVMORE, &value, &size);
    return value == 1;
}

/**
 * @brief   Returns the underlying raw socket. 
 * 
 * @return void*    A pointer to the raw socket
 */
void* QZmqSocket::zmqSocket()
{
    return this->socket;
}

/**
 * @brief   Returns the maximum number of massages to be received through
 *          QZmqSocket::onMessage() signal at a time.
 *          @sa QZmqSocket::setMaximumThroughput()
 * 
 * @return int  Maximum number of messages emitted.
 */
int QZmqSocket::maximumThroughput()
{
    return this->maxThroughput;
}

/**
 * @brief   Set the maximum number of massages to be received through
 *          QZmqSocket::onMessage() signal at a time.
 *          Lowring the maximum number of massages increases the responsiveness of the
 *          event loop.
 *          @sa QZmqSocket::maximumThroughput()
 * 
 * @param throughput    Maximum number of messages to be emitted.
 */
void QZmqSocket::setMaximumThroughput(int throughput)
{
    this->maxThroughput = throughput;
}

QZMQ_END_NAMESPACE