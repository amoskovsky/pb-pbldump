
#ifndef _pblsdk_h_
#define _pblsdk_h_

#include "pblmi.h"

// global IPBLMI generator
extern "C"
IPBLMI* 
PBLMI_GetInterface();

/*
struct PBL_ENTRYINFO
{
	char	*entry_name;	// pointer to entry name 
	DWORD	data_len; 		// data length including comments
	WORD	comment_len; 	// comments are stored at the beginning of the data
	DWORD mod_time; // entry time 
	BYTE	internal_data[8];
};
*/
#endif // ndef _pblsdk_h_