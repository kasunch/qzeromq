# QZeroMQ - A C++ Qt library for ØMQ

QZeroMQ is a [Qt](https://www.qt.io/) library written in C++ for [ØMQ](https://zeromq.org/), a widely used asynchronous messaging library. 
QZeroMQ provides an elegant socket API and seamlessly integrates with Qt's native event loop.

# Why QZeroMQ?

In short, QZeroMQ solves the problem of integrating ØMQ's sockets into Qt's native event loop.
Solving this problem particularly challenging since ØMQ's socket gives only edge triggered notifications instead of level triggered notifications. 

This is nicely explained in the articles [ZeroMQ - Edge Triggered Notification](https://funcptr.net/2012/09/10/zeromq---edge-triggered-notification/) and [Embedding ZeroMQ In The Libev Event Loop](https://funcptr.net/2013/04/20/embedding-zeromq-in-the-libev-event-loop/). 
QZeroMQ integrates with Qt's event loop via ``aboutToBlock()`` and ``awake()`` signals of [QAbstractEventDispatcher](https://doc.qt.io/qt-5/qabstracteventdispatcher.html).

# How to use the API?

The socket API of QZeroMQ can be used as follows.

```c
    // Global header file
    #include <qzmq.hpp>

    // Connecting to an end-point
    QZmqSocket *client = QZmqSocket::create((ZMQ_PAIR);
    client->connect("inproc://test");

    // Binding to an end-point
    QZmqSocket *server = QZmqSocket::create(ZMQ_PAIR);
    server->bind("inproc://test");

    // Connecting signals
    connect(client, &QZmqSocket::onMessage, this, &MyClass::onClientMessage);
    connect(server, &QZmqSocket::onMessage, this, &MyClass::onServerMessage);

    // Sending messages
    QZmqMessage *msg = QZmqMessage::create();
    client->send(msg);

    // Closing sockets
    delete client;
    delete server;
```

For more details, refer to the examples in the [perf](perf) directory.

# Developer notes

*   Like ØMQ sockets, ``QZmqSockets`` are **NOT thread safe**. No locks are used.
    
    ``QZmqSockets`` cannot be moved between threads since each ``QZmqSocket`` is associated with the event loop of the thread in which the socket is created.
    In other words, create/destroy ``QZmqSockets`` in the thread that use them. 

*   [ØMQ's API](http://api.zeromq.org/) can be used together with QZeroMQ's API with care.

    Use ``QZmqSocket::zmqSocket()`` to access the underlying raw ØMQ socket. 
    Therefore, QZeroMQ does not necessarily provide one-to-one mapped functions for all of the [ØMQ's API](http://api.zeromq.org/) functions.

# How to build
QZeroMQ's build system is based on [Cmake](https://cmake.org/).

## On Linux
You can build QZeroMQ as follows with make.

```bash
# Create a directory for build artifacts
mkdir build
cd build
# Use Cmake to generate build tool (make) files
cmake .. -DCMAKE_PREFIX_PATH="<QT installation directory>;<ZeroMQ installation directory>" -DWITH_PERF_TOOL=ON
# Build using make
make

```

## On macOS
To be written

## On Windows
To be written

# How to use in your project
Coming soon..

# Licensing
QZeroMQ is licensed under the [Apache License, Version 2.0](https://www.apache.org/licenses/LICENSE-2.0).