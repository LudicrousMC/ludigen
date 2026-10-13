#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <jni.h>

static void get_time(int* buf) {
    time_t raw_time = time(NULL);
    struct tm* local_time = localtime(&raw_time);
    buf[0] = local_time->tm_hour;
    buf[1] = local_time->tm_min;
    buf[2] = local_time->tm_sec;
}

void log_native(const char* prefix, const char* msg, va_list args) {
    int local_time[3];
    get_time(local_time);
    fprintf(stdout, "\033[0;34m[%02d:%02d:%02d] %s \033[0m", local_time[0], local_time[1], local_time[2], prefix);
    vfprintf(stdout, msg, args);
    printf("\n");
    fflush(stdout);
}

extern JavaVM* get_java_vm();

JNIEnv* get_jni_env() {
    JNIEnv* env = NULL;
    JavaVM* jvm = get_java_vm();
    jint res = (*jvm)->GetEnv(jvm, (void**)&env, JNI_VERSION_24);
    if (res == JNI_OK) {
        return env;
    }
    return NULL;
}

int log_java_interface(const char* method, const char* msg, va_list args) {
    JNIEnv* env = get_jni_env();
    if (env == NULL) {
        return -1;
    }
    jclass bridge = (*env)->FindClass(env, "com/endrf/LudiGen");
    if (bridge == NULL) {
        return -1;
    }
    jmethodID logMethod = (*env)->GetStaticMethodID(env, bridge, method, "(Ljava/lang/String;)V");
    if (logMethod == NULL) {
        (*env)->DeleteLocalRef(env, bridge);
        return -1;
    }

    va_list args_copy;
    va_copy(args_copy, args);
    int str_len = vsnprintf(NULL, 0, msg, args_copy) + 1;
    if (str_len < 0) {
        (*env)->DeleteLocalRef(env, bridge);
        return -1;
    }
    va_end(args_copy);
    char str[str_len];
    memset(str, 0, str_len);
    vsnprintf(str, str_len, msg, args);

    jstring message = (*env)->NewStringUTF(env, str);
    if (message == NULL) {
        (*env)->DeleteLocalRef(env, bridge);
        return -1;
    }
    (*env)->CallStaticVoidMethod(env, bridge, logMethod, message);
    (*env)->DeleteLocalRef(env, bridge);
    return 0;
}

void log_info(const char* msg, ...) {
    va_list args;
    const char* native_prefix = "\033[0;32m[LudicrousNative/INFO]";
    if (get_java_vm() == NULL) {
        va_start(args, msg);
        log_native(native_prefix, msg, args);
        va_end(args);
    } else {
        va_start(args, msg);
        int err = log_java_interface("nativeLogInfoInterface", msg, args);
        va_end(args);
        if (err) {
            va_start(args, msg);
            log_native(native_prefix, msg, args);
            va_end(args);
        }
    }
}

void log_err(const char* msg, ...) {
    va_list args;
    const char* native_prefix = "\033[0;31m[LudicrousNative/ERROR]";
    if (get_java_vm() == NULL) {
        va_start(args, msg);
        log_native(native_prefix, msg, args);
        va_end(args);
    } else {
        va_start(args, msg);
        int err = log_java_interface("nativeLogErrorInterface", msg, args);
        va_end(args);
        if (err) {
            va_start(args, msg);
            log_native(native_prefix, msg, args);
            va_end(args);
        }
    }
}

void log_debug(const char* msg, ...) {
    va_list args;
    const char* native_prefix = "\033[0;31m[LudicrousNative/DEBUG]";
    if (get_java_vm() == NULL) {
        va_start(args, msg);
        log_native(native_prefix, msg, args);
        va_end(args);
    } else {
        va_start(args, msg);
        int err = log_java_interface("nativeLogDebugInterface", msg, args);
        va_end(args);
        if (err) {
            va_start(args, msg);
            log_native(native_prefix, msg, args);
            va_end(args);
        }
    }
}