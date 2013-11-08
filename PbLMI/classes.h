#ifndef _classes_h_
#define _classes_h_

#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )

#include "pblmi.h"
#include "pbl_format.h"
#include "pbl_format_w.h"
#include <stdio.h>





const int BlockSize = 0x200;
const int FreeBlockSize = BlockSize;
const int FreeBlockHeaderSize = 8;
const int FreeBlockBitSize = (FreeBlockSize - FreeBlockHeaderSize) * 8;
const int DATDataSize = BlockSize - 10;
const int NodeSize = 0x0C00;
const int NodeEntOffset = 0x0020;
const int NodeDataSize = NodeSize - NodeEntOffset;
const int EntFixedLen = 0x0018;
const int EntFixedLen_W = 0x001C;
const int NodeLeftOffset = 0x0004;
const int NodeParentOffset = 0x0008;
const int NodeRightOffset = 0x000C;
const char * const DefPbSign = "0600";
const int MaxEntryNameSize = 64; // symbols
const int MAXFILEPATH = 1024;

const int DIR_LEFT = -1;
const int DIR_RIGHT = 1;

typedef int dir_t;
#define inv_dir(d) (-1 * d)

class Node {
public:
	PBL_NOD nod;
	Node* left;
	Node* right;
	Node* parent;
	DWORD offset;
};
//helper storage class

class ByteBuffer
{
   private:
   BYTE *m_pBuf;
	int m_Size;

   public:
   ~ByteBuffer(){FreeBuffer();}
   inline BYTE * & buffer() {return m_pBuf;}
   inline operator char*() {return (char*)m_pBuf;}
   inline BYTE& operator [](int i) {return m_pBuf[i];}
   inline int&  size() {return m_Size;}
   inline void FreeBuffer() { 
		if(m_pBuf) {
			delete [] m_pBuf; 
			m_pBuf = NULL;
			m_Size = 0;
		}
	}
	inline void CreateBuffer(int nSize) {
		//FreeBuffer();
		if (nSize >= 0) {
			m_pBuf = new BYTE[nSize + 2];
			m_Size = nSize;
		}
	}
   
	ByteBuffer(ByteBuffer &b) {
		CreateBuffer(b.size());
		if (size() > 0)
			memcpy(m_pBuf, b.buffer(), size());
	}
	ByteBuffer(BYTE * bp,  int nSize) {
		CreateBuffer(nSize);
		if (size() > 0)
			memcpy(m_pBuf, bp, size());
	}
	ByteBuffer(char * bp) {
		CreateBuffer(strlen(bp) + 1);
		if (size() > 0)
			memcpy(m_pBuf, bp, size());
	}
   ByteBuffer() {
		m_pBuf = NULL;
		m_Size = 0;
	}
	ByteBuffer(int nSize) {CreateBuffer(nSize);}
   
	int Read(FILE *f, int nSize) {
		FreeBuffer();
		CreateBuffer(nSize);
		if (size() > 0)
			return fread(m_pBuf,nSize, 1, f);
		else
			return -1;
	}
	inline void SetBuffer(BYTE * bp, int nSize) {
		m_pBuf = bp;
		m_Size = nSize;
	}
	int Read(FILE *f, int nSize, int nSeek) {
		fseek(f, nSeek, SEEK_SET);
		return Read(f, nSize);
	}
   void Extend(int nAddSize, BYTE fillByte = 0) {
		int nCurrSize = m_pBuf ? m_Size : 0;
		int nNewSize = nCurrSize + nAddSize; 
		BYTE * pNewBuf = new BYTE[nNewSize];
		if (m_pBuf && nCurrSize) {
			for (int i = 0; i < nCurrSize; i++)
				pNewBuf[i] = m_pBuf[i];
		}
		for (int i = nCurrSize; i < nNewSize; i++)
			pNewBuf[i] = fillByte;
		FreeBuffer();
		SetBuffer(pNewBuf, nNewSize);
	}
};

const int UNIT_SIZE = 16;
template <class T> 
class Array {
private:
   T *m_Array;
   int m_nSize;
	int m_nRealSize;
	int m_nUnitSize;
private:
public:
	Array() {
		m_nUnitSize = UNIT_SIZE;
		m_nRealSize = calcRealSize(0);
		m_Array = new T[m_nRealSize];
		m_nSize = 0;
	}
   Array(int nSize, int nUnitSize = UNIT_SIZE) {
		m_nUnitSize = nUnitSize;
		m_nRealSize = calcRealSize(nSize);
		m_Array = new T[m_nRealSize];
		m_nSize = nSize;
	}
   ~Array() {delete [] m_Array;}
   //inline operator T* () {return m_Array;}
   inline T& operator [] (int nIndex) {return m_Array[nIndex];}
   inline int size() {return m_nSize;} 
	void resize(int nSize) {
		if (nSize <= m_nRealSize) {
			m_nSize = nSize;
		}
		else {
			int nNewRealSize = calcRealSize(nSize);
			T *p = new T[nNewRealSize];
			for (int i = 0; i < m_nSize; i++)
				p[i] = m_Array[i];
			delete [] m_Array;
			m_Array = p;
			m_nRealSize = nNewRealSize;
			m_nSize = nSize;
		}
	}
private:
	int calcRealSize(int nSize) {
		if (nSize && nSize % m_nUnitSize) {
			return nSize;
		}
		else {
			return nSize + m_nUnitSize - nSize % m_nUnitSize;
		}
	}
};



