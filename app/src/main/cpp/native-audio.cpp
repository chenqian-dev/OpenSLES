#include <asm/signal.h>//
// Created by User on 2016/11/8.
//

#include "assert.h"
#include "string.h"
#include "pthread.h"

#include "SLES/OpenSLES.h"
#include "SLES/OpenSLES_Android.h"

#include "include/LogConfig.h"
#include "include/me_phelps_opensles_JniAudioRecorder.h"

//锁
static pthread_mutex_t  audioEngineLock = PTHREAD_MUTEX_INITIALIZER;

//engine
SLObjectItf engineObj = NULL;
SLEngineItf engineItf;

//recorder
SLObjectItf recordObj = NULL;
SLRecordItf recordItf;
SLAndroidSimpleBufferQueueItf recordQueueItf;


//录音参数
SLuint32 sampleRate = SL_SAMPLINGRATE_16;  //采样率默认为16K
#define RECORDER_FRAMES  (16000 / 100)  //设置录音buffer长度为10ms
static short recorderBuffer[RECORDER_FRAMES];

//创建engine
jint Java_me_phelps_opensles_JniAudio_createEngine
        (JNIEnv *, jclass){

    if(engineObj!=NULL){
       //已经创建就不在创建
        LOGI("engine 已经创建完成，不重复创建");
    }

    SLresult result;

    //创建engine
    result = slCreateEngine(&engineObj, //engine对象指针
                            0, //numOptions  ?
                            NULL,//pEngineOptions?
                            0, //numInterfaces?
                            NULL, //pInterfaceIds?
                            NULL//pInterfaceRequired?
    );
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // realize  engine
    result = (*engineObj)->Realize(engineObj, //实现对象
                                   SL_BOOLEAN_FALSE//是否异步
    );
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    // 获取engine的接口
    result = (*engineObj)->GetInterface(engineObj,
                                        SL_IID_ENGINE,
                                        &engineItf
    );
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    LOGI("engine 创建完成");
}

/******************************************************************************************************************/

//创建recorder
jint Java_me_phelps_opensles_JniAudio_createRecorder
        (JNIEnv *, jclass){

    //配置audio source
    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, //读取类型
                                      SL_IODEVICE_AUDIOINPUT,//读取的设备类型
                                      SL_DEFAULTDEVICEID_AUDIOINPUT,//读取的设备id，（默认mic）
                                      NULL//设备
    };
    SLDataSource audioSrc = {&loc_dev, //设备
                             NULL//format
    };

    //配置audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE,//buffer类型
                                                     2 //buffer的个数
    };
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM,//音频格式
                                   1,//channel
                                   sampleRate,//采样率
                                   SL_PCMSAMPLEFORMAT_FIXED_16,//bitsPerSample(每次采样的bit数)
                                   SL_PCMSAMPLEFORMAT_FIXED_16,//容器大小
                                   SL_SPEAKER_FRONT_CENTER,//好像和声道有关？
                                   SL_BYTEORDER_LITTLEENDIAN//低字节序
    };
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    //创建recorder 需要初始化engine
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};//?
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};//?
    SLresult result = (*engineItf)->CreateAudioRecorder(engineItf, //engine
                                                        &recordObj, //recorder对象
                                                        &audioSrc,//采集源
                                                        &audioSnk,//采集sink
                                                        1,//numInterfaces?
                                                        id,//pInterfaceIds
                                                        req//pInterfaceRequired
    );
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    // realize recorder
    result = (*recordObj)->Realize(recordObj,
                                   SL_BOOLEAN_FALSE//是否异步
    );
    if (SL_RESULT_SUCCESS != result) {
        return JNI_FALSE;
    }

    //获取record接口
    result = (*recordObj)->GetInterface(recordObj,
                                        SL_IID_RECORD,
                                        &recordItf
    );
    if(SL_RESULT_SUCCESS != result){
        return JNI_FALSE;
    }

    //获取bufferQueue 的接口
    result = (*recordObj)->GetInterface(recordObj,
                                        SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                        &recordQueueItf
    );
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    //注册回调
    result = (*recordQueueItf)->RegisterCallback(recordQueueItf,
                                                 bqRecorderCallback,
                                                 NULL
    );
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    return JNI_TRUE;
}


jint Java_me_phelps_opensles_JniAudio_startRecording
        (JNIEnv *, jclass){
    //清空状态
    SLresult result;
    result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_STOPPED);//停止录制
    assert(SL_RESULT_SUCCESS == result);
    (void)result;
    result = (*recordQueueItf)->Clear(recordQueueItf);//清空队列
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    //给queue两个空的buffer，用来存放数据，至少两个
    result = (*recordQueueItf)->Enqueue(recordQueueItf, recorderBuffer,RECORDER_FRAMES * sizeof(short));
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    //开始录制
    result = (*recordItf)->SetRecordState(recordItf, SL_RECORDSTATE_RECORDING);
    assert(SL_RESULT_SUCCESS == result);
    (void)result;

    return 1;
}

jint Java_me_phelps_opensles_JniAudio_stopRecording
        (JNIEnv *, jclass){
    if (recordObj != NULL) {
        (*recordObj)->Destroy(recordObj);
        recordObj = NULL;
        recordItf = NULL;
        recordQueueItf = NULL;
    }
    return 1;
}

jint Java_me_phelps_opensles_JniAudio_destoryEngine
        (JNIEnv *, jclass){

    if (engineObj != NULL) {
        (*engineObj)->Destroy(engineObj);
        engineObj = NULL;
        engineItf = NULL;
    }
    return 1;
}

// buffer queue的回调
void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
    assert(bq == recordQueueItf);
    assert(NULL == context);
    //切换另一个buffer给record，
    

    pthread_mutex_unlock(&audioEngineLock);
}