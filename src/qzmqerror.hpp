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

#ifndef __QZMQ_ERROR_H__
#define __QZMQ_ERROR_H__

#include "qzmqcommon.hpp"
#include <QObject>

QZMQ_BEGIN_NAMESPACE

class QZMQ_API QZmqError : public QObject
{
public:
    static int getLastError();
    static const char* getLastError(int error);

protected:
    QZmqError();
    virtual ~QZmqError();
    Q_DISABLE_COPY(QZmqError);
};

QZMQ_END_NAMESPACE

#endif // __QT_ZMQ_H__