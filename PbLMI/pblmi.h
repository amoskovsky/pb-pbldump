#ifndef _pblmi_h_
#define _pblmi_h_

#include <wtypes.h>
#include "pbltypes.h"
// PbLMI interfaces

// LMI method result codes
enum PBLMI_Result
{
   PBLMI_OK,
   PBLMI_ERROR,
   PBLMI_BADFORMAT,
   PBLMI_ABORT,
	PBLMI_NOTFOUND,
	PBLMI_DISKERROR,
	PBLMI_BADARG,
	PBLMI_ACCESS_DENIED
};

enum PBLMI_CodePage
{
   PBLMI_WIDE,
   PBLMI_ANSI,
   PBLMI_OEM,
	PBLMI_UTF8
};

#ifdef __cplusplus

class PBL_ENTRYINFO {
public:

#else 

typedef struct _PBL_ENTRYINFO {

#endif
   char  *entry_name;   // pointer to entry name 
   INT32 data_len;      // data length including comments
   INT16  comment_len;   // comments are stored at the beginning of the data
   short reserved1;
   DWORD mod_time; // entry time 
   DWORD node;
	INT32 data_offset;
	INT32 entry_offset;
	int name_len;  // length of the entry_name in bytes
	               // not including trailing zero
	INT32 entry_len; // fixed+variable
	int reserved2;
#ifdef __cplusplus

	PBL_ENTRYINFO() {
		reset();
	}
	void reset() {memset(this, 0, sizeof(PBL_ENTRYINFO));}
};

#else

} PBL_ENTRYINFO;

#endif

#ifdef __cplusplus

// forward declarations
class IPBLMI_Callback;
class IPBLMI_PBL;
class PBL_ENTRYINFO;

class IPBLMI
{
public:
   // create library
   virtual PBLMI_Result CreateLibrary(const char *szLib, int aPbVer = 6) = 0;

   // open library and return IPBLMI_PBL interface to misc. LMI functions
   virtual IPBLMI_PBL* OpenLibrary(const char *szLib, BOOL bReadWrite = FALSE) = 0;

   // close library opened by OpenLibrary
//   virtual PBLMI_Result CloseLibrary(IPBLMI_PBL *iPbl) = 0;

   // call this method to release IPBLMI interface
   // and delete underlying object
   virtual void Release(void) = 0;
	
	// set output encoding for strings
   virtual PBLMI_Result SetTargetCodePage(PBLMI_CodePage cp) = 0;
	// get output encoding
   virtual PBLMI_CodePage GetTargetCodePage() = 0;
};



class IPBLMI_PBL
{
public:
	virtual PBLMI_Result Optimize(const char *szNewLibName, int nTgtPBLVer = 0) = 0;
   virtual PBLMI_Result Dir(IPBLMI_Callback *iCallBack) = 0;
   virtual PBLMI_Result SeekEntry(const char *szEntryName, PBL_ENTRYINFO * pEntry, BOOL bCreate = FALSE) = 0;
   virtual PBLMI_Result DeleteEntry(PBL_ENTRYINFO *pEntry) = 0;
   virtual PBLMI_Result ReadEntryData(PBL_ENTRYINFO *pEntry, void * pBuf) = 0;
   virtual PBLMI_Result ReadSCCData(void * pBuf, int *pBufSize) = 0;
   virtual PBLMI_Result WriteSCCData(void * pBuf, int BufSize) = 0;
   virtual PBLMI_Result SetEntryTime(PBL_ENTRYINFO *pEntry) = 0;
   virtual PBLMI_Result UpdateEntryData
   (
      PBL_ENTRYINFO *pEntry, 
      const void *pData, 
      INT32 dwDataLen, 
      INT16 wComLen
   ) = 0;
   virtual void Close(void) = 0;
   virtual BOOL isUnicode() = 0;
   virtual int GetVersion() = 0;
	virtual void Debug() = 0;
	virtual void Flush() = 0;
   virtual PBLMI_Result ReadComments(void * pBuf, int *pBufSize)=0;
   virtual PBLMI_Result WriteComments(void * pBuf)=0;

};

class IPBLMI_Callback
{
public:
	virtual BOOL DirCallback(PBL_ENTRYINFO *pEntry) { return TRUE;}
};

#else


#endif // __cplusplus

//typedef BOOL (*PBLMI_DirCallback)(PBL_ENTRYINFO *pEntry, void* pData);
typedef BOOL   /*__cdecl*/ CALLBACK PBLMI_DirCallbackF (PBL_ENTRYINFO *pEntry, void* pData);

typedef PBLMI_DirCallbackF * PBLMI_DirCallback;


#endif