typedef Array<ByteBuffer> ByteBufferArray;
typedef Array<char*> LPSTRArray;
typedef Array<char> char_array;


class PBLMI;
class PBLMI_PBL;

struct FreeListItem {
	DWORD offset;
	BOOL changed;
};
typedef Array<FreeListItem> FreeListArray;
typedef Array<DWORD> BlockList;

class PBLMI_PBL: public IPBLMI_PBL
{
   friend class PBLMI;
   friend class Ent;
   protected:
   BOOL  m_bIsPBL;
   FILE  *m_Handle;
   char  m_PbSign[4];
   BOOL  m_bUTF;
   DWORD m_StartOffset;
	PBLMI *m_iPbLMI;
	BOOL  m_WriteMode;
   DWORD m_RootNodeOffset;
	DWORD m_FreeListFirstOffset;
	ByteBuffer m_FreeListData;
	FreeListArray m_FreeList;
	BOOL  m_FreeListRead;
	int m_EntFixedLen;
	int m_CharSize;
	int m_PbVer;
   inline BOOL IsPBL() {return m_bIsPBL;}


   public:


	PBLMI_Result 
	SeekEntry(const char *szEntryName, PBL_ENTRYINFO * pEntry, BOOL bCreate);

	PBLMI_Result 
	DeleteEntry(PBL_ENTRYINFO *pEntry);

	PBLMI_Result 
	ReadEntryData(PBL_ENTRYINFO *pEntry, void * pBuf);

	PBLMI_Result 
	SetEntryTime(PBL_ENTRYINFO *pEntry);

	PBLMI_Result 
	UpdateEntryData
		(
			PBL_ENTRYINFO *pEntry, 
			const void *pData, INT32 dwDataLen, INT16 wComLen
		);
   PBLMI_Result ReadSCCData(void * pBuf, int *pBufSize);
   PBLMI_Result WriteSCCData(void * pBuf, int BufSize);
   PBLMI_Result ReadComments(void * pBuf, int *pBufSize);
   PBLMI_Result WriteComments(void * pBuf);
	PBLMI_Result Optimize(const char *szNewLibName, int nTgtPBLVer = 0);

	PBLMI_PBL(PBLMI *aPbLMI, const char *szLibName, BOOL bReadWrite);

	~PBLMI_PBL();

	void 
	Close(void);
   BOOL isUnicode();
	int GetVersion() {
		return m_PbVer;
	}


	PBLMI_Result Dir(IPBLMI_Callback *iCallBack);

	void Flush() {
		if (!m_Handle)
			return;
		// write durty buffers
		WriteFreeListData();
		fflush(m_Handle);

		// force reading FreeList on next ReadFreeListData
		// should we do so?
		m_FreeListRead = FALSE;
	}


	protected:

	PBLMI_Result ReadNodeEntries
	(
		DWORD dwNode, 
		IPBLMI_Callback *iCallBack
	);
	PBLMI_Result 
	SeekEntry(const char *szEntryName, PBL_ENTRYINFO * pEntry, BOOL bCreate, DWORD dwNode);
	
