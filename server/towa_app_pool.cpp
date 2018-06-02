#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include "towa_app_pool.h"

using namespace std;

jobject TowaAppPool::getTowaRequest(string method, string queryString) {
    jstring jmethod = env->NewStringUTF(method.c_str());
    jstring jqueryString = env->NewStringUTF(queryString.c_str());
    
    return env->NewObject(RequestClass, RequestClassConstructor, jmethod, jqueryString);
}

Message *TowaAppPool::processRequest(Message *msg_in) {
    Message *msg_out;
    string msg_in_str = msg_in->getString();
    
    stringstream ss(msg_in_str);
    string item;
    vector<string> tokens;
    while (getline(ss, item, '|')) {
        tokens.push_back(item);
    }
    
    if (tokens.size() < 3) return nullptr;
    
    string method = tokens[0];
    string className = tokens[1];
    string queryString = tokens[2];
    
    // Instanciates the TowaRequest object
    jobject requestInstance = getTowaRequest(method, queryString);
    
    // Instanciates the class
    jclass TargetClass = env->FindClass(className.c_str());
    jmethodID TargetClassConstructor = env->GetMethodID(TargetClass, "<init>", "()V");
    jobject targetInstance = env->NewObject(TargetClass, TargetClassConstructor);

    jobject responseInstance = env->NewObject(ResponseClass, ResponseClassConstructor);
    
    jmethodID TargetClassGetMethod = env->GetMethodID(TargetClass, "doGet", "(Ljavax/servlet/http/HttpServletRequest;Ljavax/servlet/http/HttpServletResponse;)V");
    
    env->CallVoidMethod(targetInstance, TargetClassGetMethod, requestInstance, responseInstance);
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
    vm_args.nOptions = 1;
    JavaVMOption options[1];
    options[0].optionString = (char*)"-Djava.class.path=./classpath/servlet-api.jar:./classpath/";
    vm_args.options = options;
    vm_args.ignoreUnrecognized = JNI_FALSE;
    
    /* Get the default initialization arguments and set the class 
     * path */
    //JNI_GetDefaultJavaVMInitArgs(&vm_args);
    /* load and initialize a Java VM, return a JNI interface 
     * pointer in env */
    res = JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args); // this is what it can't find

    /* invoke the Main.test method using the JNI */

    RequestClass = env->FindClass("TowaRequest");
    RequestClassConstructor = env->GetMethodID(RequestClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    
    ResponseClass = env->FindClass("TowaResponse");
    ResponseClassConstructor = env->GetMethodID(ResponseClass, "<init>", "()V");
    ResponseClassGetOutput = env->GetMethodID(ResponseClass, "getOutput", "()[B");    
}

TowaAppPool::~TowaAppPool() {
    jvm->DestroyJavaVM();
}
