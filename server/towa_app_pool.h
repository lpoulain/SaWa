#ifndef TOWA_APP_POOL_H
#define TOWA_APP_POOL_H

#include <jni.h>
#include "towa_ipc.h"

class TowaAppPool {
    JavaVM *jvm;
    JNIEnv *env;
    Message *errorMsg;
    
    jclass RequestClass;
    jclass ResponseClass;
    jmethodID RequestClassConstructor;
    jmethodID ResponseClassConstructor;
    jmethodID ResponseClassGetOutput;
    
    jclass findClass(const char *className, string errorTxt);
    jmethodID findMethod(jclass theClass, const char *method, const char *params, string errorTxt);    
    string GetJNIException(jthrowable exc);
public:
    Message *processRequest(Message *msg);
    jobject getTowaRequest(string method, string queryString);
    TowaAppPool();
    ~TowaAppPool();
};

#endif /* TOWA_APP_POOL_H */
