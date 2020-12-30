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

#ifndef __QZMQ_MESSAGE_H__
#define __QZMQ_MESSAGE_H__

#include "qzmqcommon.hpp"
#include <QObject>

QZMQ_BEGIN_NAMESPACE

class zmq_msg_t;
class QZMQ_API QZmqMessage : public QObject
{
public:
    static QZmqMessage* create(QObject *parent=nullptr);
    static QZmqMessage* create(size_t size, QObject *parent=nullptr);
    static QZmqMessage* create(zmq_msg_t *msg, QObject *parent=nullptr);
    virtual ~QZmqMessage();
    void* data() ;
    bool copy(QZmqMessage *dst);
    bool move(QZmqMessage *dst);
    bool more();
    size_t size();
    const char* gets(const char *property);
    bool get(int property, int &value);
    bool set(int property, int value);
    zmq_msg_t* msg();
protected:
    friend class QZmqSocket;
    QZmqMessage(QObject *parent=nullptr);
    zmq_msg_t *zMsg;
};

QZMQ_END_NAMESPACE

#endif // __QZMQ_MESSAGE_H__