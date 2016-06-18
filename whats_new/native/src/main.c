#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "firmware.h"
#include "jni_facade.h"
#include "device_ftdi.h"
#include "device_hid.h"

#define DEBUG() fprintf_s(fd, "line: %d\n", __LINE__); \
                fflush(fd);

#define DEBUG_ENV(env) fprintf_s(fd, "line: %d, exception: %d\n", __LINE__, (*(env))->ExceptionCheck((env))); \
                fflush(fd);

enum Event
{
    DEVICE_CONNECT = 0x00000002,
    DEVICE_DISCONNECT = 0x00000004,
    DEVICE_RECVEIVE = 0x00000008,
    DEVICE_SENT = 0x00000010,
    DEVICE_HID = 0x00000020,
    DEVICE_FTDI = 0x00000040,
    FIRMWARE_42 = 0x00000080,
    FIRMWARE_60 = 0x00000100,
    CLOSE = 0x00000000,
    START = 0x00000001
};


int group(unsigned char b[2])
{
    if (b[0] >= '0' && b[0] <= '9') {
        if (b[1] >= '0' && b[1] <= '9')
            return ((b[0] - '0') << 4 | b[1] - '0') & 255;
        return ((b[0] - '0') << 4 | (b[1] - 'A' + 10)) & 255;
    }
    if (b[1] >= '0' && b[1] <= '9')
        return ((b[0] - 'A' + 10) << 4 | b[1] - '0') & 255;
    return ((b[0] - 'A' + 10) << 4 | (b[1] - 'A' + 10)) & 255;
}

static JavaVM *vm = NULL;
static JNIEnv *env = NULL;
static jobject callback;
static jobjectArray packets;
static jclass    packetClass;
static jmethodID packetContructor;
static jfieldID  packetGroup;
static jfieldID  packetIndex;
static jfieldID  packetBattery;
static jfieldID  packetTimeout;
static jfieldID  packetEnter;
static jobject   packetInitObject;

static FILE     *fd = NULL;
static int registered = FALSE;


/** void connected(ru.iv.support.dll.Device device, ru.iv.support.dll.Firmware firmware);(Lru/iv/support/dll/Device;Lru/iv/support/dll/Firmware;)V*/
static jmethodID connected;
/** void disconnected(ru.iv.support.dll.Device device, ru.iv.support.dll.Firmware firmware);(Lru/iv/support/dll/Device;Lru/iv/support/dll/Firmware;)V*/
static jmethodID disconnected;
/** void handle(ru.iv.support.dll.Device device, ru.iv.support.dll.Firmware firmware, ru.iv.support.dll.Packet[] packets, int count);(Lru/iv/support/dll/Device;Lru/iv/support/dll/Firmware;[Lru/iv/support/dll/Packet;I)V*/
static jmethodID handle;

static volatile HANDLE event_thread_handle;
static volatile DWORD  event_thread_id;
static volatile HANDLE event;
static volatile int runnable = FALSE;
static volatile struct _Device  df;
static volatile struct _Device  dh;


static void
receive_cb(int fw, struct _Result result[100], int count, void *user)
{
    JNIEnv *userEnv = (JNIEnv *)user;
    if (userEnv) {
        int i = 0;
        DEBUG_ENV(userEnv);
        for (i = 0; i < count; ++i) {
            jobject object = (*userEnv)->GetObjectArrayElement(userEnv, packets, i);
            DEBUG_ENV(userEnv);
            (*userEnv)->SetIntField(userEnv, object, packetGroup, result[i].group);
            DEBUG_ENV(userEnv);
            (*userEnv)->SetIntField(userEnv, object, packetIndex, result[i].index);
            DEBUG_ENV(userEnv);
            (*userEnv)->SetIntField(userEnv, object, packetBattery, result[i].battery);
            DEBUG_ENV(userEnv);
            (*userEnv)->SetIntField(userEnv, object, packetTimeout, result[i].timeout);
            DEBUG_ENV(userEnv);
            (*userEnv)->SetObjectField(userEnv, object, packetEnter, (*userEnv)->NewStringUTF(userEnv, result[i].enter_s));
            DEBUG_ENV(userEnv);
        }
        (*userEnv)->CallVoidMethod(userEnv, callback, handle, 1, df.fw == FW42 ? 0 : 1, packets, (jint)count);
        DEBUG_ENV(userEnv);
    }

}

