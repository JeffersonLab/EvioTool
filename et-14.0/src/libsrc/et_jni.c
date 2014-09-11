/*----------------------------------------------------------------------------*
 *  Copyright (c) 2010        Jefferson Science Associates,                   *
 *                            Thomas Jefferson National Accelerator Facility  *
 *                                                                            *
 *    This software was developed under a United States Government license    *
 *    described in the NOTICE file included as part of this distribution.     *
 *                                                                            *
 *    Author:  Carl Timmer                                                    *
 *             timmer@jlab.org                   Jefferson Lab, MS-12B3       *
 *             Phone: (757) 269-5130             12000 Jefferson Ave.         *
 *             Fax:   (757) 269-6248             Newport News, VA 23606       *
 *                                                                            *
 *----------------------------------------------------------------------------*
 *
 * Description:
 *      Routines to get, put, and dump events through Java's JNI interface
 *
 *----------------------------------------------------------------------------*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "et_private.h"
#include "org_jlab_coda_et_EtJniAccess.h"


static int debug = 0;
static int localByteOrder;

/* cache some frequently used values */
static jclass eventImplClass, byteBufferClass;
static jfieldID fid[4];
static jmethodID constrMethodId2, constrMethodId3,
                 getPriorityVal, getDataStatusVal;


/*
 * Class:     org_jlab_coda_et_EtJniAccess
 * Method:    closeLocalEtSystem
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_org_jlab_coda_et_EtJniAccess_closeLocalEtSystem
        (JNIEnv *env, jobject thisObj, jlong etId)
{
    et_close((et_sys_id)etId);
}


/*
 * Class:     org_jlab_coda_et_EtJniAccess
 * Method:    openLocalEtSystem
 * Signature: (Ljava/lang/String;)J
 */
JNIEXPORT void JNICALL Java_org_jlab_coda_et_EtJniAccess_openLocalEtSystem
        (JNIEnv *env, jobject thisObj, jstring fileName)
{
    int err;
    const char* mappedFile;
    et_sys_id id; /* (void *) */
    et_openconfig openconfig;
    jclass clazz, class1, classEventImpl, byteBufferImpl;
    jmethodID mid;

    /* get C string from java arg */
    mappedFile = (*env)->GetStringUTFChars(env, fileName, NULL);
    if (mappedFile == NULL) {
        /* pending exception already thrown, need to return for it to complete */
        (*env)->ReleaseStringUTFChars(env, fileName, mappedFile);
        return;
    }
    
    /* open ET system */
    et_open_config_init(&openconfig);
    et_open_config_sethost(openconfig, ET_HOST_LOCAL); /* must be local host */
    err = et_open(&id, mappedFile, openconfig);
    if (err != ET_OK) {
        if (err == ET_ERROR_TIMEOUT) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtTimeoutException");
        }
        else {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtException");
        }
        (*env)->ThrowNew(env, clazz, "openLocalEtSystem: cannot open ET system in native code");
        return;
    }
    et_open_config_destroy(openconfig);

    /* store byte order of local system */
    err = etNetLocalByteOrder(&localByteOrder);
    if (err != ET_OK) {
        localByteOrder = ET_ENDIAN_LITTLE;
    }

    /* store et id in object by calling this native method */
    class1 = (*env)->GetObjectClass(env, thisObj);
    mid    = (*env)->GetMethodID(env, class1, "setLocalEtId", "(J)V");
    (*env)->CallVoidMethod(env, thisObj, mid, (jlong)id);

    /*******************************************/
    /* cache objects for efficient, future use */
    /*******************************************/
    byteBufferImpl  = (*env)->FindClass(env, "java/nio/ByteBuffer");
    byteBufferClass = (*env)->NewGlobalRef(env, byteBufferImpl);
 
    classEventImpl = (*env)->FindClass(env, "org/jlab/coda/et/EtEventImpl");
    eventImplClass = (*env)->NewGlobalRef(env, classEventImpl);
 
    /* find id's of all the fields that we'll read/write directly to */
    fid[0] = (*env)->GetFieldID(env, classEventImpl, "id",         "I");
    fid[1] = (*env)->GetFieldID(env, classEventImpl, "length",     "I");
    fid[2] = (*env)->GetFieldID(env, classEventImpl, "byteOrder",  "I");
    fid[3] = (*env)->GetFieldID(env, classEventImpl, "control",    "[I");

    /* methods to get event's enum values */
    getPriorityVal   = (*env)->GetMethodID(env, classEventImpl, "getPriorityValue",   "()I");
    getDataStatusVal = (*env)->GetMethodID(env, classEventImpl, "getDataStatusValue", "()I");

    /* get id's of a few different constructors */
    constrMethodId2 = (*env)->GetMethodID(env, classEventImpl, "<init>", "(IIIIIIIILjava/nio/ByteBuffer;)V");
    constrMethodId3 = (*env)->GetMethodID(env, classEventImpl, "<init>", "(IIIIIIIIII[ILjava/nio/ByteBuffer;)V");
    
    if (debug) printf("\nopenLocalEtSystem (native) : done, opened ET system\n\n");
}

