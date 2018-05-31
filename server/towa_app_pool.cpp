#include <iostream>
#include <cstdlib>
#include <string>
#include "towa_app_pool.h"

using namespace std;

Message *TowaAppPool::processRequest(const char *className, Message *msg_in) {
    Message *msg_out;
    
    // Instanciates Test
    jclass TargetClass = env->FindClass(className);
    jmethodID TargetClassConstructor = env->GetMethodID(TargetClass, "<init>", "()V");
    jobject targetInstance = env->NewObject(TargetClass, TargetClassConstructor);

    jobject responseInstance = env->NewObject(ResponseClass, ResponseClassConstructor);
    
    jmethodID TargetClassGetMethod = env->GetMethodID(TargetClass, "doGet", "(Ljavax/servlet/http/HttpServletResponse;)V");
    
    env->CallVoidMethod(targetInstance, TargetClassGetMethod, responseInstance);
    jbyteArray j_value = (jbyteArray)env->CallObjectMethod(responseInstance, ResponseClassGetOutput);
    
    if (j_value == NULL) return nullptr;

    msg_out = new Message(env->GetArrayLength(j_value), (uint8_t *)env->GetByteArrayElements(j_value, NULL));
    
//    *len = env->GetArrayLength(j_value);
//    char *value = (char*)env->GetByteArrayElements(j_value, NULL);
//    cout << value << endl;
    return msg_out;
}

TowaAppPool::TowaAppPool() {
    int res;
    JavaVMInitArgs vm_args; /* JDK VM initialization arguments */
    vm_args.version = JNI_VERSION_1_6; /* VM version */
    vm_args.nOptions = 0;
    vm_args.ignoreUnrecognized = JNI_FALSE;
    
    /* Get the default initialization arguments and set the class 
     * path */
    //JNI_GetDefaultJavaVMInitArgs(&vm_args);
    /* load and initialize a Java VM, return a JNI interface 
     * pointer in env */
    res = JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args); // this is what it can't find

    /* invoke the Main.test method using the JNI */

    ResponseClass = env->FindClass("TowaResponse");
    ResponseClassConstructor = env->GetMethodID(ResponseClass, "<init>", "()V");
    ResponseClassGetOutput = env->GetMethodID(ResponseClass, "getOutput", "()[B");    
}

TowaAppPool::~TowaAppPool() {
    jvm->DestroyJavaVM();
}
