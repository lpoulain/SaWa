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
    jclass ThrowableClass;
    jclass StackTraceElementClass;
    jmethodID RequestClassConstructor;
    jmethodID ResponseClassConstructor;
    jmethodID ResponseClassGetOutput;
    jmethodID ThrowableClassGetCause;
    jmethodID ThrowableClassGetStackTrace;
    jmethodID ThrowableClassToString;
    jmethodID StackTraceElementClassToString;
    
    jclass findClass(const char *className, string errorTxt);
    jmethodID findMethod(jclass theClass, const char *method, const char *params, string errorTxt);    
    void GetJNIException(jthrowable exc, string& errorMsg);
public:
    Message *processRequest(Message *msg);
    jobject getTowaRequest(string method, string queryString);
    TowaAppPool();
    ~TowaAppPool();
};

#endif /* TOWA_APP_POOL_H */
