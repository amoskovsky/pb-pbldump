
#ifndef _pbl_format_h
#define _pbl_format_h

#include <wtypes.h>
//#include "pbltypes.h"

#include <stdio.h>

#pragma pack(push,1)

#define BLOCK_SIZE 0x200

struct PBL_TIME
{
	DWORD tm;
};

struct PBL_TRL
{
   char sign[4]; //"TRL*"
   DWORD hdr_offset;  // HDR offset
};

struct PBL_HDR
{
	char sign[4]; //"HDR*"
	char name[0x11-0x04]; //"PowerBuilder"
	BYTE reserved1; 
	char major_ver[2]; //"06"
	char minor_ver[2]; //"00"
	PBL_TIME creation_time;  
	WORD reserved2; // 0x0000
	char comments[BLOCK_SIZE / 2]; // sz
	INT32 sc_data_offset;  // source control data
	INT32 sc_data_length; // ?
};

struct PBL_FRE
{
	char sign[4]; //"FRE*"
	DWORD next;
	BYTE data[0x200-0x08];
};


struct PBL_DAT
{
	char	sign[4]; //"DAT*"
	DWORD	next;	//offset of next DAT from the file begin
					// or 0 if it's the last DAT in the chain
	WORD	datalen;
	char	data[0x200-0x04 - 0x4 - 0x2];
};
struct PBL_DAT_HDR
{
	char	sign[4]; //"DAT*"
	DWORD	next;	//offset of next DAT from the file begin
					// or 0 if it's the last DAT in the chain
	WORD	datalen;
};

struct PBL_ENT
{
	char	sign[4]; // "ENT*"
	char	major_ver[2]; // "06"
	char	minor_ver[2]; // "00"
	DWORD	data_offset;  // offset of the first DAT of the entry
	DWORD	data_len; //  including comments
	PBL_TIME creation_time; // 
	WORD	comment_len; // comments are stored at the beginning of the data
	WORD	entry_name_len; // including trailing zerro;
							// the name follows after this field
};
struct PBL_NOD
{
	char	sign[4]; // "NOD*"
	DWORD	left;  // offset of the left NOD from the file begin
	DWORD	parent;
	DWORD	right; // offset of the right NOD
	INT16	free_space;  // # of free bytes for entries
	INT16	last_offset; // offset of the first (alphabetical order) entry name in the node
	INT16	ent_count;
	INT16	first_offset; // offset of the last entry name in the node
	char	reserved[8];
};


#define BinDataBlockSize  1996

#pragma pack(pop)


#endif
