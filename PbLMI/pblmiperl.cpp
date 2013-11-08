#include "stdafx.h"
#include "pblsdk.h"
#include <stdio.h>
#include "debug.h"
#include <time.h>
#include "classes.h"

#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )


extern "C" {
unsigned long crc32(
   unsigned long crc, 
   const unsigned char * buf, 
   unsigned int len
);
}

static BOOL bInitialized = FALSE;

/**
 * @return 
 *	size of entry
 * or negative error code
 */
extern "C"
int PBLMI_ExportEntry(
								const char * szPBL, 
								const char * szEntry, 
								char *buf, int bufSize, 
								int *pCommSize, 
								time_t * pEntryTime,
								BOOL * isUnicode
							) 
{
   IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, FALSE);
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	PBLMI_Result ret;	
   
	*isUnicode = iPbl->isUnicode();
	PBL_ENTRYINFO entry;
	ret = iPbl->SeekEntry(szEntry, &entry, FALSE);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return 0;
   }
	if (!bufSize) {
		iPbl->Close();
		iPbLMI->Release();
		if (pCommSize)
			*pCommSize = entry.comment_len;
		if (pEntryTime)
			*pEntryTime = entry.mod_time;
		return entry.data_len;
	}
	if (bufSize < (int)entry.data_len) {
		iPbl->Close();
		iPbLMI->Release();
		return -3;
	}
	
	ret = iPbl->ReadEntryData(&entry, buf);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -4;
   }
   iPbl->Close();
   iPbLMI->Release();
	if (pCommSize)
		*pCommSize = entry.comment_len;
	if (pEntryTime)
		*pEntryTime = entry.mod_time;
   return entry.data_len;   
}


extern "C"
int PBLMI_SetEntryTime(
								const char * szPBL, 
								const char * szEntry, 
								time_t entryTime
							) 
{
   IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, TRUE); // for RW
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	PBLMI_Result ret;	
   
	PBL_ENTRYINFO entry;
	ret = iPbl->SeekEntry(szEntry, &entry, FALSE);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -3;
   }
	entry.mod_time = entryTime;
	iPbl->SetEntryTime(&entry);
   iPbl->Close();
   iPbLMI->Release();
   return 0;   
}


extern "C"
int PBLMI_DeleteEntry(
								const char * szPBL, 
								const char * szEntry
							) 
{

	//FILE * logout = fopen("pblmi.log", "a+t");
	//fprintf(logout, "!!!delete entry: %s ", szEntry); 
	//fclose(logout);
   IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, TRUE); // for RW
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	PBLMI_Result ret;	
   
	PBL_ENTRYINFO entry;
	ret = iPbl->SeekEntry(szEntry, &entry, FALSE);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -3;
   }
	ret = iPbl->DeleteEntry(&entry);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -4;
   }
	iPbl->Close();
	iPbLMI->Release();
   return 0;   
}

/**
 * @return 
 *	0
 * or negative error code
 */
extern "C"
int PBLMI_ImportEntry(
								const char * szPBL, 
								const char * szEntry, 
								char *buf, 
								int dataSize, 
								int commSize, 
								time_t entryTime
							) 
{
   IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, TRUE);
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	PBLMI_Result ret;	
   
	PBL_ENTRYINFO entry;
	ret = iPbl->SeekEntry(szEntry, &entry, TRUE);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -4;
   }
	entry.mod_time = entryTime;
	ret = iPbl->UpdateEntryData(&entry, buf, dataSize, commSize);
	if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
		return -3;
	}
   iPbl->Close();
   iPbLMI->Release();
   return 0;   
}


extern "C"
int PBLMI_Optimize(
								const char * szSrcPBL, 
								const char * szTgtPBL,
								int nTgtPBLVer
							) 
{
	if (!bInitialized)
		return -5;
	//FILE * logout = fopen("pblmi.log", "a+t");
	///fprintf(logout, "!!!delete entry: %s ", szEntry); 
	//fclose(logout);
   IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szSrcPBL, FALSE); // 
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	PBLMI_Result ret;	
   ret = iPbl->Optimize(szTgtPBL, nTgtPBLVer);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -3;
   }
	iPbl->Close();
	iPbLMI->Release();
   return 0;   
}

BOOL CheckKey(const char *szKey1, const char *szCheckSum, time_t aTime) 
{
	//printf ("PBLMI: time %u\n", aTime);
	const char *szKey = "pmlmi_1";
	const int MAXKEYLEN = 128  ;
	const int BUFLEN = MAXKEYLEN + 
		                1  /* separator */ + 
		                10 /* max digits in time_t*/ + 
							 1  /* zero */;
	char buf[BUFLEN];
	strncpy(buf, szKey, MAXKEYLEN);
	char *s = buf + strlen(buf);
	sprintf(s, "!%u", aTime);
	int l = strlen(buf);
	//printf ("PBLMI: str '%s' len=%i\n", buf, l);
	unsigned long crc = crc32(0L, (const unsigned char *)buf, l) ^ 0x196d2e2a;
	//printf ("PBLMI: crc %u\n", crc);
	sprintf(buf, "%u", crc);
	return !strcmp(szCheckSum, buf);
}

