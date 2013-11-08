// PblDump.h: interface for the PblDump class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PBLDUMP_H__FD0B4158_3055_4249_883C_DC6FB2D1197B__INCLUDED_)
#define AFX_PBLDUMP_H__FD0B4158_3055_4249_883C_DC6FB2D1197B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <stdio.h>
#include <string.h>
#include "pblsdk.h"
#include <time.h>
#include <sys/types.h> 
#include <sys/timeb.h> 
#include <sys/utime.h>

#include "classes.h"

#define PBLDUMP_VERSION "1.3.1"

/*
1.3.1 
	- FIX: failed to process readonly files 
*/

class PblDump : public IPBLMI_Callback 
{
	IPBLMI* m_iPbLMI;
	IPBLMI_PBL * m_iPbl;
	int m_nEntCount;
	enum EntryType {SOURCE =0, PCODE = 1, RESOURCE = 2, NODE =3, SCCDATA = 4};
	enum DirType {ListDir, ExportDir, DeleteDir};
	enum ExportCPType {ECP_AUTO, ECP_ANSI, ECP_UNICODE};
	enum DebugFlag { DebugNone =0, DebugPrefixNameWithNode = 1};
	char * TypePrefix[5];
	DirType m_dirType;
	BOOL m_bExportPcode; // *.* exports pcode?
	ExportCPType m_iExportCP; 
	BOOL m_bDumpNodes; 
	DWORD m_DebugFlags;
	BOOL m_bExportEntryTime;
public:
	int getSCCDataSize();
	int exportSCCData(char * e_name);
	PblDump();
	virtual ~PblDump();
	int main(int argc, char ** argv);
	void  logo();
	void  usage();
	BOOL DirCallback(PBL_ENTRYINFO *pEntry);
	EntryType getEntryType( const char * szEntryName); 
	int deleteEntries(int e_num, char * e_list []);
	int deleteEntry(PBL_ENTRYINFO *pEntry, char * e_name);
	int deleteEntry(char * e_name);
	int exportEntries(int e_num, char * e_list []);
	int exportEntry(char * e_name);
	int exportEntry(PBL_ENTRYINFO *pEntry, char * e_name);
	int init(const char * szPbl, bool bReadWrite = FALSE);
	int cleanup();
	int listPBL();
	int FindEndOfDir(const char *szPath);
	int writeBinaryData(FILE *f, PBL_ENTRYINFO *pEntry, char * e_name, char * binName, BOOL isUTF);
	int convertToUTF(ByteBuffer& buf, INT16 &comm_len, INT32 &data_len);
	int convertToANSI(ByteBuffer& buf, INT16 &comm_len, INT32 &data_len, BOOL &bHexUsed);
private:
	void ListNodes(Node* apNode);
	void ListNodes(Node *apNode, char *asPath, int aiPos, char *asPrefix);
};

#define u_fprintf(f,t) \
	if (isUTF) { \
		fwprintf(f, L##t); \
	} \
	else { \
		fprintf(f, t); \
	}
#define u_fprintf1(f,t, a1) \
	if (isUTF) { \
		fwprintf(f, L##t, a1); \
	} \
	else { \
		fprintf(f, t, a1); \
	}
#define u_fprintf2(f,t, a1, a2) \
	if (isUTF) { \
		fwprintf(f, L##t, a1, a2); \
	} \
	else { \
		fprintf(f, t, a1, a2); \
	}
#define u_C(c) (isUTF ? L##c : c)
#define u_T(t) (isUTF ? L##t : t)

#endif // !defined(AFX_PBLDUMP_H__FD0B4158_3055_4249_883C_DC6FB2D1197B__INCLUDED_)