/*
 * The following routine WORKS but is slower than
 * the version in which the buffer is set in Java.
 */
 
/*
 * Class:     org_jlab_coda_et_EtJniAccess
 * Method:    getEvents
 * Signature: (JIIIIII)[Lorg/jlab/coda/et/EtEvent;
 */
JNIEXPORT jobjectArray JNICALL Java_org_jlab_coda_et_EtJniAccess_getEvents
        (JNIEnv *env , jobject thisObj, jlong etId, jint attId,
         jint mode, jint sec, jint nsec, jint count)
{
    int i, j, numread, status, biteOrder;
    void *data;
    et_event *pe[count];
    jclass clazz;
    jboolean isCopy;
    jint* intArrayElems;
    jintArray controlInts;
    jobjectArray eventArray;
    jobject event, byteBuf;

    /* translate timeout */
    struct timespec deltaTime;
    deltaTime.tv_sec  = sec;
    deltaTime.tv_nsec = nsec;

if (debug) printf("getEvents (native) : will attempt to get events\n");

    /* reading array of up to "count" events */
    status = et_events_get((et_sys_id)etId, (et_att_id)attId, pe, mode, &deltaTime, count, &numread);
    if (status != ET_OK) {
        if (status == ET_ERROR_DEAD) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtDeadException");
        }
        else if (status == ET_ERROR_WAKEUP) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtWakeUpException");
        }
        else if (status == ET_ERROR_TIMEOUT) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtTimeoutException");
        }
        else if (status == ET_ERROR_CLOSED) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtClosedException");
        }
        else if (status == ET_ERROR_BUSY) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtBusyException");
        }
        else if (status == ET_ERROR_EMPTY) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtEmptyException");
        }
        else {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtException");
        }
        
        (*env)->ThrowNew(env, clazz, "getEvents (native): cannot get events");
        return NULL;
    }

    /* create array of EventImpl objects */
    eventArray = (*env)->NewObjectArray(env, numread, eventImplClass, NULL);

    /* fill array */
    for (i=0; i < numread; i++) {
        /*printf("getEvents (native) : data for event %d = %d\n", i, *((int *)pe[i]->pdata));*/

        /* create control int array */
        controlInts   = (*env)->NewIntArray(env, ET_STATION_SELECT_INTS);
        intArrayElems = (*env)->GetIntArrayElements(env, controlInts, &isCopy);
        for (j=0; j < ET_STATION_SELECT_INTS; j++) {
            intArrayElems[j] = pe[i]->control[j];
        }
        if (isCopy == JNI_TRUE) {
            (*env)->ReleaseIntArrayElements(env, controlInts, intArrayElems, 0);
        }
        
        /* If we're on a little endian machine, int args will be swapped as
           they go through the jni interface. We don't want this for the int
           designating the byte order, so swap it here to compensate. */
        biteOrder = pe[i]->byteorder;
        if (localByteOrder == ET_ENDIAN_LITTLE) {
            biteOrder = ET_SWAP32(biteOrder);
        }

        /* wrap data pointer in ByteBuffer object */
        et_event_getdata(pe[i], (void **) &data);
        byteBuf = (*env)->NewDirectByteBuffer(env, data, (jlong) pe[i]->memsize);

        /* create event object */
        event = (*env)->NewObject(env, eventImplClass, constrMethodId3, /* constructor args ... */
        (jint)pe[i]->memsize, (jint)pe[i]->memsize, (jint)pe[i]->datastatus,
        (jint)pe[i]->place,   (jint)pe[i]->age,     (jint)pe[i]->owner,
        (jint)pe[i]->modify,  (jint)pe[i]->length,  (jint)pe[i]->priority,
        (jint)biteOrder, controlInts, byteBuf);
      
        /* put event in array */
        (*env)->SetObjectArrayElement(env, eventArray, i, event);

        /* get rid of uneeded references - good if numread is big */
        (*env)->DeleteLocalRef(env, event);
        (*env)->DeleteLocalRef(env, controlInts);
        (*env)->DeleteLocalRef(env, byteBuf);
    }