extern "C"
BOOL PBLMI_Init(const char *szInitString1, const char *szInitString2) 
{
	time_t now = time(NULL);
	for (time_t t = now - 5; t < now + 6; t ++) {
		if (CheckKey(szInitString1, szInitString2, t)) {
			bInitialized = TRUE;
			return TRUE;
		}
	}
	bInitialized = FALSE;
	return FALSE;
}


extern "C"
int PBLMI_Dir(
								const char * szPBL, 
								PBLMI_DirCallback dirCallback,
								void *pData
							) 
{
	if (!bInitialized)
		return -5;
	//FILE * logout = fopen("pblmi.log", "a+t");
	///fprintf(logout, "!!!delete entry: %s ", szEntry); 
	//fclose(logout);
   IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, FALSE); 
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	class Dir1 : public IPBLMI_Callback {
		PBLMI_DirCallback m_Dir;
		void *m_pData;
	public:
		Dir1 (PBLMI_DirCallback d, void *p): m_Dir(d), m_pData(p){}
		virtual BOOL DirCallback(PBL_ENTRYINFO *pEntry) { 
			return m_Dir(pEntry, m_pData);
		}

	} dir(dirCallback, pData);
	PBLMI_Result ret;	
   ret = iPbl->Dir(&dir);
   if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
      return -3;
   }
	iPbl->Close();
	iPbLMI->Release();
   return 0;   
}


extern "C"
int PBLMI_GetPBLVersion(const char * szPBL) 
{
	if (!bInitialized)
		return -5;
	IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, FALSE); 
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	int v = iPbl->GetVersion();
	iPbl->Close();
	iPbLMI->Release();
   return v;   
}

extern "C"
int PBLMI_IsUnicodePBL(const char * szPBL) 
{
	if (!bInitialized)
		return -5;
	IPBLMI* iPbLMI = PBLMI_GetInterface();
   if (!iPbLMI) {
      return -1;
   }
   IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(szPBL, FALSE); 
   if (!iPbl) {
		iPbLMI->Release();
      return -2;
   }
	BOOL b = iPbl->isUnicode();
	iPbl->Close();
	iPbLMI->Release();
   return (b ? 1 : 0);   
}




char_array tmpBuf(10000);
char_array outBuf(20000);
char_array commBuf(600);

//extern "C"
DLLEXPORT
int APIENTRY PBLMI_ExportSourceEntryW(
								const wchar_t * szPBL, 
								const wchar_t * szEntry, 
								char **source,  
								char **comment,  
								time_t * pEntryTime
							) 
{
	//dp("+PBLMI_ExportSourceEntryW\n");
	//dumpMemory(szPBL, 100, "szPBL");
	//dumpMemory(szEntry, 50, "szEntry");
	IPBLMI* iPbLMI = PBLMI_GetInterface();
	if (!iPbLMI) {
		return -1;
	}
	char_array library(0);
	WideToAnsi(szPBL, library);
	char_array entryName(0);
	WideToAnsi(szEntry, entryName);
	//dumpMemory(&library[0], 0, "library");
	//dumpMemory(&entryName[0], 0, "entryName");


	IPBLMI_PBL * iPbl = iPbLMI->OpenLibrary(&library[0], FALSE);
	if (!iPbl) {
		iPbLMI->Release();
		return -2;
	}
	PBLMI_Result ret;	
   
	BOOL isUnicode = iPbl->isUnicode();
	PBL_ENTRYINFO entry;
	ret = iPbl->SeekEntry(&entryName[0], &entry, FALSE);
	if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
		return 0;
	}

	tmpBuf.resize(entry.data_len + 2);

	int commSize = (isUnicode ? entry.comment_len : entry.comment_len * 2); //in bytes of target UTF16
	commBuf.resize(commSize + 2);
	if (pEntryTime)
		*pEntryTime = entry.mod_time;
	
	ret = iPbl->ReadEntryData(&entry, &tmpBuf[0]);
	if (ret != PBLMI_OK) {
		iPbl->Close();
		iPbLMI->Release();
		return -4;
	}
	iPbl->Close();
	iPbLMI->Release();

	*comment = &commBuf[0];
	commBuf[commSize] = '\0';
	commBuf[commSize + 1] = '\0';
	tmpBuf[entry.data_len] = 0;
	if (isUnicode) {
		tmpBuf[entry.data_len + 1] = 0;
		*source = &tmpBuf[commSize];
		memcpy(&commBuf[0], &tmpBuf[0], commSize);
	}
	else {
		int li_NewLen = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED,
			(LPCSTR)&tmpBuf[0],
			entry.data_len + 1, // incl 0
			NULL, 
			0 // return buf size in wchar's
		);
		if (li_NewLen <= 0) {
			return -5;
		}
		outBuf.resize(li_NewLen * 2);
		int ret = MultiByteToWideChar(
			CP_ACP,
			MB_PRECOMPOSED,
			(LPCSTR)&tmpBuf[0],
			entry.data_len + 1, // incl 0
			(LPWSTR)&outBuf[0], 
			li_NewLen
		);
		*source = &outBuf[commSize]; 
		memcpy(&commBuf[0], &tmpBuf[0], commSize);
		
	}
    return 1;  
}


