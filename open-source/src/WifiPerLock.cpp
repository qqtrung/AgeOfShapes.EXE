#include "WifiPerLock.hpp"

#if defined(ANDROID) || defined(__ANDROID__)

#include "Debug.hpp"

#include <SFML/System/NativeActivity.hpp>

#include <android/native_activity.h>
#include <jni.h>

#include <string>

static jobject s_wifiLock = nullptr;

void WifiPerfLock::acquire()
{
    ANativeActivity* activity = sf::getNativeActivity();
    if (!activity || !activity->vm)
    {
        logout("WifiPerfLock::acquire() FAILED: activity or vm null", true);
        return;
    }

    JavaVM* vm  = activity->vm;
    JNIEnv* env = nullptr;

    jint res         = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    bool needsDetach = false;

    if (res == JNI_EDETACHED)
    {
        JavaVMAttachArgs attachArgs{};
        attachArgs.version = JNI_VERSION_1_6;
        attachArgs.name    = "WifiLockThread";
        if (vm->AttachCurrentThread(&env, &attachArgs) != JNI_OK)
        {
            logout("WifiPerfLock::acquire() FAILED: AttachCurrentThread failed", true);
            return;
        }
        needsDetach = true;
        logout("WifiPerfLock: Thread attached to JVM (was detached)");
    }
    else if (res != JNI_OK)
    {
        logout("WifiPerfLock::acquire() FAILED: GetEnv returned " + std::to_string(res), true);
        return;
    }

    jint apiLevel = 0;
    {
        jclass versionClass = env->FindClass("android/os/Build$VERSION");
        if (versionClass)
        {
            jfieldID sdkIntField = env->GetStaticFieldID(versionClass, "SDK_INT", "I");
            apiLevel             = env->GetStaticIntField(versionClass, sdkIntField);
            env->DeleteLocalRef(versionClass);
        }
    }
    const jint lockMode = (apiLevel >= 29) ? 4 : 3;
    logout("WifiPerfLock: API level = " + std::to_string(apiLevel) + ", lockMode = " + std::to_string(lockMode) +
           ((lockMode == 4) ? " (LOW_LATENCY)" : " (HIGH_PERF)"));

    jobject activityObj   = activity->clazz;
    jclass  activityClass = env->GetObjectClass(activityObj);

    jclass contextClass = env->FindClass("android/content/Context");
    if (!contextClass)
    {
        logout("WifiPerfLock::acquire() FAILED: cannot find android/content/Context", true);
        if (needsDetach)
            vm->DetachCurrentThread();
        return;
    }

    jfieldID wifiServiceField = env->GetStaticFieldID(contextClass, "WIFI_SERVICE", "Ljava/lang/String;");
    jobject  wifiServiceName  = env->GetStaticObjectField(contextClass, wifiServiceField);

    jmethodID getSystemService = env->GetMethodID(activityClass,
                                                  "getSystemService",
                                                  "(Ljava/lang/String;)Ljava/lang/Object;");
    jobject   wifiManager      = env->CallObjectMethod(activityObj, getSystemService, wifiServiceName);

    if (env->ExceptionCheck())
    {
        logout("WifiPerfLock::acquire() FAILED: exception thrown during getSystemService", true);
        env->ExceptionDescribe();
        env->ExceptionClear();
        wifiManager = nullptr;
    }

    if (wifiManager != nullptr)
    {
        logout("WifiPerfLock: Got WifiManager OK");

        jclass    wifiManagerClass = env->GetObjectClass(wifiManager);
        jmethodID createWifiLock   = env->GetMethodID(wifiManagerClass,
                                                      "createWifiLock",
                                                      "(ILjava/lang/String;)Landroid/net/wifi/WifiManager$WifiLock;");

        jstring tag           = env->NewStringUTF("EnetWifiLock");
        jobject wifiLockLocal = env->CallObjectMethod(wifiManager, createWifiLock, lockMode, tag);

        if (env->ExceptionCheck())
        {
            logout(
                "WifiPerfLock::acquire() FAILED: exception thrown during createWifiLock (missing permission "
                "CHANGE_WIFI_STATE?)",
                true);
            env->ExceptionDescribe();
            env->ExceptionClear();
            wifiLockLocal = nullptr;

            if (lockMode == 4)
            {
                logout("WifiPerfLock: LOW_LATENCY failed, fallback to HIGH_PERF...", true);
                wifiLockLocal = env->CallObjectMethod(wifiManager, createWifiLock, 3, tag);
                if (env->ExceptionCheck())
                {
                    env->ExceptionDescribe();
                    env->ExceptionClear();
                    wifiLockLocal = nullptr;
                }
            }
        }

        if (wifiLockLocal != nullptr)
        {
            s_wifiLock = env->NewGlobalRef(wifiLockLocal);

            jclass lockClass = env->GetObjectClass(s_wifiLock);

            jmethodID setRefCounted = env->GetMethodID(lockClass, "setReferenceCounted", "(Z)V");
            env->CallVoidMethod(s_wifiLock, setRefCounted, JNI_FALSE);

            jmethodID acquireMethod = env->GetMethodID(lockClass, "acquire", "()V");
            env->CallVoidMethod(s_wifiLock, acquireMethod);

            if (env->ExceptionCheck())
            {
                logout(
                    "WifiPerfLock::acquire() FAILED: exception thrown during wifiLock.acquire() (missing "
                    "permission "
                    "WAKE_LOCK?)",
                    true);
                env->ExceptionDescribe();
                env->ExceptionClear();
            }
            else
            {
                jmethodID isHeldMethod = env->GetMethodID(lockClass, "isHeld", "()Z");
                jboolean  held         = env->CallBooleanMethod(s_wifiLock, isHeldMethod);

                if (held)
                    logout("WifiPerfLock: WifiLock ACQUIRED successfully, isHeld() = true");
                else
                    logout("WifiPerfLock: WifiLock acquire() called but isHeld() = false", true);
            }

            env->DeleteLocalRef(lockClass);
            env->DeleteLocalRef(wifiLockLocal);
        }
        else
        {
            logout("WifiPerfLock::acquire() FAILED: createWifiLock returned null", true);
        }

        env->DeleteLocalRef(tag);
        env->DeleteLocalRef(wifiManagerClass);
        env->DeleteLocalRef(wifiManager);
    }
    else
    {
        logout("WifiPerfLock::acquire() FAILED: getSystemService(WIFI_SERVICE) returned null", true);
    }

    env->DeleteLocalRef(activityClass);
    env->DeleteLocalRef(contextClass);
    env->DeleteLocalRef(wifiServiceName);

    if (env->ExceptionCheck())
    {
        logout("WifiPerfLock: Clearing remaining exception...", true);
        env->ExceptionClear();
    }

    if (needsDetach)
        vm->DetachCurrentThread();
}