if (debug) printf("getEvents (native) : filled array!\n");
   
    /* return the array */
    return eventArray;
}


/* Keep a copy of the original routine around just in case. */
/*
 * Class:     org_jlab_coda_et_EtJniAccess
 * Method:    newEvents
 * Signature: (JIIIIIII)[Lorg/jlab/coda/et/EtEventImpl;
 */
JNIEXPORT jobjectArray JNICALL Java_org_jlab_coda_et_EtJniAccess_newEvents
        (JNIEnv *env, jobject thisObj, jlong etId, jint attId, jint mode,
         jint sec, jint nsec, jint count, jint size, jint group)
{
    int i, j, numread, status, biteOrder;
    void *data;
    et_event *pe[count];
    jclass clazz;
    jboolean isCopy;
    jobjectArray eventArray;
    jobject event, byteBuf;

    /* translate timeout */
    struct timespec deltaTime;
    deltaTime.tv_sec  = sec;
    deltaTime.tv_nsec = nsec;

    if (debug) printf("newEvents (native) : will attempt to get new events\n");
    
    /* reading array of up to "count" events */
    status = et_events_new_group((et_sys_id)etId, (et_att_id)attId, pe, mode,
                                  &deltaTime, size, count, group, &numread);
    if (status != ET_OK) {
        if (status == ET_ERROR_DEAD) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtDeadException");
        }
        else if (status == ET_ERROR_WAKEUP) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtWakeUpException");
        }
        else if (status == ET_ERROR_TIMEOUT) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtTimeoutException");
        }
        else if (status == ET_ERROR_CLOSED) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtClosedException");
        }
        else if (status == ET_ERROR_BUSY) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtBusyException");
        }
        else if (status == ET_ERROR_EMPTY) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtEmptyException");
        }
        else {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtException");
        }
        
        (*env)->ThrowNew(env, clazz, "newEvents (native): cannot get new events");
        return NULL;
    }

    /* create array of EventImpl objects */
    eventArray = (*env)->NewObjectArray(env, numread, eventImplClass, NULL);    
        
    /* fill array */
    for (i=0; i < numread; i++) {        
        /* If we're on a little endian machine, int args will be swapped as
           they go through the jni interface. We don't want this for the int
           designating the byte order, so swap it here to compensate. */
        biteOrder = pe[i]->byteorder;
        if (localByteOrder == ET_ENDIAN_LITTLE) {
            biteOrder = ET_SWAP32(biteOrder);
        }

        /* wrap data pointer in ByteBuffer object */
        et_event_getdata(pe[i], (void **) &data);
        byteBuf = (*env)->NewDirectByteBuffer(env, data, (jlong) pe[i]->memsize);

        /* create event object */
        event = (*env)->NewObject(env, eventImplClass, constrMethodId2, /* constructor args ... */
        (jint)pe[i]->memsize, (jint)pe[i]->memsize,
        (jint)pe[i]->place,   (jint)pe[i]->owner,
        (jint)pe[i]->modify,  (jint)pe[i]->length,  (jint)pe[i]->priority,
        (jint)biteOrder, byteBuf);
      
        /* put event in array */
        (*env)->SetObjectArrayElement(env, eventArray, i, event);

        /* get rid of uneeded references - good if numread is big */
        (*env)->DeleteLocalRef(env, event);
        (*env)->DeleteLocalRef(env, byteBuf);
    }

    if (debug) printf("newEvents (native) : filled array!\n");

    /* return the array */
    return eventArray;
}


/*
 * Class:     org_jlab_coda_et_EtJniAccess
 * Method:    putEvents
 * Signature: (JI[Lorg/jlab/coda/et/EtEvent;II)V
 */
