#include "stdafx.h"
#include "rec.h"

/************************************************************************/
/* ÒôÆµÂ¼ÖÆmp3                                                          */
/************************************************************************/

LP_T_MP3 Init_Encoder(int samplerate, int channels, int mode, int bitr, int original) {
	shine_config_t config;
	shine_set_config_mpeg_defaults(&config.mpeg);
	config.wave.samplerate = samplerate;
	config.wave.channels = channels;
	config.mpeg.mode = mode;
	config.mpeg.bitr = bitr;
	config.mpeg.original = original;
	LP_T_MP3 pSm = (LP_T_MP3)malloc(sizeof(T_MP3));
	memset(pSm, 0, sizeof(sizeof(T_MP3)));
	pSm->start = TRUE;
	pSm->st = shine_initialise(&config);
	pSm->channels = channels;
	if (pSm->st != NULL) {
		pSm->samples_per_pass = Get_Samples_Per_Pass(pSm) << channels;
	}
	else {
		free(pSm);
		pSm = NULL;
	}
	return pSm;
}

BYTE *Encode_Buffer_Interleaved(LP_T_MP3 lSm, INT16 *pBuffer,INT iNumBytes,INT *pWriteBytes) {
	BYTE *ret = NULL;
	if (lSm != NULL) {
		if (lSm->start) {
			if (lSm->st != NULL) {
				ret = shine_encode_buffer_interleaved((shine_t)lSm->st, pBuffer, pWriteBytes);
			}
		}
	}
	return ret;
}

BYTE *Encode_Flush(LP_T_MP3 lSm, INT *pWriteBytes) {
	BYTE *ret = NULL;
	if (lSm != NULL) {
		if (lSm->start) {
			if (lSm->st != NULL) {
				return shine_flush((shine_t)lSm->st, pWriteBytes);
			}
		}
	}
	return ret;
}

INT Get_Samples_Per_Pass(LP_T_MP3 lSm) {
	if (lSm != NULL) {
		if (lSm->st != NULL) {
			return shine_samples_per_pass((shine_t)lSm->st);
		}
	}
	return 0;
}
BOOL		StartAnalysis_Encoder(LP_T_MP3 lSm, LPVOID p) {
	return FALSE;
}
BOOL		EndAnalysis_Encoder(LP_T_MP3 lSm, LPVOID p) {
	return FALSE;
}

BOOL Destory_Encoder(LP_T_MP3 lSm) {
	BOOL ret = FALSE;
	if (lSm != NULL) {
		if (lSm->start) {
			if (lSm->st != NULL) {
				shine_close((shine_t)lSm->st);
			}
			lSm->start = FALSE;
		}
		ret = lSm->start;
		free(lSm);
		lSm = NULL;
	}
	return ret;
}