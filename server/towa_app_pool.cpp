#include <iostream>
#include <cstdlib>
#include <string>
#include <sstream>
#include <vector>
#include <signal.h>
#include "towa_app_pool.h"

using namespace std;

jobject TowaAppPool::getTowaRequest(string method, string queryString) {
    jstring jmethod = env->NewStringUTF(method.c_str());
    jstring jqueryString = env->NewStringUTF(queryString.c_str());
    
    return env->NewObject(RequestClass, RequestClassConstructor, jmethod, jqueryString);
}

void _append_exception_trace_messages(
                        JNIEnv&      a_jni_env,
                        std::string& a_error_msg,
                        jthrowable   a_exception,
                        jmethodID    a_mid_throwable_getCause,
                        jmethodID    a_mid_throwable_getStackTrace,
                        jmethodID    a_mid_throwable_toString,
                        jmethodID    a_mid_frame_toString)
{
    // Get the array of StackTraceElements.
    jobjectArray frames =
        (jobjectArray) a_jni_env.CallObjectMethod(
                                        a_exception,
                                        a_mid_throwable_getStackTrace);
    jsize frames_length = a_jni_env.GetArrayLength(frames);

    // Add Throwable.toString() before descending
    // stack trace messages.
    if (0 != frames)
    {
        jstring msg_obj =
            (jstring) a_jni_env.CallObjectMethod(a_exception,
                                                 a_mid_throwable_toString);
        const char* msg_str = a_jni_env.GetStringUTFChars(msg_obj, 0);

        // If this is not the top-of-the-trace then
        // this is a cause.
        if (!a_error_msg.empty())
        {
            a_error_msg += "\nCaused by: ";
            a_error_msg += msg_str;
        }
        else
        {
            a_error_msg = msg_str;
        }

        a_jni_env.ReleaseStringUTFChars(msg_obj, msg_str);
        a_jni_env.DeleteLocalRef(msg_obj);
    }

    // Append stack trace messages if there are any.
    if (frames_length > 0)
    {
        jsize i = 0;
        for (i = 0; i < frames_length; i++)
        {
            // Get the string returned from the 'toString()'
            // method of the next frame and append it to
            // the error message.
            jobject frame = a_jni_env.GetObjectArrayElement(frames, i);
            jstring msg_obj =
                (jstring) a_jni_env.CallObjectMethod(frame,
                                                     a_mid_frame_toString);

            const char* msg_str = a_jni_env.GetStringUTFChars(msg_obj, 0);

            a_error_msg += "\n    ";
            a_error_msg += msg_str;

            a_jni_env.ReleaseStringUTFChars(msg_obj, msg_str);
            a_jni_env.DeleteLocalRef(msg_obj);
            a_jni_env.DeleteLocalRef(frame);
        }
    }

    // If 'a_exception' has a cause then append the
    // stack trace messages from the cause.
    if (0 != frames)
    {
        jthrowable cause = 
            (jthrowable) a_jni_env.CallObjectMethod(
                            a_exception,
                            a_mid_throwable_getCause);
        if (0 != cause)
        {
            _append_exception_trace_messages(a_jni_env,
                                             a_error_msg, 
                                             cause,
                                             a_mid_throwable_getCause,
                                             a_mid_throwable_getStackTrace,
                                             a_mid_throwable_toString,
                                             a_mid_frame_toString);
        }
    }
}

string TowaAppPool::GetJNIException(jthrowable exc) {
    jclass throwable_class = env->FindClass("java/lang/Throwable");
    jmethodID mid_throwable_getCause =
    env->GetMethodID(throwable_class,
                      "getCause",
                      "()Ljava/lang/Throwable;");
    jmethodID mid_throwable_getStackTrace =
    env->GetMethodID(throwable_class,
                      "getStackTrace",
                      "()[Ljava/lang/StackTraceElement;");
    jmethodID mid_throwable_toString =
    env->GetMethodID(throwable_class,
                      "toString",
                      "()Ljava/lang/String;");

    jclass frame_class = env->FindClass("java/lang/StackTraceElement");
    jmethodID mid_frame_toString =
    env->GetMethodID(frame_class,
                      "toString",
                      "()Ljava/lang/String;");

    string error;
    _append_exception_trace_messages(*env,
                                 error,
                                 exc,
                                 mid_throwable_getCause,
                                 mid_throwable_getStackTrace,
                                 mid_throwable_toString,
                                 mid_frame_toString);
    
    cout << error << endl;
    return error;
}

