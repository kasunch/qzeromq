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
#include <zmq.h>

QZMQ_BEGIN_NAMESPACE

QZmqMessage::QZmqMessage(QObject *parent) : QObject(parent)
{
    this->msg = NULL;
}

QZmqMessage::~QZmqMessage()
{
    if(this->msg != NULL) {
        int rc = zmq_msg_close(this->msg);
        Q_ASSERT(rc == 0);
        delete this->msg;
        this->msg = NULL;
    }
}

QZmqMessage* QZmqMessage::create(QObject *parent)
{
    zmq_msg_t *msg = new zmq_msg_t();
    int rc = zmq_msg_init(msg);
    if (rc != 0) {
        delete msg;
        return NULL;
    }

    QZmqMessage* qmsg = new QZmqMessage(parent);
    qmsg->msg = msg;
    return qmsg;
}

QZmqMessage* QZmqMessage::create(size_t size, QObject *parent)
{
    zmq_msg_t *msg = new zmq_msg_t();
    int rc = zmq_msg_init_size(msg, size);
    if (rc != 0) {
        delete msg;
        return NULL;
    }

    QZmqMessage* qmsg = new QZmqMessage(parent);
    qmsg->msg = msg;
    return qmsg;
}

QZmqMessage* QZmqMessage::create(zmq_msg_t *msg, QObject *parent)
{
    QZmqMessage* qmsg = new QZmqMessage(parent);
    qmsg->msg = msg;
    return qmsg;
}

void* QZmqMessage::data()
{
    return zmq_msg_data(this->msg);
}

bool QZmqMessage::copy(QZmqMessage* dst)
{
    Q_ASSERT(dst != NULL);
    int rc = zmq_msg_copy(dst->msg, this->msg);
    return rc == 0;
}

bool QZmqMessage::move(QZmqMessage* dst)
{
    Q_ASSERT(dst != NULL);
    int rc = zmq_msg_move(dst->msg, this->msg);
    return rc == 0;
}

bool QZmqMessage::more()
{
    return zmq_msg_more(this->msg) != 0;
}

size_t QZmqMessage::size()
{
    return zmq_msg_size(this->msg);
}

const char* QZmqMessage::gets(const char *property)
{
    return zmq_msg_gets(this->msg, property);
}

bool QZmqMessage::get(int property, int &value)
{
    value = zmq_msg_get(this->msg, property);
    return value != EINVAL;
}

bool QZmqMessage::set(int property, int value)
{
    int rc = zmq_msg_set(this->msg, property, value);
    return rc != EINVAL;
}

zmq_msg_t* QZmqMessage::zmqMsg()
{
    return this->msg;
}

QZMQ_END_NAMESPACE