	int 
	StrCmp (const void * s1, const void * s2);
	int 
	StrLen (const void * s1);
	PBLMI_Result ReadFreeListData();
	PBLMI_Result WriteFreeListData();
	PBLMI_Result AllocateFreelistBlocks(int aCount);
	PBLMI_Result AllocateBlocks(int aCount, BlockList &aList, BOOL ab_Continuous = FALSE);
	inline void MarkBlock(int ai_BlockIndex, int ai_State = 1){
		m_FreeListData[ai_BlockIndex] = ai_State;
		m_FreeList[int(ai_BlockIndex / FreeBlockBitSize)].changed = TRUE;
	}
	PBLMI_Result FreeBlocks(BlockList &aList, int ai_StartIndex = 0, int ai_Count = -1);
	PBLMI_Result WriteData(void * ap_Buf, int ai_Size, long ai_Offset=-1);
	PBLMI_Result ReadData(void * ap_Buf, int ai_Size, long ai_Offset=-1);
	PBLMI_Result CreateNode(DWORD adw_ParentNode, DWORD &adw_NewNode);
	PBLMI_Result AddEntryToNode(
				PBL_ENTRYINFO *pEntry, 
				BOOL ab_ToFirst,
				BOOL ab_ToLast,
				DWORD adw_Node, 
				PBL_NOD * pNod
   ); 
	void PackEntry(PBL_ENTRYINFO *pEntry, PBL_ENT *pEnt, PBL_ENT_W *pEnt_W);
	void UnpackEntry(PBL_ENTRYINFO *pEntry, PBL_ENT *pEnt, PBL_ENT_W *pEnt_W);
	PBLMI_Result 
	ListBlocks(DWORD aFirstOffset, BlockList &aList);
	PBLMI_Result 
	PBLMI_PBL::UpdateBlocksData
	(
		INT32 *a_dwFirstBlockOffset, 
		const void *pBuf, INT32 bufLen, BOOL a_bAllocEmpty 
	);
	PBLMI_Result 
	MoveDownNode(DWORD aNode, DWORD aNewParent);
	PBLMI_Result CreateOptimizedEntries
	(
		Array<Ent> & names,
		Array<int> & nodepos,
		int posIndex1,
		int posIndex2,
		IPBLMI_PBL* aiNewPBL
	) ;
	PBLMI_Result UpdateOptimizedEntries
	(
		Array<Ent> & names,
		IPBLMI_PBL* aiNewPBL
	); 
	public: 
		void FreeNodeDump(Node* apNode);
		Node* DumpNodes();
		void Debug() {
			//BlockList bb;
			//PBLMI_Result ret ;
			//ret = AllocateBlocks(15, bb);
			
			//ret = FreeBlocks(bb, 3, 2);
			//ret = AllocateBlocks(8, bb, TRUE);
			//ret = AllocateBlocks(6, bb);
			//DWORD n;
			//ret = CreateNode(m_RootNodeOffset, n);

			
		}

private:
	void _print_node(FILE* aOut = NULL, int aLevel = 0, DWORD aNode = 0, DWORD aParentNode = 0);
	void _dump_node_tree();
	PBLMI_Result DumpNodes(Node* apNode, DWORD adwNode );
};




class PBLMI : public IPBLMI
{
	friend class PBLMI_PBL;
   private:
	PBLMI_CodePage m_TgtCp;
   public:

	PBLMI() ;
	// create library
	PBLMI_Result 
	CreateLibrary(const char *szLib, int aPbVer = 6);

	// open library and return IPBLMI_PBL interface to misc. LMI functions
	IPBLMI_PBL* 
	OpenLibrary(const char *szLib, BOOL bReadWrite);

	void 
	Release(void);

	// set output encoding for strings
	PBLMI_Result 
	SetTargetCodePage(PBLMI_CodePage enc);
	// get output encoding
	PBLMI_CodePage 
	GetTargetCodePage();

	protected:
	char * ConvCodePage(const char * as_Src, int ai_SrcLen,  BOOL ab_SrcIsWide) ;
};



class Ent {
public:
	Ent() {
		offset = -1;
		deleted = FALSE;
		name = NULL;
		len = 0;
		freename = FALSE;
	}
	~Ent() {
		if (freename && name)
			delete [] name;
	}

	static PBLMI_PBL * iPbl;
	char * name; 
	int offset; // -1 - curr entry
	int len; 
	BOOL deleted;
	BOOL freename;
	static int __cdecl cmp (const void  *a, const void *b) 
	{ // for qsort
		return iPbl->StrCmp(((Ent*)a)->name, ((Ent*)b)->name);
	}
	static int __cdecl cmprev (const void  *a, const void *b) 
	{ // for qsort
		return iPbl->StrCmp(((Ent*)b)->name, ((Ent*)a)->name);
	}
	static int __cdecl cmpansi (const void  *a, const void *b) 
	{ // for qsort
		return strcmp(((Ent*)a)->name, ((Ent*)b)->name);
	}
	static int __cdecl cmprevansi (const void  *a, const void *b) 
	{ // for qsort
		return strcmp(((Ent*)b)->name, ((Ent*)a)->name);
	}
};

char * MultiToWide(const char * as_Src, int ai_Len, UINT ai_Cp);
char * WideToMulti(const char * as_Src, int ai_Len, UINT ai_Cp);
void WideToAnsi(const wchar_t * src, char_array &tgt);
UINT MapToCP(PBLMI_CodePage cp);
void PrintLastError();

#define Get1(a)  (m_bUTF ? a##_W : a)
#define GetF(a,b)  (m_bUTF ? a##_W.b : a.b)
#define GetPF(a,b)  (m_bUTF ? a##_W->b : a->b)
#define SetF(a,b,e)  {if (m_bUTF) a##_W.b = e; else a.b = e;}
#define SetPF(a,b,e)  {if (m_bUTF) a##_W->b = e; else a->b = e;}
#define SizeOf(a)  (m_bUTF ? sizeof(a##_W) : sizeof(a))
#define SizeOfF(a, f)  (m_bUTF ? sizeof(a##_W.f) : sizeof(a.f))
#define SizeOfPF(a, f)  (m_bUTF ? sizeof(a##_W->f) : sizeof(a->f))

#endif