static DWORD WINAPI
EventLoop(LPVOID pData)
{
    static JNIEnv *eventEnv = NULL;
    DWORD   rx_wait;
    HANDLE  h[8];
    int i = 0;

    h[0] = df.events[DEVICE_STATUS_OPENED];
    h[1] = df.events[DEVICE_STATUS_CLOSED];
    h[2] = df.events[DEVICE_STATUS_READED];
    h[3] = df.events[DEVICE_STATUS_WRITED];

    h[4] = dh.events[DEVICE_STATUS_OPENED];
    h[5] = dh.events[DEVICE_STATUS_CLOSED];
    h[6] = dh.events[DEVICE_STATUS_READED];
    h[7] = dh.events[DEVICE_STATUS_WRITED];
    runnable = TRUE;
    DEBUG();
    (*vm)->AttachCurrentThread(vm, (void **)&eventEnv, NULL);
    DEBUG();
    fprintf_s(fd, "Event env: %p", eventEnv);
    while (runnable) {
        rx_wait = WaitForMultipleObjects(8, h, FALSE, 1000);
        switch (rx_wait) {
        case WAIT_OBJECT_0 + DEVICE_STATUS_OPENED:
        {
            fprintf(fd, "FTDI connected\n");
            DEBUG();
            fflush(fd);
            if (eventEnv) {
                (*eventEnv)->CallVoidMethod(eventEnv, callback, connected, 1, df.fw == FW42 ? 0 : 1);
            }
            break;
        }

        case WAIT_OBJECT_0 + DEVICE_STATUS_CLOSED:
        {
            fprintf(fd, "FTDI disconnected\n");
            DEBUG();
            if (eventEnv) {
                (*eventEnv)->CallVoidMethod(eventEnv, callback, disconnected, 1, df.fw == FW42 ? 0 : 1);
            }
            break;
        }

        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_OPENED:
        {
            fprintf(fd, "HID  connected\n");
            DEBUG();
            if (eventEnv) {
                (*eventEnv)->CallVoidMethod(eventEnv, callback, connected, 0, df.fw == FW42 ? 0 : 1);
            }
            break;
        }

        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_CLOSED:
        {
            fprintf(fd, "HID  disconnected\n");
            DEBUG();
            if (eventEnv) {
                (*eventEnv)->CallVoidMethod(eventEnv, callback, disconnected, 0, df.fw == FW42 ? 0 : 1);
            }
            break;
        }


        case WAIT_OBJECT_0 + DEVICE_STATUS_READED: /** Приняли с FTDI */
        {
            device_receive(receive_cb, eventEnv, &df);
            break;
        }

        case WAIT_OBJECT_0 + 4 + DEVICE_STATUS_READED: /** Приняли с HID */
        {
            device_receive(receive_cb, eventEnv, &dh);
            break;
        }
        default:
            break;
        }
        //        if (i > 0) {
        //            uint8_t d[] = { 2, 6, 'Q', '2', 'D', '\r', 0, 0 };

        //            i = 0;
        //            fw_crc_create(&d[2], 4, &d[6]);
        //            device_write(ev->dh, d, sizeof(d));
        //        }
    }
    DEBUG();
    (*vm)->DetachCurrentThread(vm);
    DEBUG();
    return 0;
}

static jobject get_field_value(JNIEnv *env, jclass classObject, const char *name, const char * signature)
{
    jfieldID field = (*env)->GetStaticFieldID(env, classObject, name, signature);
    return (*env)->GetStaticObjectField(env, classObject, field);
}

