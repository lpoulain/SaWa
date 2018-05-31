#ifndef TOWA_APP_POOL_H
#define TOWA_APP_POOL_H

#include <jni.h>
#include "towa_ipc.h"

class TowaAppPool {
    JavaVM *jvm;
    JNIEnv *env;
    
    jclass ResponseClass;
    jmethodID ResponseClassConstructor;
    jmethodID ResponseClassGetOutput;
public:
    Message *processRequest(const char *className, Message *msg);
    TowaAppPool();
    ~TowaAppPool();
};

#endif /* TOWA_APP_POOL_H */