void WifiPerfLock::release()
{
    if (!s_wifiLock)
    {
        logout("WifiPerfLock::release() ignored: s_wifiLock is null");
        return;
    }

    ANativeActivity* activity = sf::getNativeActivity();
    if (!activity || !activity->vm)
    {
        logout("WifiPerfLock::release() FAILED: activity or vm null", true);
        return;
    }

    JavaVM* vm  = activity->vm;
    JNIEnv* env = nullptr;

    jint res         = vm->GetEnv((void**)&env, JNI_VERSION_1_6);
    bool needsDetach = false;

    if (res == JNI_EDETACHED)
    {
        JavaVMAttachArgs attachArgs{};
        attachArgs.version = JNI_VERSION_1_6;
        if (vm->AttachCurrentThread(&env, &attachArgs) != JNI_OK)
        {
            logout("WifiPerfLock::release() FAILED: AttachCurrentThread failed", true);
            return;
        }
        needsDetach = true;
    }

    jclass    lockClass    = env->GetObjectClass(s_wifiLock);
    jmethodID isHeldMethod = env->GetMethodID(lockClass, "isHeld", "()Z");

    if (env->CallBooleanMethod(s_wifiLock, isHeldMethod))
    {
        jmethodID releaseMethod = env->GetMethodID(lockClass, "release", "()V");
        env->CallVoidMethod(s_wifiLock, releaseMethod);
        logout("WifiPerfLock: WifiLock released");
    }
    else
    {
        logout("WifiPerfLock: release() called but lock was not held", true);
    }

    env->DeleteLocalRef(lockClass);
    env->DeleteGlobalRef(s_wifiLock);
    s_wifiLock = nullptr;

    if (env->ExceptionCheck())
        env->ExceptionClear();

    if (needsDetach)
        vm->DetachCurrentThread();
}

#else

void WifiPerfLock::acquire()
{
}

void WifiPerfLock::release()
{
}

#endif
