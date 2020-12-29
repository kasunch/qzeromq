#include "qzmqerror.hpp"
#include <zmq.h>
#include <cerrno>

QZMQ_BEGIN_NAMESPACE

QZmqError::QZmqError()
{

}

QZmqError::~QZmqError()
{

}

int QZmqError::getLastError()
{
#ifdef _WIN32
    return zmq_errno();
#else
    return errno;
#endif
}

const char* QZmqError::getLastError(int error)
{
    return zmq_strerror(error);
}

QZMQ_END_NAMESPACE