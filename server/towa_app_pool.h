#ifndef TOWA_APP_POOL_H
#define TOWA_APP_POOL_H

#include <jni.h>
#include "towa_ipc.h"

class TowaAppPool {
    JavaVM *jvm;
    JNIEnv *env;
    
    jclass RequestClass;
    jclass ResponseClass;
    jmethodID RequestClassConstructor;
    jmethodID ResponseClassConstructor;
    jmethodID ResponseClassGetOutput;
public:
    Message *processRequest(Message *msg);
    jobject getTowaRequest(string method, string queryString);
    TowaAppPool();
    ~TowaAppPool();
};

#endif /* TOWA_APP_POOL_H */
