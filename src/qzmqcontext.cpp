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

#include "qzmqcontext.hpp"
#include <zmq.h>

QZMQ_BEGIN_NAMESPACE

QZmqContext::QZmqContext()
{
    this->context = zmq_ctx_new();
    Q_ASSERT(this->context != NULL);
}

QZmqContext::~QZmqContext()
{
    if (this->context != NULL) {
        int rc = zmq_ctx_destroy(this->context);
        Q_ASSERT(rc == 0);
        this->context = NULL;
    }
}

bool QZmqContext::setOption(int option, int value)
{
    Q_ASSERT(this->context != NULL);
    int rc = zmq_ctx_set(this->context, option, value);
    return rc == 0;
}

bool QZmqContext::getOption(int option, int &value)
{
   Q_ASSERT(this->context != NULL);
    value = zmq_ctx_get(this->context, option);
    return value >= 0;
}

QZmqContext* QZmqContext::instance()
{
    // ZMQ documentation says:
    // A ØMQ context is thread safe and may be shared among as many application threads 
    // as necessary, without any additional locking required on the part of the caller.
    //
    // So we use a singleton for the context.
    static QZmqContext instance;
    return &instance;
}

QZMQ_END_NAMESPACE