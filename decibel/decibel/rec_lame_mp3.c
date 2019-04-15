#include "stdafx.h"
#include "rec_lame_mp3.h"
#include "lame.h"
#ifdef _DEBUG
#pragma comment(lib,"libmp3lame-static_d.lib")
#pragma comment(lib,"libmpghip-static_d.lib")
#else
#pragma comment(lib,"libmp3lame-static.lib")
#pragma comment(lib,"libmpghip-static.lib")
#endif // DEBUG

#define	BUFF_SIZE	1024
/************************************************************************/
/* ÒôÆµÂ¼ÖÆmp3                                                          */
/************************************************************************/
static long skipId3v2(HANDLE hFile);

LP_T_MP3 LAME_Init_Encoder(int samplerate, int channels, int mode, int bitr, int original) {
	LPBYTE pShare = (LPBYTE)malloc(sizeof(T_MP3) + BUFF_SIZE << 2);
	LP_T_MP3 pSm = NULL;
	if (pShare) {
		memset(pShare, 0, sizeof(T_MP3) + (BUFF_SIZE << 2));
		pSm = (LP_T_MP3)pShare;
		pSm->start = TRUE;
		pSm->st = lame_init();
		pSm->channels = channels;
		pSm->stat = FALSE;
		lame_set_in_samplerate(pSm->st, samplerate);
		lame_set_num_channels(pSm->st, channels);
		lame_set_quality(pSm->st, 2);
		lame_set_mode(pSm->st, mode);
		if (bitr > 0) {
			lame_set_brate(pSm->st, bitr);
		}
		lame_set_VBR(pSm->st, vbr_default);
		lame_set_write_id3tag_automatic(pSm->st, 1);
		lame_init_params(pSm->st);
		pShare += sizeof(T_MP3);
		pSm->target = pShare;
		if (pSm->st != NULL) {
			pSm->samples_per_pass = LAME_Get_Samples_Per_Pass(pSm);
		}
		else {
			free(pSm);
			pSm = NULL;
		}
	}
	return pSm;
}

BYTE* LAME_Encode_Buffer_Interleaved(LP_T_MP3 lSm, INT16 *pBuffer, INT iNumBytes, INT *pWriteBytes) {
	if (lSm != NULL) {
		lSm->stat = FALSE;
		if (lSm->start) {
			if (lSm->st != NULL) {
				*pWriteBytes = lame_encode_buffer_interleaved((lame_global_flags*)lSm->st, pBuffer, (iNumBytes / lSm->channels / sizeof(INT16)), lSm->target, BUFF_SIZE << 2);
				return lSm->target;
			}
		}
	}
	return NULL;
}

BYTE* LAME_Encode_Flush(LP_T_MP3 lSm, INT *pWriteBytes) {
	if (lSm != NULL) {
		lSm->stat = FALSE;
		if (lSm->start) {
			if (lSm->st != NULL) {
				*pWriteBytes = lame_encode_flush((lame_global_flags*)lSm->st, lSm->target, BUFF_SIZE << 2);
				return lSm->target;
			}
		}
	}
	return NULL;
}

INT LAME_Get_Samples_Per_Pass(LP_T_MP3 lSm) {
	if (lSm != NULL) {
		if (lSm->st != NULL) {
			return BUFF_SIZE;
		}
	}
	return 0;
}

//LAME°á³öÀ´
static long skipId3v2(HANDLE hFile) {
	size_t  nbytes;
	long    id3v2TagSize = 0;
	unsigned char id3v2Header[10];
	if (INVALID_SET_FILE_POINTER == SetFilePointer(hFile, 0, NULL, FILE_BEGIN)) {
		return -2;
	}
	if (ReadFile(hFile, id3v2Header, sizeof(id3v2Header), &nbytes, NULL)) {
		if (nbytes != sizeof(id3v2Header)) {
			return -3;
		}
		if (!strncmp((char *)id3v2Header, "ID3", 3)) {
			id3v2TagSize = (((id3v2Header[6] & 0x7f) << 21)
				| ((id3v2Header[7] & 0x7f) << 14)
				| ((id3v2Header[8] & 0x7f) << 7)
				| (id3v2Header[9] & 0x7f)) + sizeof(id3v2Header);
		}
		else {
			id3v2TagSize = 0;
		}
	}
	return id3v2TagSize;
}

BOOL	LAME_StartAnalysis_Encoder(LP_T_MP3 lSm, LPVOID pParam) {
	HANDLE hTarget = (HANDLE)pParam;
	if (lSm != NULL) {
		if (hTarget != INVALID_HANDLE_VALUE) {
			/*BYTE cBuff3v2[BUFF_SIZE<<1];
			size_t iRet = 0, iRealWriteBytes = 0;
			iRet = lame_get_id3v2_tag(lSm->st, cBuff3v2, sizeof(cBuff3v2));
			if ((iRet = lame_get_id3v2_tag(lSm->st, cBuff3v2, sizeof(cBuff3v2))) > 0) {
				WriteFile(hTarget, cBuff3v2, iRet, &iRealWriteBytes, NULL);
			}*/
			return TRUE;
		}
	}
	return FALSE;
}

BOOL	LAME_EndAnalysis_Encoder(LP_T_MP3 lSm, LPVOID pParam) {
	lame_global_flags *gfp = NULL;
	UINT8 buffer[2880];
	INT iReadBytes = 0, iRealWriteBytes = 0, iBytes = 0;
	if (lSm != NULL) {
		if (!lSm->stat) {
			HANDLE hTarget = (HANDLE)pParam;
			if (hTarget != INVALID_HANDLE_VALUE) {
				gfp = (lame_global_flags*)lSm->st;
				iReadBytes = lame_get_lametag_frame(gfp, buffer, sizeof(buffer));
				if (iReadBytes && (iBytes = skipId3v2(hTarget)) >= 0) {
					if (INVALID_SET_FILE_POINTER != SetFilePointer(hTarget, iBytes, NULL, FILE_BEGIN)) {
						WriteFile(hTarget, buffer, iReadBytes, &iRealWriteBytes, NULL);
						return TRUE;
					}
				}
			}
		}
	}
	return FALSE;
}

BOOL LAME_Destory_Encoder(LP_T_MP3 lSm) {
	BOOL ret = FALSE;
	if (lSm != NULL) {
		if (lSm->start) {
			if (lSm->st != NULL) {
				lame_close((lame_global_flags*)lSm->st);
			}
			lSm->start = FALSE;
		}
		ret = lSm->start;
		free(lSm);
		lSm = NULL;
	}
	return ret;
}