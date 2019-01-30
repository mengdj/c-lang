#include <windows.h>
#include <speex/speex_preprocess.h>
#include "bass.h"
#include "bassenc_mp3.h"
#include <stdio.h>
#pragma comment(lib,"bass.lib")
#pragma comment(lib,"bassenc_mp3.lib")
#pragma comment(lib,"bassenc.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"libspeex.lib")
#pragma comment(lib,"libspeexdsp.lib")
#ifndef T
    #define T(c)    c
#endif
#define DENOISE_DB (-90)
HRECORD rchan=0;
SpeexPreprocessState *pSps=NULL;
BOOL CALLBACK RecordingCallback(HRECORD handle, const void *buffer, DWORD length, void *user);
void CALLBACK Mp3RecordingCallback(HENCODE handle, DWORD channel, const void *buffer, DWORD length, QWORD offset, void *user);
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow){
	if(hPrevInstance!=NULL){
			return 0;
	}
	BASS_RecordFree();
	if(BASS_RecordInit(0)){
	    const char* pName=NULL;
	    int c;
	    BOOL findDevice=FALSE;
	    FILE* pFile=NULL;
	    for(c=0;pName=BASS_RecordGetInputName(c);c++){
	        if(!(BASS_RecordGetInput(c,NULL)&BASS_INPUT_OFF)){
	            findDevice=TRUE;
                break;
	        }
	    }
	    if(findDevice==TRUE){
	        BASS_RecordSetInput(0,BASS_INPUT_ON,1);
	        int iv=0,noiseSuppress=DENOISE_DB,denoise=1;
	        iv=8000;
	        pSps=speex_preprocess_state_init(1024,44100);
	        if(pSps!=NULL){
	        	speex_preprocess_ctl(pSps,SPEEX_PREPROCESS_SET_DENOISE,&denoise);
	        	speex_preprocess_ctl(pSps,SPEEX_PREPROCESS_SET_NOISE_SUPPRESS,&noiseSuppress);
	        	int vad = 1;
						int vadProbStart = 80;
						int vadProbContinue = 65;
						speex_preprocess_ctl(pSps,SPEEX_PREPROCESS_SET_VAD,&vad); 
						speex_preprocess_ctl(pSps,SPEEX_PREPROCESS_SET_PROB_START,&vadProbStart); 
						speex_preprocess_ctl(pSps,SPEEX_PREPROCESS_SET_PROB_CONTINUE,&vadProbContinue);  
	        }
	        //FILE* pFile=fopen("rec.pcm","wb+");
	        //生成文件名(取系统生成日期)
	        SYSTEMTIME systime;
	        char names[124];
	        GetLocalTime(&systime);
	        sprintf(names,"%d-%02d-%02d-%02d-%02d-%02d.mp3",systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,systime.wSecond);
	        FILE* pFile=fopen(names,"wb+");
	        //16BITS 44100KHZ Stereo
	        rchan=BASS_RecordStart(44100,2,0,RecordingCallback,NULL);
	        if(!rchan){
	            MessageBox(NULL,T("Couldn't Start Recording"),T("Failture"),MB_ICONWARNING|MB_OK);
	        }else{
	            //主线程休眠90秒钟(录制90秒)
	            if(BASS_Encode_MP3_Start(rchan,NULL,BASS_ENCODE_AUTOFREE,Mp3RecordingCallback,(void*)pFile)){
	                Sleep(90000);
	            }
                BASS_ChannelStop(rchan);
                rchan=0;
	        }
	        if(pSps!=NULL){
          	speex_preprocess_state_destroy(pSps);
          }
	    }else{
	        MessageBox(NULL,T("Can't Found Input Device."),T("Failture"),MB_ICONWARNING|MB_OK);
	    }
	    BASS_RecordFree();
	    BASS_Free();
	    if(pFile!=NULL){
	        fclose(pFile);
	        pFile=NULL;
	    }
    }else{
        MessageBox(NULL,T("Init Device Failture"),T("Failture"),MB_ICONWARNING|MB_OK);
    }
	return S_OK;
}

/** MP3解码器的回调 */
void CALLBACK Mp3RecordingCallback(HENCODE handle, DWORD channel, const void *buffer, DWORD length, QWORD offset, void *user){
    FILE* pFile=(FILE*)user;
    if(pFile!=NULL){
        //写入mp3原始音频数据
        fwrite(buffer,length,1,pFile);
    }
}

/** PCM的回调 */
BOOL CALLBACK RecordingCallback(HRECORD handle,const void *buffer, DWORD length, void *user){
    //刷新缓存，以便mp3取得数据
    if(pSps!=NULL){
    	speex_preprocess_run(pSps,buffer);
    }
    BASS_ChannelUpdate(handle,length);
    return TRUE;
}