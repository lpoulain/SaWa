/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   towa.cpp
 * Author: lpoulain
 *
 * Created on May 20, 2018, 11:01 PM
 */

#include <iostream>
#include <cstdlib>
#include <jni.h>

/*
 * ToWa: Tomcat Wannabe
 */

using namespace std;

int main(int argc, char** argv) {
    JavaVM *jvm;       /* denotes a Java VM */
//    JavaVMOption* options = new JavaVMOption[1];
//    options[0].optionString = "-Djava.class.path=;servlet-api.jar;";
    JNIEnv *env;       /* pointer to native method interface */
    JavaVMInitArgs vm_args; /* JDK VM initialization arguments */
    vm_args.version = JNI_VERSION_1_6; /* VM version */
    vm_args.nOptions = 0;
    vm_args.ignoreUnrecognized = JNI_FALSE;
    
    /* Get the default initialization arguments and set the class 
     * path */
    //JNI_GetDefaultJavaVMInitArgs(&vm_args);
    /* load and initialize a Java VM, return a JNI interface 
     * pointer in env */
    int res = JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args); // this is what it can't find

    /* invoke the Main.test method using the JNI */
    jclass TestClass = env->FindClass("Test");
    jmethodID TestClassConstructor = env->GetMethodID(TestClass, "<init>", "()V");
    jobject testInstance = env->NewObject(TestClass, TestClassConstructor);

    jclass ResponseClass = env->FindClass("SawaResponse");
    jmethodID ResponseClassConstructor = env->GetMethodID(ResponseClass, "<init>", "()V");
    jobject responseInstance = env->NewObject(ResponseClass, ResponseClassConstructor);
    
    jmethodID TestClassGetMethod = env->GetMethodID(TestClass, "doGet", "(Ljavax/servlet/http/HttpServletResponse;)V");
    jmethodID ResponseClassGetOutput = env->GetMethodID(ResponseClass, "getOutput", "()[B");
    env->CallVoidMethod(testInstance, TestClassGetMethod, responseInstance);
    jbyteArray j_value = (jbyteArray)env->CallObjectMethod(responseInstance, ResponseClassGetOutput);
    
    if(j_value != NULL)
    {
        int len = env->GetArrayLength(j_value);
        char *value = (char*)env->GetByteArrayElements(j_value, NULL);
        cout << value << endl;
//        env->ReleaseByteArrayElements(j_value, value, 0);
    }
    
    /* We could have created an Object and called methods on it instead */
    /* We are done. */
    jvm->DestroyJavaVM();
    
    return 0;
}
