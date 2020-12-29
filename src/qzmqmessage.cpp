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

#include "qzmqmessage.hpp"

QZMQ_BEGIN_NAMESPACE

QZmqMessage::QZmqMessage(QObject *parent) : QObject(parent)
{
    this->zMsg = NULL;
}

QZmqMessage::~QZmqMessage()
{
    if(this->zMsg != NULL) {
        int rc = zmq_msg_close(this->zMsg);
        Q_ASSERT(rc == 0);
        delete this->zMsg;
        this->zMsg = NULL;
    }
}

QZmqMessage* QZmqMessage::create(QObject *parent)
{
    zmq_msg_t *zMsg = new zmq_msg_t();
    int rc = zmq_msg_init(zMsg);
    if (rc != 0) {
        delete zMsg;
        return NULL;
    }

    QZmqMessage* qmsg = new QZmqMessage(parent);
    qmsg->zMsg = zMsg;
    return qmsg;
}

QZmqMessage* QZmqMessage::create(size_t size, QObject *parent)
{
    zmq_msg_t *zMsg = new zmq_msg_t();
    int rc = zmq_msg_init_size(zMsg, size);
    if (rc != 0) {
        delete zMsg;
        return NULL;
    }

    QZmqMessage* qmsg = new QZmqMessage(parent);
    qmsg->zMsg = zMsg;
    return qmsg;
}

QZmqMessage* QZmqMessage::create(zmq_msg_t *zMsg, QObject *parent)
{
    QZmqMessage* qmsg = new QZmqMessage(parent);
    qmsg->zMsg = zMsg;
    return qmsg;
}

void* QZmqMessage::data()
{
    return zmq_msg_data(this->zMsg);
}

bool QZmqMessage::copy(QZmqMessage* dst)
{
    Q_ASSERT(dst != NULL);
    int rc = zmq_msg_copy(dst->zMsg, this->zMsg);
    return rc == 0;
}

bool QZmqMessage::move(QZmqMessage* dst)
{
    Q_ASSERT(dst != NULL);
    int rc = zmq_msg_move(dst->zMsg, this->zMsg);
    return rc == 0;
}

bool QZmqMessage::more()
{
    return zmq_msg_more(this->zMsg) != 0;
}

size_t QZmqMessage::size()
{
    return zmq_msg_size(this->zMsg);
}

const char* QZmqMessage::gets(const char *property)
{
    return zmq_msg_gets(this->zMsg, property);
}

bool QZmqMessage::get(int property, int &value)
{
    value = zmq_msg_get(this->zMsg, property);
    return value != EINVAL;
}

bool QZmqMessage::set(int property, int value)
{
    int rc = zmq_msg_set(this->zMsg, property, value);
    return rc != EINVAL;
}

QZMQ_END_NAMESPACE