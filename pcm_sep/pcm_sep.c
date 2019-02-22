#define DR_WAV_IMPLEMENTATION
#include "dr_wav.h"
#define READ_FRAME 40

int main(int argc,char* argv[]){
    drwav_uint64 ret=0;
    drwav* pWav=drwav_open_file("s.wav");
    if(pWav!=NULL){
        int calc_sim_bytes=pWav->channels*pWav->bitsPerSample>>3;
        int calc_total_bytes=calc_sim_bytes*READ_FRAME;
        //分一块大内存，避免反复分配(共享内存)
        signed char *buff=(signed char*)malloc(calc_total_bytes<<1);
        signed char *l_buff=NULL,*r_buff=NULL;
        FILE *l_file=fopen("l_channel.pcm","wb+");
        FILE *r_file=fopen("r_channel.pcm","wb+");
        if(pWav->channels==1){
            l_buff=buff+calc_total_bytes;
        }else{
            l_buff=buff+calc_total_bytes;
            r_buff=l_buff+(calc_total_bytes>>1);
        }
        while((ret=drwav_read_pcm_frames_s16(pWav,READ_FRAME,(drwav_int16*)buff))){
            int read_bytes=calc_sim_bytes*(int)ret;
            signed short *pTmpBuff=(signed short*)buff;
            signed short *l_tmp=(signed short*)l_buff;
            signed short *r_tmp=(signed short*)r_buff;
            if(pWav->channels==1){
                memcpy(l_buff,pTmpBuff,read_bytes);
            }else{
                int tic=read_bytes>>1;
                while(tic>0){
                    if((tic%2)==0){
                        //左声道
                        *l_tmp=*pTmpBuff;
                        l_tmp++;
                    }else{
                        //右声道
                        *r_tmp=*pTmpBuff;
                        r_tmp++;
                    }
                    pTmpBuff++;
                    tic--;
                }
            }
            if(l_buff!=NULL&&l_file!=NULL){
                fwrite(l_buff,(pWav->channels==1)?read_bytes:read_bytes>>1,1,l_file);
            }
            if(r_buff!=NULL&&r_file!=NULL){
                fwrite(r_buff,(pWav->channels==1)?read_bytes:read_bytes>>1,1,r_file);
            }
        }
        //释放内存
        free(buff);
        if(l_file!=NULL){
            fflush(l_file);
            fclose(l_file);
        }
        if(r_file!=NULL){
            fflush(r_file);
            fclose(r_file);
        }
        printf("Finished.\n");
    }else{
        printf("Can't Found File.\n");
    }
    drwav_close(pWav);
    return 0;
}