jclass TowaAppPool::findClass(const char *className, string errorTxt) {
    try {
        return env->FindClass(className);
    } catch (...) {
        errorMsg = new Message(char(TOWA_HTTP_404) + errorTxt);
        throw 42;
    }
}

jmethodID TowaAppPool::findMethod(jclass theClass, const char *method, const char *params, string errorTxt) {
    try {
        return env->GetMethodID(theClass, method, params);
    } catch (...) {
        errorMsg = new Message(char(TOWA_HTTP_500) + errorTxt);
        throw 42;
    }
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
    
    if (tokens.size() < 2) return nullptr;
    
    string method = tokens[0];
    string className = tokens[1];
    string queryString = "";
    if (tokens.size() >= 3) queryString = tokens[2];
    
    cout << "Towa request. Class: " << className << ", query string: " << queryString << endl;
    
    // Instanciates the TowaRequest object
    jobject requestInstance = getTowaRequest(method, queryString);
    
    // Instanciates the class
    errorMsg = nullptr;
    try {
        jclass TargetClass = findClass(className.c_str(), "Cannot find " + className + ".class");
        jmethodID TargetClassConstructor = findMethod(TargetClass, "<init>", "()V", "Cannot find contructor for class " + className + ".class");
        jobject targetInstance = env->NewObject(TargetClass, TargetClassConstructor);

        jobject responseInstance = env->NewObject(ResponseClass, ResponseClassConstructor);

        jmethodID TargetClassGetMethod = findMethod(TargetClass, "doGet", "(Ljavax/servlet/http/HttpServletRequest;Ljavax/servlet/http/HttpServletResponse;)V", "Cannot find method 'doGet' in class " + className + ".class");

        env->CallVoidMethod(targetInstance, TargetClassGetMethod, requestInstance, responseInstance);

        jthrowable exc;
        if(exc = env->ExceptionOccurred())
        {
            cout << "Java Exception" << endl;
            env->ExceptionClear();
            string error = GetJNIException(exc);
            msg_out = new Message(string(1, char(TOWA_HTTP_500)) + "<pre>" + error + "</pre>");
        }
        else {

            jbyteArray j_value = (jbyteArray)env->CallObjectMethod(responseInstance, ResponseClassGetOutput);

            if (j_value == NULL) return nullptr;

            msg_out = new Message(string(1, char(TOWA_HTTP_200)) + (char*)env->GetByteArrayElements(j_value, NULL));
        }
    }
    catch (...) {
        if (errorMsg != nullptr) msg_out = errorMsg;
    }
//    *len = env->GetArrayLength(j_value);
//    char *value = (char*)env->GetByteArrayElements(j_value, NULL);
//    cout << value << endl;
    return msg_out;
}

void termination_handler (int signum)
{
    cout << "Signal " << signum << " caught" << endl;
    throw 42;
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
    
    struct sigaction new_action, old_action;

    /* Set up the structure to specify the new action. */
    new_action.sa_handler = termination_handler;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_flags = 0;
    sigaction(SIGSEGV, &new_action, &old_action);
    
    try {
    RequestClass = env->FindClass("TowaRequest");
    RequestClassConstructor = env->GetMethodID(RequestClass, "<init>", "(Ljava/lang/String;Ljava/lang/String;)V");
    } catch (int e) {
        cout << "Exception " << e << " caught" << endl;
    }
    ResponseClass = env->FindClass("TowaResponse");
    ResponseClassConstructor = env->GetMethodID(ResponseClass, "<init>", "()V");
    ResponseClassGetOutput = env->GetMethodID(ResponseClass, "getOutput", "()[B");    
}

TowaAppPool::~TowaAppPool() {
    jvm->DestroyJavaVM();
}