void JNICALL Java_ru_iv_support_dll_Library_register(JNIEnv *callEnv, jclass unusedJclass, jobject callbackObject)
{
    if (registered == FALSE) {
        int i = 0;

        DEBUG_ENV(callEnv);
        jclass callbackClass = (*callEnv)->GetObjectClass(callEnv, callbackObject);
        DEBUG_ENV(callEnv);
        packetClass = (*callEnv)->FindClass(callEnv, "ru/iv/support/Packet");
        DEBUG_ENV(callEnv);
        jobjectArray packetsArray = (*callEnv)->NewObjectArray(callEnv, 100, packetClass, NULL);
        DEBUG_ENV(callEnv);
        callback = (*callEnv)->NewGlobalRef(callEnv, callbackObject);
        DEBUG_ENV(callEnv);
        packetContructor = (*callEnv)->GetMethodID(callEnv, packetClass, "<init>", "()V");
        DEBUG_ENV(callEnv);
        for (i = 0; i < 100; ++i) {
            jobject object = (*callEnv)->NewObject(callEnv, packetClass, packetContructor);
            (*callEnv)->SetObjectArrayElement(callEnv, packetsArray, i, object);
        }
        connected = (*callEnv)->GetMethodID(callEnv, callbackClass, "connected", "(II)V");
        DEBUG_ENV(callEnv);
        disconnected = (*callEnv)->GetMethodID(callEnv, callbackClass, "disconnected", "(II)V");
        DEBUG_ENV(callEnv);
        handle = (*callEnv)->GetMethodID(callEnv, callbackClass, "handle", "(II[Lru/iv/support/Packet;I)V");
        DEBUG_ENV(callEnv);
        packets = (*callEnv)->NewGlobalRef(callEnv, packetsArray);
        DEBUG_ENV(callEnv);
        packetGroup = (*callEnv)->GetFieldID(callEnv, packetClass, "group", "I");
        DEBUG_ENV(callEnv);
        packetIndex = (*callEnv)->GetFieldID(callEnv, packetClass, "index", "I");
        DEBUG_ENV(callEnv);
        packetBattery = (*callEnv)->GetFieldID(callEnv, packetClass, "battery", "I");
        DEBUG_ENV(callEnv);
        packetTimeout = (*callEnv)->GetFieldID(callEnv, packetClass, "timeout", "I");
        DEBUG_ENV(callEnv);
        packetEnter = (*callEnv)->GetFieldID(callEnv, packetClass, "enter", "Ljava/lang/String;");
        DEBUG_ENV(callEnv);

        DEBUG();
        device_ftdi_init(&df);
        DEBUG();
        device_hid_init(&dh);
        DEBUG();
        DEBUG_ENV(callEnv);
        event = CreateEvent(NULL, FALSE, FALSE, NULL);
        DEBUG_ENV(callEnv);
        event_thread_handle = CreateThread(NULL, 0, EventLoop, NULL, 0, &event_thread_id);
        DEBUG_ENV(callEnv);
        registered = TRUE;
        DEBUG();
    }
}

void JNICALL Java_ru_iv_support_dll_Library_send(JNIEnv *callEnv, jclass unusedJclass, jint id, jstring strCommand, jboolean unuJboolean)
{
    static uint8_t buffer[26];
    size_t len;

    const char *command = (*callEnv)->GetStringUTFChars(callEnv, strCommand, NULL);
    DEBUG_ENV(callEnv);
    len = strlen(command);
    if (len < 20) {
        buffer[0] = 2;
        memcpy(buffer + 2, command, len);
        buffer[1] = len + 2;
        fw_crc_create(&buffer[2], len + 1, &buffer[len + 2]);
        fprintf(fd, "COMMAND: %.*s\n", (int)len + 4, (const char *)buffer);
        if (id == 0) {
            device_write(&dh, buffer, len + 5);
        } else if (id == 1) {
            device_write(&df, buffer, len + 5);
        }
    }
    (*callEnv)->ReleaseStringUTFChars(callEnv, strCommand, command);
    DEBUG_ENV(callEnv);
}

jboolean JNICALL Java_ru_iv_support_dll_Library_hasDevice(JNIEnv *callEnv, jclass unusedJclass, jint id)
{
    if (id == 0) {
        return (jboolean)dh.fw != NONE;
    } else if (id == 1) {
        return (jboolean)df.fw != NONE;
    }
    return JNI_FALSE;
}

jboolean JNICALL Java_ru_iv_support_dll_Library_init(JNIEnv *callEnv, jclass unusedJclass)
{
    if (!(*callEnv)->GetJavaVM(callEnv, &vm) < 0) {
        return JNI_FALSE;
    }
    (*vm)->AttachCurrentThread(vm, (void **)&env, NULL);
    if (env != NULL) {
        fopen_s(&fd, "support.log", "a");
        if (fd == NULL)
            return JNI_FALSE;
        return JNI_TRUE;
    }
    return JNI_FALSE;
}

jboolean JNICALL Java_ru_iv_support_dll_Library_destroy(JNIEnv *callEnv, jclass unusedJclass)
{
    if (vm != NULL) {
        (*vm)->DetachCurrentThread(vm);
    }
    CloseHandle(event);
    TerminateThread(event_thread_handle, -1);
    if (fd) {
        fclose(fd);
    }
    device_hid_exit(&dh);
    device_ftdi_exit(&df);
    return JNI_TRUE;
}
