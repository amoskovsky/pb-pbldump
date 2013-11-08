
#ifndef _pbl_format_w_h
#define _pbl_format_w_h

#include <wtypes.h>
#include <stdio.h>
#include "pbl_format.h"

#pragma pack(push,1)



struct PBL_HDR_W
{
	char sign[4]; //"HDR*"
	char name[0x1E - 0x04]; // L"PowerBuilder"
	WORD reserved1;
	char major_ver[4]; //L"06"
	char minor_ver[4]; //L"00"
	PBL_TIME creation_time;
	WORD reserved2; // 0x0001
//	char comments[0x400 - 0x2e]; // wsz
	char comments[BLOCK_SIZE]; // wsz
	INT32 sc_data_offset;  // source control data
	INT32 sc_data_length; //
};

union PBL_HDR_U {
	PBL_HDR hdr;
	PBL_HDR_W hdr_W;
};

typedef struct PBL_FRE PBL_FRE_W;

typedef struct PBL_DAT PBL_DAT_W;

typedef struct PBL_NOD PBL_NOD_W;


struct PBL_ENT_W
{
	char	sign[4]; // "ENT*"
	char	major_ver[4]; // L"06"
	char	minor_ver[4]; // L"00"
	DWORD	data_offset;  // offset of the first DAT of the entry
	DWORD	data_len; //  including comments
	PBL_TIME creation_time; //
	WORD	comment_len; // comments are stored at the beginning of the data
	WORD	entry_name_len; // in bytes, including 2 trailing zeros;
							// the name follows after this field
};

union PBL_ENT_U {
	PBL_ENT   ent;
	PBL_ENT_W ent_W;
};



#pragma pack(pop)


#endif