JNIEXPORT void JNICALL Java_org_jlab_coda_et_EtJniAccess_putEvents
        (JNIEnv *env, jobject thisObj, jlong etId, jint attId,
         jobjectArray events, jint length)
{
    int i, j, status, place, biteOrder;
    et_event *pe[length];
    jclass clazz = NULL;
    jobject event;
    jboolean isCopy;
    jint *controlElements;
    jintArray controlInts;
    et_id *etid = (et_id *) etId;
    
if (debug) printf("putEvents (native) : put 'em back\n");

     /* create array of (pointers to) events */
    for (i=0; i<length; i++) {
        /* get event object from Java array of events */
        event = (*env)->GetObjectArrayElement(env, events, i);
        
        /* find ptr to event in C world */
        place = (*env)->GetIntField(env, event, fid[0]);
        pe[i] = ET_P2EVENT(etid, place);
        
        /* set fields in event struct that may have been modified in Java */
        pe[i]->priority   = (*env)->CallIntMethod(env, event, getPriorityVal);
        pe[i]->length     = (uint64_t) ((*env)->GetIntField(env, event, fid[1]));
        pe[i]->datastatus = (*env)->CallIntMethod(env, event, getDataStatusVal);
        
        /* If we're on a little endian machine, ints will be swapped as
           they go through the jni interface. We don't want this for the int
           designating the byte order, so swap it back again to compensate. */
        pe[i]->byteorder = (*env)->GetIntField(env, event, fid[2]);
        if (localByteOrder == ET_ENDIAN_LITTLE) {
            pe[i]->byteorder = ET_SWAP32(pe[i]->byteorder);
        }

        /* set control ints */
        controlInts     = (*env)->GetObjectField(env, event, fid[3]);
        controlElements = (*env)->GetIntArrayElements(env, controlInts, &isCopy);
        for (j=0; j<ET_STATION_SELECT_INTS; j++) {
            pe[i]->control[j] = controlElements[j];
        }

        /* clean up */
        if (isCopy == JNI_TRUE) {
            (*env)->ReleaseIntArrayElements(env, controlInts, controlElements, 0);
        }

        (*env)->DeleteLocalRef(env, event);
    }
    
    /* put back array of events (pointers) */
    status = et_events_put((et_sys_id)etId, (et_att_id)attId, pe, length);
    if (status != ET_OK) {
        if (status == ET_ERROR_DEAD) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtDeadException");
        }
        else if (status == ET_ERROR_CLOSED) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtClosedException");
        }
        else {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtException");
        }
        
        (*env)->ThrowNew(env, clazz, "putEvents (native): cannot put events");
        return;
    }

    return;
}


/*
 * Class:     org_jlab_coda_et_EtJniAccess
 * Method:    dumpEvents
 * Signature: (JI[Lorg/jlab/coda/et/EtEventImpl;I)V
 */
JNIEXPORT void JNICALL Java_org_jlab_coda_et_EtJniAccess_dumpEvents
        (JNIEnv *env, jobject thisObj, jlong etId, jint attId, jobjectArray events, jint length)
{
    int i, j, status, place;
    et_event *pe[length];
    jclass clazz = NULL;
    jobject event;
    et_id *etid = (et_id *) etId;
    
if (debug) printf("dumpEvents (native) : dump 'em\n");

    /* create array of (pointers to) events */
    for (i=0; i<length; i++) {
        /* get event object from Java array of events */
        event = (*env)->GetObjectArrayElement(env, events, i);
        
        /* find ptr to event in C world */
        place = (*env)->GetIntField(env, event, fid[0]);
        pe[i] = ET_P2EVENT(etid, place);
        
        (*env)->DeleteLocalRef(env, event);
    }
    
    /* dump array of events (pointers) */
    status = et_events_dump((et_sys_id)etId, (et_att_id)attId, pe, length);
    if (status != ET_OK) {
        if (status == ET_ERROR_DEAD) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtDeadException");
        }
        else if (status == ET_ERROR_CLOSED) {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtClosedException");
        }
        else {
            clazz = (*env)->FindClass(env, "org/jlab/coda/et/exception/EtException");
        }
        (*env)->ThrowNew(env, clazz, "dumpEvents (native): cannot dump events");
        return;
    }

    return;
}
