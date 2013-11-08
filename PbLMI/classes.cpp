#include "stdafx.h"

#include "classes.h"
#include "debug.h"
#include <time.h>


PBLMI_PBL * Ent::iPbl = NULL;

PBLMI_Result
PBLMI_PBL::SeekEntry(const char *szEntryName, PBL_ENTRYINFO * pEntry, BOOL bCreate) {
	pEntry->entry_name = NULL;
	PBLMI_Result ret = this->SeekEntry(szEntryName, pEntry, bCreate, m_StartOffset + m_RootNodeOffset);
	if (pEntry->entry_name) {
		delete [] pEntry->entry_name;
		pEntry->entry_name = NULL;
	}
	return ret;
}

int PBLMI_PBL::StrCmp (const void * s1, const void * s2)
{
	if (m_bUTF)
		return wcsicmp((const wchar_t*)s1, (const wchar_t*)s2);
	else
		return stricmp((const char*)s1, (const char*)s2);
}
int PBLMI_PBL::StrLen (const void * s1)
{
	if (m_bUTF)
		return wcslen((const wchar_t*)s1);
	else
		return strlen((const char*)s1);
}
PBLMI_Result
PBLMI_PBL::SeekEntry(const char *szEntryName, PBL_ENTRYINFO * pEntry, BOOL bCreate, DWORD dwNode)
{
//   my ($me, $entry, $create, $nod) = @_;
   // в Perl $create - ref to array [0, 0, 0, 0]
	// где находятся значения по умолчанию для ENT*
	// в C++ это просто bool, а значения передаются в pEntry

   //$nod = $$me[$StartOffset] + 0x200 + $FreeBlockSize if !defined $nod;
	//bool debug = false;
	//if (dwNode == 0x4E00)
	//	debug = true;

	if (! dwNode)
		return PBLMI_NOTFOUND;
	if( szEntryName) {
		pEntry->name_len = strlen(szEntryName);
		if (m_bUTF) {
			pEntry->entry_name =  MultiToWide(szEntryName,
														 pEntry->name_len,
														 CP_ACP);
			pEntry->name_len = wcslen((wchar_t *)pEntry->entry_name) * 2;
		}
		else {
			// see delete in SeekEntry(other)
			pEntry->entry_name = new char[pEntry->name_len + 1];
			strcpy(pEntry->entry_name, szEntryName);

		}
	}


	//	PBLMI_CodePage l_TgtCp = m_iPbLMI->GetTargetCodePage();

   int entry_len;
   entry_len = pEntry->name_len + m_EntFixedLen +  (m_bUTF ? 2 : 1);
	//if (debug) wprintf(L"SeekEntry '%s', elen=%i\n", pEntry->entry_name, entry_len);

	if (entry_len > NodeSize - NodeEntOffset)
		// such entries cannot be stored in PBL
		return PBLMI_NOTFOUND;

	PBL_NOD nod;
	ReadData(&nod, sizeof(nod), dwNode);
	if (strncmp(nod.sign, "NOD*", 4))
		return PBLMI_BADFORMAT;
	//pf("left: 0x%08X  right: 0x%08X", nod.left, nod.right);


   //my ($first_name, $last_name);
	ByteBuffer first_name, last_name;

   if (nod.ent_count)
   {
		WORD li_nlen;
		ReadData(&li_nlen, sizeof(li_nlen), dwNode + nod.first_offset - 2);
		first_name.Read(m_Handle, li_nlen);
      if ( StrCmp((char*)first_name.buffer(),  pEntry->entry_name)> 0)
      {
			if (bCreate && !nod.left && nod.free_space < entry_len)
         {
            PBLMI_Result ret;
				ret = CreateNode(dwNode, nod.left);
				if (ret != PBLMI_OK)
					return ret;
				ret = WriteData(&nod.left, sizeof(nod.left), dwNode + NodeLeftOffset);
				if (ret != PBLMI_OK)
					return ret;
				return this->SeekEntry(NULL, pEntry, bCreate, nod.left);
         }

			if (nod.left)
				return this->SeekEntry(NULL, pEntry, bCreate, nod.left);
			if (!bCreate)
				return PBLMI_NOTFOUND;
         // add to current node (w/o check for space)

         return AddEntryToNode(pEntry, 1, 0, dwNode, &nod);

			//return PBLMI_NOTFOUND;
      }

		ReadData(&li_nlen, sizeof(li_nlen), dwNode + nod.last_offset - 2);
		last_name.Read(m_Handle, li_nlen);
      if ( StrCmp((char*)last_name.buffer(),  pEntry->entry_name) < 0)
      {
			if (bCreate && !nod.right && nod.free_space < entry_len)
         {
            PBLMI_Result ret;
				ret = CreateNode(dwNode, nod.right);
				if (ret != PBLMI_OK)
					return ret;
				ret = WriteData(&nod.right, sizeof(nod.right), dwNode + NodeRightOffset);
				if (ret != PBLMI_OK)
					return ret;
				return this->SeekEntry(NULL, pEntry, bCreate, nod.right);
         }
			if (nod.right)
				return this->SeekEntry(NULL, pEntry, bCreate, nod.right);
			if (!bCreate)
				return PBLMI_NOTFOUND;
         // add to current node (w/o check for space)
			return AddEntryToNode(pEntry, 0, 1, dwNode, &nod);
			//return PBLMI_NOTFOUND;
      }
   }
	// the entry is in this node
	//print "loop\n";
	ByteBuffer buf;
	buf.Read(
		m_Handle,
		NodeSize - NodeEntOffset, // - nod.free_space,
		dwNode + NodeEntOffset
	);

   //my @names;

   //@names = (1 .. $ent_count*2 + 2) if $create;
	Array<Ent> names(bCreate ? nod.ent_count + 1 : 0);
	PBL_ENT *pent = NULL;
	PBL_ENT_W *pent_W = NULL;
   for (int offset = 0, ent_num = 0; ent_num < nod.ent_count ; ent_num ++)
   {
      //my ($name_len = unpack "S", substr($buf, $offset + $EntFixedLen - 2, 2);
		if (m_bUTF) {
			pent_W = (PBL_ENT_W *) (buf.buffer() + offset);
		}
		else {
			pent = (PBL_ENT *) (buf.buffer() + offset);
		}

		//pent_W = (PBL_ENT_W *)pent;
		if (GetPF(pent, entry_name_len) == 0 ||
			 GetPF(pent, entry_name_len) > 16000) {
			//	 return undef if $name_len <= 0;
			return PBLMI_BADFORMAT;
		}
		char * cur_name = (char*)buf.buffer() + offset + m_EntFixedLen;
      if (!StrCmp(cur_name, pEntry->entry_name))
      {
			//if(m_bUTF)
			//	pent_W->comment_len *= 2;
			//pEntry->comment_len = GetPF(pent, comment_len);
			//pEntry->data_len = GetPF(pent, data_len);
			//pEntry->mod_time = GetPF(pent, creation_time.tm);
			//pEntry->data_offset = GetPF(pent, data_offset);
			UnpackEntry(pEntry, pent, pent_W);
			pEntry->node = dwNode;
			pEntry->entry_offset = dwNode + NodeEntOffset + offset;
			return PBLMI_OK;

      }
		int len = m_EntFixedLen + GetPF(pent, entry_name_len);
      if (bCreate)
      {
         names[ent_num].name = cur_name;
			names[ent_num].offset = offset;
			names[ent_num].len = len;
      }

      offset += len;
   }
	if (!bCreate)
		return PBLMI_NOTFOUND;


	//add entry
   if (nod.free_space >= entry_len)
   {
		return AddEntryToNode(pEntry, 0, 0, dwNode, &nod);
   }else
   {
//	  if (debug)
//		  debug = true;
      names[nod.ent_count].name = pEntry->entry_name;
      names[nod.ent_count].offset = -1; //потом анализируется
      names[nod.ent_count].len = entry_len;
      nod.free_space -= entry_len; //# здесь всегда д.б. < 0
      nod.ent_count ++;
/*
   добавить выбор прямой или обратной сортировки
   по среднему элементу (для равномерного разветвления дерева)

   в дальнейшем добавить равномерный разнос 2/3 (эспериментально установить
   оптимальное число) нода по его дочерним
   при вставке в полностью заполненный нод
 */
      DWORD nextnode;
      int dir = 1;
		// по очереди?
      if (dir == 1)
      {
			Ent::iPbl = this;
			qsort(&names[0], names.size(), sizeof(Ent),
				(int (__cdecl *)(const void *,const void *))
				& Ent::cmprev);
         if (!nod.right) {
				PBLMI_Result ret;
				ret = CreateNode(dwNode, nod.right);
				if (ret != PBLMI_OK)
					return ret;
			}
         nextnode = nod.right;
      }else
      {
			Ent::iPbl = this;
			qsort(&names[0], names.size(), sizeof(Ent), & Ent::cmp);
         if (!nod.left) {
				PBLMI_Result ret;
				ret = CreateNode(dwNode, nod.left);
				if (ret != PBLMI_OK)
					return ret;
			}
         nextnode = nod.left;
      }
      //my ($curentry, $retentry);
		//PBL_ENTRYINFO retentry;
		PBLMI_Result ret;
      for(int i = 0; i < names.size(); i ++) {
		  //wprintf(L"[%i] ", i);
			if (names[i].offset == -1 ) {
				//if (debug) wprintf(L"Moving new '%s' 0x%X->0x%X\n", pEntry->entry_name, dwNode, nextnode);
				ret = SeekEntry(NULL, pEntry, TRUE, nextnode);
				if (ret != PBLMI_OK)
					return ret;
			} else {
				pent = (PBL_ENT *) (buf.buffer() + names[i].offset);
				pent_W = (PBL_ENT_W *)pent;
				PBL_ENTRYINFO curentry;
				UnpackEntry(&curentry, pent, pent_W);
				curentry.entry_name = (char*) (buf.buffer() + names[i].offset + m_EntFixedLen);
				curentry.name_len = StrLen(curentry.entry_name) * (m_bUTF ? 2 : 1);
				//if (debug) wprintf(L"replace '%s' with '%s' 0x%X->0x%X\n", curentry.entry_name, pEntry->entry_name, dwNode, nextnode);
				ret = SeekEntry(NULL, &curentry, TRUE, nextnode);
				if (ret != PBLMI_OK)
					return ret;
			}
	     //FIX is here
         //WAS: nod.free_space += StrLen(names[i].name) + m_EntFixedLen + (m_bUTF?2:1);
		 nod.free_space += StrLen(names[i].name) * (m_bUTF?2:1) + m_EntFixedLen + (m_bUTF?2:1);
         names[i].deleted = TRUE;
         nod.ent_count --;
		 //if (debug) wprintf(L"free=%i(+%i) count=%i(-1)\n", nod.free_space, StrLen(names[i].name) + m_EntFixedLen + (m_bUTF?2:1), nod.ent_count);

         if(nod.free_space >= 0)
				break;
      }
      //# записываем измененный нод на диск
      nod.last_offset = 0;
      nod.first_offset = 0;
      fseek(m_Handle, dwNode + NodeEntOffset, SEEK_SET);

      for (int ei = 0; ei < names.size(); ei ++) {
			int i = (dir == 1 ? names.size() - ei - 1 : ei);
			if(names[i].deleted)
				continue;
			long offset = ftell(m_Handle);
         nod.last_offset = (INT16)(offset - dwNode + m_EntFixedLen);
         if (!nod.first_offset)
				nod.first_offset = nod.last_offset ;
         if (names[i].offset == -1)
         {
				pEntry->node = dwNode;
				pEntry->entry_offset = offset;
				names[i].offset = buf.size();
				buf.Extend(entry_len);
				pent = (PBL_ENT *) (buf.buffer() + names[i].offset);
				pent_W = (PBL_ENT_W *) pent;
				PackEntry(pEntry, pent, pent_W);
				memcpy(
					buf.buffer() + names[i].offset + m_EntFixedLen,
					pEntry->entry_name,
					pEntry->name_len + (m_bUTF?2:0)
				);
         }

			ret = WriteData(buf.buffer() + names[i].offset, names[i].len);
         if(ret != PBLMI_OK)
				return ret;
      }
      if (nod.free_space > 0)
      {
			BYTE b = 0;
			if(fwrite(&b, sizeof(b), nod.free_space, m_Handle) != (DWORD)nod.free_space)
				return PBLMI_DISKERROR;
      }

      //заголовок нода
		ret = WriteData(&nod, sizeof(nod), dwNode);
		if(ret != PBLMI_OK)
			return ret;

      return PBLMI_OK;
   }

	return PBLMI_ERROR;
}

PBLMI_Result
PBLMI_PBL::DeleteEntry(PBL_ENTRYINFO *pEntry)
{
	static int num = 0;
	//FILE * logout = fopen("pblmi.log", "a+t");
	//fprintf(logout, ": (%i)%08X\n", ++num , pEntry->entry_offset);
	PBLMI_Result ret;
	if (!pEntry || !pEntry->node || !pEntry->entry_offset)
		return PBLMI_BADARG;
	PBL_NOD nod;
	ReadData(&nod, sizeof(nod), pEntry->node);
	if (strncmp(nod.sign, "NOD*", 4))
		return PBLMI_BADFORMAT;
	ByteBuffer buf;
	buf.Read(
		m_Handle,
		NodeDataSize, // - nod.free_space,
		pEntry->node + NodeEntOffset
	);

	PBL_ENT *pent;
	PBL_ENT_W *pent_W;
	int entOffset = (pEntry->entry_offset - pEntry->node - NodeEntOffset );
	int nameOffset = entOffset + m_EntFixedLen;
	pent = (PBL_ENT *) (buf.buffer() + entOffset);
	pent_W = (PBL_ENT_W *) pent;
	int entLen = m_EntFixedLen + GetPF(pent, entry_name_len);
	memmove(
		buf.buffer() + entOffset,
		buf.buffer() + entOffset + entLen,
		NodeDataSize - entOffset - entLen
	);
	memset(buf.buffer() + NodeDataSize - entLen, 0, entLen);
	nod.ent_count --;
	nod.free_space += entLen;

	BYTE * entName, *firstName = NULL, *lastName = NULL;
	nod.first_offset = 0;
	nod.last_offset = 0;
	int offset = 0;
	for (int i = 0; i < nod.ent_count; i ++) {
		pent = (PBL_ENT *) (buf.buffer() + offset);
		pent_W = (PBL_ENT_W *) pent;
		entName = buf.buffer() + offset + m_EntFixedLen;
		if (firstName && StrCmp(entName, firstName) > 0 ) {
			// do nothing
		}
		else {
			firstName = entName;
			nod.first_offset = NodeEntOffset + offset + m_EntFixedLen;
		}
		if (lastName && StrCmp(entName, lastName) < 0) {
			// do nothing
		}
		else {
			lastName = entName;
			nod.last_offset = NodeEntOffset + offset + m_EntFixedLen;
		}
		offset += m_EntFixedLen + GetPF(pent, entry_name_len);
	}
	if (nod.ent_count == 0 && (nod.parent || nod.left || nod.right)) {
		// delete empty node
		// also if this is the root node we must move here
		// some other node

		//if (pEntry->node == 0x00049000) {
		//	printf("!\n");
		//}
		//!!!debug
		//Flush();
		//fprintf(logout, "before delete node\n");
		//_print_node(logout, 0, m_RootNodeOffset);

		//fprintf(logout, "!!!Delete empty node: %08X\n", pEntry->node);
		DWORD nodeForDel = pEntry->node;
		DWORD newNode = 0;
		if (nod.right) {
			newNode = nod.right;
			//fprintf(logout, "!!!newNode: right: %08X\n", newNode);
			if (nod.left) {
				//fclose(logout); ///!!!
				ret = MoveDownNode(nod.left, newNode);
				//logout = fopen("pblmi.log", "a+t"); //!!!
				if (ret != PBLMI_OK)
					return ret;
			}
		}
		else if (nod.left) {
			newNode = nod.left;
			//fprintf(logout, "!!!newNode: left: %08X\n", newNode);
		}
		else {
			//fprintf(logout, "!!!no children\n");
		}
		PBL_NOD newNod;
		if (newNode) {
			ret = ReadData(&newNod, sizeof(newNod), newNode);
			if (ret != PBLMI_OK)
				return ret;
			newNod.parent = nod.parent;
			ret = WriteData(&newNod, sizeof(newNod), newNode);
			if (ret != PBLMI_OK)
				return ret;
		}
		if (nod.parent) {
			PBL_NOD parentNod;
			ret = ReadData(&parentNod, sizeof(parentNod), nod.parent);
			if (ret != PBLMI_OK)
				return ret;
			if (strcmp(parentNod.sign, "NOD*"))
				return PBLMI_BADFORMAT;
			if (parentNod.left == pEntry->node)
				parentNod.left = newNode; // can be 0
			else if (parentNod.right == pEntry->node)
				parentNod.right = newNode;
			else
				return PBLMI_BADFORMAT;

			ret = WriteData(&parentNod, sizeof(parentNod), nod.parent);
			if (ret != PBLMI_OK)
				return ret;
		}
		else {
			// This is the root node: we must not delete it
			// instead we relocate here our newNod.

			if (newNode) {
				//fprintf(logout, "!!!Relocating node: %08X->%08X\n", newNode, pEntry->node);
				// copy entry data
				ByteBuffer data(NodeDataSize);
				ret = ReadData(data.buffer(), NodeDataSize, newNode + NodeEntOffset);
				if (ret != PBLMI_OK)
					return ret;
				ret = WriteData(data.buffer(), NodeDataSize, pEntry->node + NodeEntOffset);
				if (ret != PBLMI_OK)
					return ret;

				// update references to the node
				if (newNod.left)	{
					PBL_NOD child;
					ret = ReadData(&child, sizeof(child), newNod.left);
					if (ret != PBLMI_OK)
						return ret;
					//fprintf(logout, "!!!  Changing left (%08X) child parent: %08X->%08X\n",
					//	newNod.left, child.parent, pEntry->node);
					child.parent = pEntry->node;
					ret = WriteData(&child, sizeof(child), newNod.left);
					if (ret != PBLMI_OK)
						return ret;
				}
				if (newNod.right)	{
					PBL_NOD child;
					ret = ReadData(&child, sizeof(child), newNod.right);
					if (ret != PBLMI_OK)
						return ret;
					//fprintf(logout, "!!!  Changing right (%08X) child parent: %08X->%08X\n",
					//	newNod.right, child.parent, pEntry->node);
					child.parent = pEntry->node;
					ret = WriteData(&child, sizeof(child), newNod.right);
					if (ret != PBLMI_OK)
						return ret;
				}
				nodeForDel = newNode;
				newNode = pEntry->node;

				ret = WriteData(&newNod, sizeof(newNod), newNode);
				if (ret != PBLMI_OK)
					return ret;
			}
		}

		if (nodeForDel) {
			BlockList delNodes(NodeSize / BlockSize);
			for (int i = 0; i < NodeSize / BlockSize; i ++)
				delNodes[i] = nodeForDel + i * BlockSize;
			ret = FreeBlocks(delNodes);
			if (ret != PBLMI_OK)
				return ret;
		}

		// debug !!!
		//fflush(stdout);
		//!!!debug
		//Flush();
		//fprintf(logout, "after delete node\n");
		//_print_node(logout, 0, m_RootNodeOffset);
	}
	else /*!(nod.ent_count == 0 && (nod.parent || nod.left || nod.right))*/{

		//if (!nod.ent_count)
		//	fprintf(logout, "!!! Node is not deleted: %08X\n", pEntry->node);
		ret = WriteData(&nod, sizeof(nod), pEntry->node);
		if (ret != PBLMI_OK)
			return ret;
		ret = WriteData(buf.buffer(), buf.size(), pEntry->node + NodeEntOffset);
		if (ret != PBLMI_OK)
			return ret;
	}

	if (pEntry->data_offset) {
		BlockList data;
		ret = ListBlocks(pEntry->data_offset, data);
		if (ret != PBLMI_OK)
			return ret;
		ret = FreeBlocks(data);
		if (ret != PBLMI_OK)
			return ret;
	}
	//fclose(logout);

	return PBLMI_OK;
}

PBLMI_Result
PBLMI_PBL::MoveDownNode(DWORD aNode, DWORD aNewParent)
{
	/*
	 this method places one of the child nodes of the deleted node
	 to the correct new parent node
	 it is called if the deleted node has both children;
  	 the outermost call to the method must pass the left
	 subnode of the deleted node as aNode and the right subnode
	 as aNewParent;
	 in this implementation we move nodes always
	 down to the left branch; this behavior corresponds to the
	 full node refilling strategy (see SeekEntry, int dir = 1;);
	*/
	//FILE * logout = fopen("pblmi.log", "a+t");

	PBL_NOD parent;
	PBLMI_Result ret;
	ret = ReadData(&parent, sizeof(parent), aNewParent);
	if (ret != PBLMI_OK)
		return ret;
	if (parent.left)
		return MoveDownNode(aNode, parent.left);

	PBL_NOD nod;
	ret = ReadData(&nod, sizeof(nod), aNode);
	if (ret != PBLMI_OK)
		return ret;
	//debug!
	//fprintf(logout,
	//	"\n!!!MoveDownNode:\n"
	//	"aNode: %08X\n"
	//	"aNewParent: %08X\n"
	//	,
	//	aNode,
	//	aNewParent
	//);

	parent.left = aNode;
	nod.parent = aNewParent;
	ret = WriteData(&parent, sizeof(parent), aNewParent);
	if (ret != PBLMI_OK)
		return ret;
	ret = WriteData(&nod, sizeof(nod), aNode);
	if (ret != PBLMI_OK)
		return ret;
	//fclose(logout);
	return PBLMI_OK;
}

PBLMI_Result
PBLMI_PBL::ReadEntryData(PBL_ENTRYINFO *pEntry, void * p)
{
	char * pBuf = (char*)p;
	if (!pEntry)
		return PBLMI_ERROR;
	//pf("len: %lu\n", pEntry->data_len);
	//pf("commlen: %lu\n", pEntry->comment_len);
	//pf("len: %lu\n", pEntry->data_len);
	PBL_DAT_HDR hdr;
	DWORD curr = 0;
//	WORD skip = m_bUTF ? pEntry->comment_len * 2 : pEntry->comment_len;
	hdr.next = pEntry->data_offset;
	while(hdr.next) {
		fseek(m_Handle, hdr.next, SEEK_SET);
		fread(&hdr, sizeof(hdr), 1, m_Handle);
		if (strncmp(hdr.sign, "DAT*", 4))
			return PBLMI_BADFORMAT;
/*		if (skip) {
			fseek(m_Handle, skip, SEEK_CUR);
			hdr.datalen -= skip;
			skip = 0;
		}
*/
		//pf("readlen: %lu\n", hdr.datalen);
		fread(pBuf + curr, hdr.datalen, 1, m_Handle);
		curr += hdr.datalen;
	}
	//pf("read total: %lu\n", curr);

	return PBLMI_OK;
}

PBLMI_Result
PBLMI_PBL::ListBlocks(DWORD aFirstOffset, BlockList &aList)
{
	PBL_DAT_HDR hdr;
	aList.resize(0);
	hdr.next = aFirstOffset;
	while(hdr.next) {
		int i = aList.size();
		aList.resize(i + 1);
		aList[i] = hdr.next;
		fseek(m_Handle, hdr.next, SEEK_SET);
		fread(&hdr, sizeof(hdr), 1, m_Handle);
		if (strncmp(hdr.sign, "DAT*", 4))
			return PBLMI_BADFORMAT;
	}

	return PBLMI_OK;
}

BOOL PBLMI_PBL::isUnicode()
{
	return m_bUTF;
}

PBLMI_Result
PBLMI_PBL::SetEntryTime(PBL_ENTRYINFO *pEntry)
{
	long offset = pEntry->entry_offset;
	offset += (isUnicode() ? sizeof(PBL_ENT_W) : sizeof(PBL_ENT)) - 8;
	fseek(m_Handle, offset, SEEK_SET);
	int ret = fwrite(& pEntry->mod_time, sizeof(pEntry->mod_time), 1, m_Handle);
	//pf("ret: %i", ret);
	//pf("time: %u", pEntry->mod_time);
	return ret == 1 ? PBLMI_OK : PBLMI_ERROR;
}

PBLMI_Result
PBLMI_PBL::UpdateEntryData
	(
		PBL_ENTRYINFO *pEntry,
		const void *pBuf, INT32 dwDataLen, INT16 wComLen
	)
{
	/*
	  for PB10, comments part must be in unicode but wComLen must be
	  in bytes and multiple of 2
	*/
	if (m_bUTF && (wComLen % 2 != 0))
		return PBLMI_BADARG;
	PBLMI_Result ret;

	INT32 bufLen = dwDataLen + wComLen;
	ret = UpdateBlocksData(&pEntry->data_offset, pBuf, bufLen, TRUE);
	if (ret != PBLMI_OK)
		return ret;

	pEntry->comment_len = wComLen;
	pEntry->data_len = pEntry->comment_len + dwDataLen;
	PBL_ENT_U u;
	ret = ReadData(&u, m_EntFixedLen, pEntry->entry_offset);
	if (ret != PBLMI_OK)
		return ret;
	SetF(u.ent, data_offset, pEntry->data_offset);
	SetF(u.ent, data_len, pEntry->data_len);
	SetF(u.ent, comment_len, pEntry->comment_len / ( m_bUTF ? 2 : 1));
	SetF(u.ent, creation_time.tm,
		pEntry->mod_time ? pEntry->mod_time :time(NULL));
	ret = WriteData(&u, m_EntFixedLen, pEntry->entry_offset);
	if (ret != PBLMI_OK)
		return ret;

	return PBLMI_OK;
}


PBLMI_PBL::PBLMI_PBL(PBLMI *aPbLMI, const char *szLibName, BOOL bReadWrite)
{
	m_bIsPBL = FALSE;
	m_Handle = NULL;
	m_bUTF = FALSE;
	m_iPbLMI = aPbLMI;
	m_WriteMode = bReadWrite;
	m_Handle = fopen(szLibName, bReadWrite ? "rb+" : "rb");
	Ent::iPbl = this;
	if (!m_Handle)
		return;
	char buf[4];
	fseek(m_Handle, 0, SEEK_SET);
	if (!fread(buf, 4, 1, m_Handle))
		return;
	if (!strncmp(buf, "HDR*", 4))
	{
		m_StartOffset = 0;
	}else
	{
		fseek(m_Handle, -0x200, SEEK_END);
		if (!fread(buf, 4, 1, m_Handle))
			return;
		if (!strncmp(buf, "TRL*", 4))
		{
			if (!fread(&m_StartOffset, sizeof(m_StartOffset), 1, m_Handle))
				return;
			fseek(m_Handle, m_StartOffset, SEEK_SET);
			if (!fread(buf, 4, 1, m_Handle))
				return;
			if (strncmp(buf, "HDR*", 4))
				return;
		}else
			return;
	}
	fseek(m_Handle, m_StartOffset + 0x04, SEEK_SET);
	if (!fread(buf, 4, 1, m_Handle))
		return;
	if (*(DWORD*)buf == 0x006f0050)
		m_bUTF = TRUE; // PB10+
	else if (*(DWORD*)buf == 0x65776f50)
		m_bUTF = FALSE; // PB9-
	else
		return;
	m_bIsPBL = TRUE;
	m_RootNodeOffset = m_bUTF ? 0x600 : 0x400;
	m_FreeListFirstOffset = m_bUTF ? 0x400 : 0x200;
	m_FreeListRead = FALSE;
	m_EntFixedLen = Get1(EntFixedLen);
	m_CharSize = m_bUTF ? 2 : 1;
	if (m_bUTF)
		m_PbVer = 10;
	else {
		fseek(m_Handle, m_StartOffset + 0x12, SEEK_SET);
		fread(buf, 4, 1, m_Handle);
		m_PbVer = buf[1] - '0';
	}
}

PBLMI_PBL::~PBLMI_PBL()
{
	if (m_Handle) {
		Flush();
		fclose(m_Handle);
	}
}
void
PBLMI_PBL::Close(void)
{
	Flush();
	delete this;
}


PBLMI_Result PBLMI_PBL::Dir(IPBLMI_Callback *iCallBack)
{
	return ReadNodeEntries(m_StartOffset + m_RootNodeOffset, iCallBack);
}


PBLMI_Result PBLMI_PBL::ReadNodeEntries
(
	DWORD dwNode,
	IPBLMI_Callback *iCallBack
)
{
	if (!dwNode)
		return  PBLMI_OK;
	//!!!debug
	//pf("node: %08X", dwNode);
	static int n = 0;
	//p("0");
	PBL_NOD nod;
	PBL_ENT *pent;
	PBL_ENT_W *pent_W;
	PBL_ENTRYINFO ei;
	fseek(m_Handle, dwNode, SEEK_SET);
	//p("1");
	if (!fread(&nod, sizeof(nod), 1, m_Handle))
		return PBLMI_BADFORMAT;
	//p("2");
	//printf("node: %08X  L=%08X  R=%08X\n", dwNode, nod.left, nod.right);

	ByteBuffer data(NodeSize - sizeof(PBL_NOD));
	fseek(m_Handle, dwNode + NodeEntOffset, SEEK_SET);
	if (!fread(data.buffer(), NodeSize - sizeof(PBL_NOD), 1, m_Handle))
		return PBLMI_BADFORMAT;
	//p("3");

	int ent_offset = 0;
	int li_ent;
	for(li_ent = 0;li_ent < nod.ent_count; li_ent ++ )
	{
		n ++;
		//pf("%i", n);
		pent = (PBL_ENT*) (data.buffer() + ent_offset);
		pent_W = (PBL_ENT_W*) pent;
		if(strncmp(pent->sign, "ENT*", 4))
			return PBLMI_BADFORMAT;
		//p("5");
		ei.entry_name = m_iPbLMI->ConvCodePage(
			(char *) (data.buffer() + ent_offset + m_EntFixedLen),
			GetPF(pent,entry_name_len),
			m_bUTF
		);
		if(m_bUTF)
			pent_W->comment_len *= 2;
		ei.comment_len = GetPF(pent, comment_len);
		ei.data_len = GetPF(pent, data_len);
		ei.data_offset = GetPF(pent, data_offset);
		ei.node = dwNode;
		ei.mod_time = GetPF(pent, creation_time.tm);
		ei.entry_offset = dwNode + NodeEntOffset + ent_offset;
		ei.entry_len = m_EntFixedLen + GetPF(pent,entry_name_len);
		//printf("entry len = %i\n", ei.entry_len);
		BOOL ret = iCallBack->DirCallback(&ei);
		if (ei.entry_name)
			delete [] ei.entry_name;
		if (!ret)
			return PBLMI_ABORT;
		ent_offset += m_EntFixedLen + GetPF(pent,entry_name_len);
	}
	//p("6");
	data.FreeBuffer();
	//p("7");
	PBLMI_Result ret;
	ret = ReadNodeEntries(nod.left,  iCallBack);
	if( ret != PBLMI_OK )
		return ret;
	ret = ReadNodeEntries(nod.right, iCallBack);
	if( ret != PBLMI_OK )
		return ret;

	return PBLMI_OK;
}




PBLMI::PBLMI() {
	m_TgtCp = PBLMI_ANSI;
//	pf("%i\n", sizeof(PBL_HDR_W));
//	pf("%i\n", sizeof(PBL_HDR));
}
// create library
PBLMI_Result
PBLMI::CreateLibrary(const char *szLib, int aPbVer)
{
	FILE * f = fopen(szLib, "w+b");
	if (!f)
		return PBLMI_ACCESS_DENIED;
	int retn;
	PBL_HDR_U u;
	BOOL m_bUTF = aPbVer >= 10;
	if (aPbVer < 5)
		return PBLMI_BADARG;
	else if (aPbVer < 10) {
		strcpy(u.hdr.name, "PowerBuilder");
		memcpy(u.hdr.major_ver, (aPbVer == 5?"0500":"0600"), 4);
	}
	else {
		wcscpy((wchar_t*)u.hdr_W.name, L"PowerBuilder");
		memcpy(u.hdr_W.major_ver, L"0600", 8);
	}
	memcpy(u.hdr.sign, "HDR*", 4);
	SetF(u.hdr, creation_time.tm, time(NULL));
	SetF(u.hdr, sc_data_offset, 0);
	SetF(u.hdr, sc_data_length, 0);
	memset(GetF(u.hdr, comments), 0, SizeOfF(u.hdr, comments));
	SetF(u.hdr, reserved1, 0);
	if (m_bUTF) {
		SetF(u.hdr, reserved2, 1);
	}
	else {
		SetF(u.hdr, reserved2, 0);
	}
	retn = fwrite(&u, SizeOf(u.hdr), 1, f);
	if(retn != 1) {
		fclose(f);
		return PBLMI_DISKERROR;
	}
	long offset;
	offset = m_bUTF ? 0x400 : 0x200;
	PBL_FRE fre;
	memcpy(fre.sign, "FRE*", 4);
	fre.next = 0;
	memset(fre.data, 0, sizeof(fre.data));
	fre.data[0] =0xFF;
	if (m_bUTF)
		fre.data[1] =0x80;
	fseek(f, offset, SEEK_SET);
	retn = fwrite(&fre, sizeof(fre), 1, f);
	if(retn != 1) {
		fclose(f);
		return PBLMI_DISKERROR;
	}

	PBL_NOD nod;
	memset(&nod, 0, sizeof(nod));
	memcpy(nod.sign, "NOD*", 4);
	nod.free_space = NodeDataSize;
	retn = fwrite(&nod, sizeof(nod), 1, f);
	if(retn != 1) {
		fclose(f);
		return PBLMI_DISKERROR;
	}
	ByteBuffer buf(NodeDataSize);
	memset(buf.buffer(), 0, NodeDataSize);
	retn = fwrite(buf.buffer(), NodeDataSize, 1, f);
	if(retn != 1) {
		fclose(f);
		return PBLMI_DISKERROR;
	}

	fclose(f);
	return PBLMI_OK;
}

// open library and return IPBLMI_PBL interface to misc. LMI functions
IPBLMI_PBL*
PBLMI::OpenLibrary(const char *szLib, BOOL bReadWrite)
{
	PBLMI_PBL *iPbl = new PBLMI_PBL(this, szLib, bReadWrite);
	if (!iPbl->m_bIsPBL) {
		delete iPbl;
		return NULL;
	}
	return (IPBLMI_PBL*)iPbl;
}



void
PBLMI::Release(void)
{
	delete this;
}

// set output encoding for strings
PBLMI_Result
PBLMI::SetTargetCodePage(PBLMI_CodePage cp){
	m_TgtCp = cp;
	return PBLMI_OK;
}
// get output encoding
PBLMI_CodePage
PBLMI::GetTargetCodePage() {
	return m_TgtCp;
}

char *
PBLMI::ConvCodePage(const char * as_Src,
						  int ai_SrcLen,
						  BOOL ab_SrcIsWide) {
	int li_Len = ai_SrcLen;
	UINT li_SrcCp = CP_ACP;
	UINT li_TgtCp = MapToCP(m_TgtCp);
	char * ls_Tgt;
	int li_NewLen = 0;
	BOOL lb_UsedDefChar = FALSE;
	if (ab_SrcIsWide && m_TgtCp != PBLMI_WIDE) {
		ls_Tgt = WideToMulti(as_Src, li_Len, li_TgtCp);
	}else if (!ab_SrcIsWide && m_TgtCp == PBLMI_WIDE) {
		ls_Tgt = MultiToWide(as_Src, li_Len, li_SrcCp);
	}else if (!ab_SrcIsWide && li_SrcCp != li_TgtCp) {
		char *ls_tmp = MultiToWide(as_Src, li_Len, li_SrcCp);
		ls_Tgt = WideToMulti(ls_tmp, li_Len*2, li_TgtCp);
		delete [] ls_tmp;
	}else {
		ls_Tgt = new char[li_Len + 1];
		strncpy(ls_Tgt, as_Src, li_Len);
	}
	return ls_Tgt;
}
UINT MapToCP(PBLMI_CodePage cp) {
	switch (cp){
	case PBLMI_UTF8:
		return CP_UTF8;
	case PBLMI_OEM:
		return CP_OEMCP;
	case PBLMI_ANSI:
		return CP_ACP;
	default:
		return 0;
	};
}
char * MultiToWide(const char * as_Src, int ai_Len, UINT ai_Cp) {
	int li_NewLen = MultiByteToWideChar(
		ai_Cp,
		MB_PRECOMPOSED,
		(LPCSTR) as_Src,
		ai_Len , // incl 0
		NULL,
		0 // return buf size in wchar's
	);
	char *ls_Tgt = new char[li_NewLen * 2 + 2];
	int ret = MultiByteToWideChar(
		ai_Cp,
		MB_PRECOMPOSED,
		(LPCSTR) as_Src,
		ai_Len, // incl 0
		(LPWSTR)ls_Tgt,
		li_NewLen
	);
	if (ret != li_NewLen) {
		PrintLastError();

	}
	ls_Tgt[li_NewLen * 2 + 0] = '\0';
	ls_Tgt[li_NewLen * 2 + 1] = '\0';
	return ls_Tgt;

}
char * WideToMulti(const char * as_Src, int ai_Len, UINT ai_Cp) {
	int li_NewLen = WideCharToMultiByte(
		ai_Cp,
		0, //WC_DEFAULTCHAR,
		(LPCWSTR) as_Src,
		ai_Len / 2, // incl 0
		NULL,
		0, // return buf size in bytes
		NULL, //"?", // def char
		NULL // &lb_UsedDefChar
	);

	if (li_NewLen == 0) {
		PrintLastError();
		//exit(1);
		return "";
	}
	char *ls_Tgt = new char[li_NewLen];
	WideCharToMultiByte(
		ai_Cp,
		0, //WC_DEFAULTCHAR,
		(LPCWSTR) as_Src,
		ai_Len / 2, // incl 0
		(LPSTR)ls_Tgt,
		li_NewLen,
		NULL, //"?", // def char
		NULL  //&lb_UsedDefChar
	);
	return ls_Tgt;
}
// global IPBLMI generator
extern "C"
//__declspec(dllexport)
IPBLMI*
PBLMI_GetInterface()
{
   return new PBLMI();
}


void PrintLastError() {
	char errs[80];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		0,
		(LPTSTR)errs,
		sizeof(errs),
		NULL
	);
	printf("LastError: '%s'\n", errs);
}



PBLMI_Result PBLMI_PBL::ReadFreeListData()
{
	if(m_FreeListRead)
		return PBLMI_OK;

	PBL_FRE fre;
	// count FRE blocks
	int fre_count = 0;
	for (fre.next = m_FreeListFirstOffset; fre.next; ) {
		fseek(m_Handle, fre.next + sizeof(fre.sign), SEEK_SET);
		fread(&fre.next, sizeof(fre.next), 1, m_Handle);
		fre_count ++;
	}
	// allocate buffers
	m_FreeListData.FreeBuffer();
	m_FreeListData.Extend(FreeBlockBitSize * fre_count);
	m_FreeList.resize(fre_count);
	// read FRE data
	int fre_index = 0;
	for (fre.next = m_FreeListFirstOffset; fre.next; ) {
		m_FreeList[fre_index].changed = 0;
		m_FreeList[fre_index].offset = fre.next;
		fseek(m_Handle, fre.next, SEEK_SET);
		fread(&fre, sizeof(fre), 1, m_Handle);

		int bit_index = 0;
		BYTE * buf = m_FreeListData.buffer() + FreeBlockBitSize * fre_index;
		for (int byte_index = 0; byte_index < sizeof(fre.data); byte_index ++) {
			for (BYTE mask = 0x80; mask; mask >>= 1) {
				buf[bit_index] = (fre.data[byte_index] & mask) ? 1 : 0;
				bit_index ++;
			}
		}
		fre_index ++;
	}
	m_FreeListRead = TRUE;
	return PBLMI_OK;
}

PBLMI_Result PBLMI_PBL::AllocateFreelistBlocks(int ai_Count)
{
	if(!m_WriteMode)
		return PBLMI_ACCESS_DENIED;
	if(ReadFreeListData() != PBLMI_OK)
		return PBLMI_ERROR;

	/*
    usually we don't create more than 1 freelist block at once.
    this may happen if we need to allocate 2064384 or more bytes
    e.g. large BMPs as resources
	*/
	int li_CurBlocks = m_FreeList.size();
	// allocate additional buffers
	m_FreeList.resize(li_CurBlocks + ai_Count);
	m_FreeListData.Extend(ai_Count * FreeBlockBitSize, 0);

	// mark the last block as modified because its "next" field is changed
	if (li_CurBlocks > 0)
		m_FreeList[li_CurBlocks - 1].changed = TRUE;
	for (int li_Block = 0; li_Block < ai_Count; li_Block ++) {
		// calculate the offset on disk and mark buffer for writing
		m_FreeList[li_CurBlocks + li_Block].changed = TRUE;
		m_FreeList[li_CurBlocks + li_Block].offset =
			(li_CurBlocks + li_Block) * BlockSize * FreeBlockBitSize;
		// The first bit in the freelist block marks the block itself
		// This is not applicable to the first free list block
		// whose first bits mark PBL's header
		// But we never allocate the first block in AllocateFreelistBlocks()
		m_FreeListData[(li_CurBlocks + li_Block) * FreeBlockBitSize] = 1;
	}
	/*
	  we do not write to disk here.
	  this is done by WriteFreeListData
	*/
	return PBLMI_OK;
}

PBLMI_Result PBLMI_PBL::WriteFreeListData()
{
	if (!m_FreeListRead)
		return PBLMI_OK;
	PBL_FRE fre;
	for (int li_Block = 0; li_Block < m_FreeList.size(); li_Block ++) {
		// skip unchanged blocks
		if (!m_FreeList[li_Block].changed)
			continue;
		// fill PBL_FRE structure for writing
		if (li_Block + 1 < m_FreeList.size())
			fre.next = m_FreeList[li_Block + 1].offset;
		else
			fre.next = 0;
		memcpy(fre.sign, "FRE*", 4);
		// bitwise pack block data
		BYTE * buf = m_FreeListData.buffer() + li_Block * FreeBlockBitSize;
		int bit_index = 0;
		for (int byte_index = 0; byte_index < sizeof(fre.data); byte_index ++) {
			for (BYTE mask = 0x80; mask; mask >>= 1) {
				if (buf[bit_index])
					fre.data[byte_index] |= mask;
				else
					fre.data[byte_index] &= ~mask;
				bit_index ++;
			}
		}
		fseek(m_Handle, m_FreeList[li_Block].offset, SEEK_SET);
		int res = fwrite(&fre, sizeof(fre), 1, m_Handle);
		if (res < 1)
			return PBLMI_DISKERROR;
		m_FreeList[li_Block].changed = FALSE;

	}
	return PBLMI_OK;
}

PBLMI_Result PBLMI_PBL::AllocateBlocks(int ai_Count, BlockList &aList, BOOL ab_Continuous)
{
	if(!m_WriteMode)
		return PBLMI_ACCESS_DENIED;
	if(ReadFreeListData() != PBLMI_OK)
		return PBLMI_ERROR;

	int li_OldSize = aList.size();
	aList.resize(li_OldSize + ai_Count);
	BYTE * p;
	int li_BlockIndex = 0;
	int li_AllocatedCount = 0;
	while (li_AllocatedCount < ai_Count) {
		p = (BYTE *) memchr(m_FreeListData.buffer() +  li_BlockIndex,
			0, m_FreeListData.size() - li_BlockIndex);
		if (!p) {
			// there is no more space in free list

			// this will be the beginning of the new FRE block
			li_BlockIndex = m_FreeListData.size();

			// allocate a new free list block
			PBLMI_Result ret = AllocateFreelistBlocks(1);
			if (ret != PBLMI_OK)
				return ret;

			continue; // start searching from the new free list block
		}
		li_BlockIndex = p - m_FreeListData.buffer();
		if (ab_Continuous && (li_AllocatedCount == 0)) {
			if (li_BlockIndex + ai_Count > m_FreeListData.size()) {
				// the current free list has no ai_Count continuous blocks
				// we force allocation of a new free list block
				li_BlockIndex = m_FreeListData.size();
				continue;
			}
			if (memchr(p + 1, 1, ai_Count - 1)) {
				// some of our ai_Count blocks are taken
				// we skip them all
				li_BlockIndex += ai_Count;
				continue;
			}
			// here we have ai_Count continuous blocks
			// starting at li_BlockIndex
		}
		MarkBlock(li_BlockIndex, 1);
		aList[li_OldSize + li_AllocatedCount] = DWORD(BlockSize * li_BlockIndex);
		li_AllocatedCount ++;
		li_BlockIndex ++; // start searching from the next block
	}
	return PBLMI_OK;
}


PBLMI_Result PBLMI_PBL::FreeBlocks(BlockList &aList, int ai_StartIndex, int ai_Count)
{
	if(!m_WriteMode)
		return PBLMI_ACCESS_DENIED;
	if(ReadFreeListData() != PBLMI_OK)
		return PBLMI_ERROR;
	if(ai_Count < 0)
		ai_Count = aList.size() - ai_StartIndex;
	for (int b = ai_StartIndex; ai_Count > 0; b ++, ai_Count --) {
		int li_BlockIndex = aList[b] / BlockSize;
		MarkBlock(li_BlockIndex, 0);
	}
	return PBLMI_OK;
}

PBLMI_Result PBLMI_PBL::CreateNode(DWORD adw_ParentNode, DWORD &adw_NewNode)
{
	BlockList bb;
	PBLMI_Result ret;
	ret = AllocateBlocks(NodeSize / BlockSize, bb, TRUE);
	if (ret != PBLMI_OK)
		return ret;
	adw_NewNode = bb[0];
	//printf("Create Node 0x%08X\n",  adw_NewNode);
	PBL_NOD nod;
	memset(&nod, 0, sizeof(nod));
	memcpy(nod.sign, "NOD*", 4);
	nod.parent = adw_ParentNode;
	nod.free_space = NodeSize - sizeof(nod);
	ret = WriteData(&nod, sizeof(nod), adw_NewNode);
	if (ret != PBLMI_OK)
		return ret;
	ret = WriteData(const_cast<char*>("\0"), 1, adw_NewNode + NodeSize - 1);
	if (ret != PBLMI_OK)
		return ret;
	return PBLMI_OK;
}

PBLMI_Result PBLMI_PBL::WriteData(void * ap_Buf, int ai_Size, long ai_Offset)
{
	int ret;
	if (ai_Offset >= 0) {
		ret = fseek(m_Handle, ai_Offset, SEEK_SET);
	}
	ret = fwrite(ap_Buf, ai_Size, 1, m_Handle);
	return ret == 1 ? PBLMI_OK : PBLMI_ACCESS_DENIED;
}

PBLMI_Result PBLMI_PBL::ReadData(void * ap_Buf, int ai_Size, long ai_Offset)
{
	if (ai_Offset >= 0) {
		fseek(m_Handle, ai_Offset, SEEK_SET);
	}
	int ret = fread(ap_Buf, ai_Size, 1, m_Handle);
	return ret == 1 ? PBLMI_OK : PBLMI_ACCESS_DENIED;
}

/*
	Adds the entry to the node assuming the nod has enough free space

*/

PBLMI_Result PBLMI_PBL::AddEntryToNode(
				PBL_ENTRYINFO *pEntry,
				BOOL ab_ToFirst,
				BOOL ab_ToLast,
				DWORD adw_Node,
				PBL_NOD * pNod
         )
{
	PBL_ENT_U u;
	PBL_NOD __nod; // do not access directly: use pNod
	PBLMI_Result ret;
	if (!pNod) {
		ret = ReadData(&__nod, sizeof(__nod), adw_Node);
		if (ret != PBLMI_OK)
			return ret;
		pNod = &__nod;
	}

//	bool debug = false;
//	if (adw_Node == 0x4E00)
//		debug = true;

	if (!pEntry->mod_time)
		pEntry->mod_time = time(NULL);
	PackEntry(pEntry, &u.ent, &u.ent_W);


	WORD entryoffset = NodeSize
		                - pNod->free_space;
/*
	if (debug) {
		wprintf(L"e='%s'\n", pEntry->entry_name);
		printf("node=%08X  entoffset=%08X freespace=%i entsize=%i,%i preventcount=%i\n",
			adw_Node,
			adw_Node + entryoffset,
			pNod->free_space,
			m_EntFixedLen + GetF(u.ent, entry_name_len),
			sizeof(u.ent_W) + pEntry->name_len + 2,
			pNod->ent_count
		);
	}
*/
	if (pNod->ent_count == 0) {
		pNod->first_offset = pNod->last_offset = entryoffset + m_EntFixedLen;
	}
	else if (ab_ToFirst) {
		pNod->first_offset = entryoffset + m_EntFixedLen;
	}
	else if (ab_ToLast) {
		pNod->last_offset = entryoffset + m_EntFixedLen;
	}
	pNod->ent_count ++;
	pNod->free_space -= m_EntFixedLen + GetF(u.ent, entry_name_len);
	// FIXME: add assert(pNod->free_space >= 0)
	pEntry->entry_offset = adw_Node + entryoffset;
	pEntry->node = adw_Node;

	if (m_bUTF) {
		ret = WriteData(&u.ent_W,
			             sizeof(u.ent_W),
							 pEntry->entry_offset);
		if (ret == PBLMI_OK) {
			ret = WriteData(pEntry->entry_name, pEntry->name_len + 2);
		}
	}
	else {
		ret = WriteData(&u.ent,
			             sizeof(u.ent),
							 pEntry->entry_offset);
		if (ret == PBLMI_OK) {
			ret = WriteData(pEntry->entry_name, pEntry->name_len + 1);
		}
	}
	if (ret == PBLMI_OK) {
		ret = WriteData(pNod, sizeof(PBL_NOD), adw_Node);
	}
	if (ret != PBLMI_OK)
		return ret;



	return PBLMI_OK;
}

void PBLMI_PBL::PackEntry(PBL_ENTRYINFO *pEntry, PBL_ENT *pEnt, PBL_ENT_W *pEnt_W)
{
	memcpy(GetPF(pEnt, sign), "ENT*", 4);
	memcpy(GetPF(pEnt, major_ver),
		(m_bUTF?(void*)L"0600":(void*)"0600"), (m_bUTF?8:4));
	SetPF(pEnt, creation_time.tm, pEntry->mod_time);
	SetPF(pEnt, data_offset, pEntry->data_offset);
	SetPF(pEnt, data_len, pEntry->data_len);
	SetPF(pEnt, comment_len, pEntry->comment_len / (m_bUTF?2:1));
	SetPF(pEnt, entry_name_len, pEntry->name_len + (m_bUTF?2:1));
}

void PBLMI_PBL::UnpackEntry(PBL_ENTRYINFO *pEntry, PBL_ENT *pEnt, PBL_ENT_W *pEnt_W)
{
	pEntry->mod_time = GetPF(pEnt, creation_time.tm);
	pEntry->data_offset = GetPF(pEnt, data_offset);
	pEntry->data_len = GetPF(pEnt, data_len);
	pEntry->comment_len = GetPF(pEnt, comment_len) * (m_bUTF?2:1);
	pEntry->name_len = GetPF(pEnt, entry_name_len) - (m_bUTF?2:1);
}

/**
	@param p pointer to buffer
	@param pBufSize pointer to size of the buffer

   @returns
	  PBLMI_Result and
	  size of data in *pBufSize if *pBufSize is 0
	  or
	  data in p and size of data in *pBufSize otherwise

*/
PBLMI_Result PBLMI_PBL::ReadSCCData(void *p, int *pBufSize)
{
	if(!pBufSize)
		return PBLMI_BADARG;
	char * pBuf = (char*)p;
	PBL_HDR_U pbl;
	PBL_DAT_HDR hdr;
	PBLMI_Result ret;
	ret = ReadData(
		&pbl,
		(m_bUTF ? sizeof(pbl.hdr_W) : sizeof(pbl.hdr)),
		m_StartOffset
	);
	if (ret != PBLMI_OK)
		return ret;
	if (! *pBufSize) {
		*pBufSize = GetF(pbl.hdr, sc_data_length);
		return PBLMI_OK;
	}

	if (*pBufSize < GetF(pbl.hdr, sc_data_length)) {
		return PBLMI_BADARG;
	}
	*pBufSize = GetF(pbl.hdr, sc_data_length);
	DWORD curr = 0;
	hdr.next = GetF(pbl.hdr, sc_data_offset);
	while(hdr.next) {
		fseek(m_Handle, hdr.next, SEEK_SET);
		fread(&hdr, sizeof(hdr), 1, m_Handle);
		if (strncmp(hdr.sign, "DAT*", 4))
			return PBLMI_BADFORMAT;
		fread(pBuf + curr, hdr.datalen, 1, m_Handle);
		curr += hdr.datalen;
	}
	return PBLMI_OK;
}


/**
	@param pBuf pointer to buffer
	@param BufSize size of the buffer

   @returns
	  PBLMI_Result

*/
PBLMI_Result PBLMI_PBL::WriteSCCData(void *p, int BufSize)
{
	char * pBuf = (char*)p;
	PBL_HDR_U pbl;
//	PBL_DAT_HDR hdr;
	PBLMI_Result ret;
	ret = ReadData(
		&pbl,
		(m_bUTF ? sizeof(pbl.hdr_W) : sizeof(pbl.hdr)),
		m_StartOffset
	);
	if (ret != PBLMI_OK)
		return ret;

	ret = UpdateBlocksData(
		GetF(&pbl.hdr, sc_data_offset),
		p,
		BufSize,
		FALSE
	);
	if (ret != PBLMI_OK)
		return ret;
	SetF(pbl.hdr, sc_data_length, BufSize);
	ret = WriteData(
		&pbl,
		(m_bUTF ? sizeof(pbl.hdr_W) : sizeof(pbl.hdr)),
		m_StartOffset
	);
	if (ret != PBLMI_OK)
		return ret;


	return PBLMI_OK;
}


PBLMI_Result
PBLMI_PBL::UpdateBlocksData
	(
		INT32 *a_dwFirstBlockOffset,
		const void *pBuf, INT32 bufLen, BOOL a_bAllocEmpty
	)
{
	INT32 &dwFirstBlockOffset = *a_dwFirstBlockOffset;
	PBLMI_Result ret;
	BlockList blocks;
	ret = ListBlocks(dwFirstBlockOffset, blocks);
	if (ret != PBLMI_OK)
		return ret;

	int newBlockCount =
		int(bufLen / DATDataSize) + (bufLen % DATDataSize ? 1 : 0);
	if (newBlockCount == 0 && a_bAllocEmpty)
		newBlockCount = 1;
	if (newBlockCount > blocks.size()) {
		ret = AllocateBlocks(newBlockCount - blocks.size(), blocks);
		if (ret != PBLMI_OK)
			return ret;
	}
	else if (newBlockCount < blocks.size()) {
		ret = FreeBlocks(blocks, newBlockCount);
		if (ret != PBLMI_OK)
			return ret;
		blocks.resize(newBlockCount);
	}
	PBL_DAT dat;
	int curr = 0;
	for (int i = 0; i < blocks.size(); i ++) {
		dat.datalen = WORD(
			DATDataSize > bufLen - curr
				? bufLen - curr
				: DATDataSize
		);
		memcpy(dat.sign, "DAT*", 4);
		if (i < blocks.size() - 1)
			dat.next = blocks[i + 1];
		else
			dat.next = 0;

		ret = WriteData(&dat, sizeof(PBL_DAT_HDR), blocks[i]);
		if (ret != PBLMI_OK)
			return ret;
		ret = WriteData((BYTE*)pBuf + curr, dat.datalen);
		if (ret != PBLMI_OK)
			return ret;
		if (dat.datalen < DATDataSize) {
			memset(&dat.data, 0, DATDataSize - dat.datalen);
			ret = WriteData(&dat.data, DATDataSize - dat.datalen);
			if (ret != PBLMI_OK)
				return ret;
		}
		curr += dat.datalen;
	}
	if (blocks.size() > 0)
		dwFirstBlockOffset = blocks[0];
	else
		dwFirstBlockOffset = 0;

	return PBLMI_OK;
}


PBLMI_Result PBLMI_PBL::ReadComments(void *p, int *pBufSize)
{
	if(!pBufSize)
		return PBLMI_BADARG;
	PBL_HDR_U pbl;
	PBLMI_Result ret;
	ret = ReadData(
		&pbl,
		SizeOf(pbl.hdr),
		m_StartOffset
	);
	if (ret != PBLMI_OK)
		return ret;
	int dataLen = StrLen(GetF(pbl.hdr, comments)) * m_CharSize + m_CharSize;
	if (! *pBufSize) {
		*pBufSize = dataLen;
		return PBLMI_OK;
	}
	//char * pBuf = (char*)p;
	if (*pBufSize < dataLen) {
		return PBLMI_BADARG;
	}
	*pBufSize = dataLen;
	memcpy(p, GetF(pbl.hdr, comments), dataLen);
	return PBLMI_OK;
}

/**
	@param p null-terminated string (for PB10 - in unicode)

*/
PBLMI_Result PBLMI_PBL::WriteComments(void *p)
{

	char * pBuf = (char*)p;
	PBL_HDR_U pbl;
	PBLMI_Result ret;
	ret = ReadData(
		&pbl,
		SizeOf(pbl.hdr),
		m_StartOffset
	);
	if (ret != PBLMI_OK)
		return ret;
	int dataLen = StrLen(pBuf) * m_CharSize + m_CharSize;
	if (dataLen > (int)SizeOfF(pbl.hdr, comments)) {
		return PBLMI_BADARG;
	}
	memcpy(GetF(pbl.hdr, comments), p, dataLen);
	memset(GetF(pbl.hdr, comments) + dataLen, 0, SizeOfF(pbl.hdr, comments) - dataLen);
	//SetF(pbl.hdr, creation_time, time(NULL));
	ret = WriteData(
		&pbl,
		SizeOf(pbl.hdr),
		m_StartOffset
	);

	return PBLMI_OK;
}

Node* PBLMI_PBL::DumpNodes()
{
	Node *pNode = new Node;
	DumpNodes(pNode, m_StartOffset + m_RootNodeOffset);
	return pNode;
}

PBLMI_Result PBLMI_PBL::DumpNodes(Node *apNode, DWORD adwNode)
{
	if (!apNode || !adwNode)
		return PBLMI_BADARG;
	apNode->parent = NULL; // not supported yet
	apNode->left = NULL;
	apNode->right = NULL;
	apNode->offset = adwNode;
	PBLMI_Result ret;
	ret = ReadData(&apNode->nod, sizeof(apNode->nod), adwNode);
	if (ret != PBLMI_OK)
		return ret;
	if (apNode->nod.left) {
		apNode->left = new Node;
		ret = DumpNodes(apNode->left, apNode->nod.left);
		if (ret != PBLMI_OK)
			return ret;
	}
	if (apNode->nod.right) {
		apNode->right = new Node;
		ret = DumpNodes(apNode->right, apNode->nod.right);
		if (ret != PBLMI_OK)
			return ret;
	}

	return PBLMI_OK;
}

void PBLMI_PBL::FreeNodeDump(Node *apNode)
{
	if (apNode) {
		if (apNode->left)
			FreeNodeDump(apNode->left);
		if (apNode->right)
			FreeNodeDump(apNode->right);
		delete apNode;
	}
}

PBLMI_Result PBLMI_PBL::Optimize(const char *szNewLibName, int nTgtPBLVer)
{
	PBLMI_Result ret;
	if (nTgtPBLVer <= 0 )
		nTgtPBLVer = m_PbVer;
	if (nTgtPBLVer >= 10) {
		nTgtPBLVer = 10;
	}
	else if (nTgtPBLVer >= 6 && nTgtPBLVer <= 9) {
		nTgtPBLVer = 6;
	}

	ret = m_iPbLMI->CreateLibrary(szNewLibName, nTgtPBLVer);
	if (ret != PBLMI_OK)
		return ret;
	IPBLMI_PBL* iNewPBL = m_iPbLMI->OpenLibrary(szNewLibName, TRUE);
	if (!iNewPBL)
		return PBLMI_ERROR;

	Array<Ent> names(0, 1000);
	class OptimizeDir : public IPBLMI_Callback {
	private:
		Array<Ent> * m_pNames;
		int m_nPos;
		OptimizeDir() {}
	public:
		OptimizeDir(Array<Ent> *a_pNames) {
			m_pNames = a_pNames;
			m_nPos = 0;
		}
		BOOL DirCallback(PBL_ENTRYINFO *pEntry) {
			(*m_pNames).resize(m_nPos + 1);
			(*m_pNames)[m_nPos].name = pEntry->entry_name;
			(*m_pNames)[m_nPos].freename = TRUE;
			pEntry->entry_name = NULL;
			(*m_pNames)[m_nPos].offset = pEntry->entry_offset;
			(*m_pNames)[m_nPos].len = pEntry->entry_len;

			//printf("[%i] %i %s\n", m_nPos +1, (*m_pNames)[m_nPos].len,
			//	(*m_pNames)[m_nPos].name);
			m_nPos ++;
			return TRUE;
		}
	} oDir(&names);

	Ent::iPbl = this;

	ret = this->Dir(&oDir);
	if (ret != PBLMI_OK) {
		iNewPBL->Close();
		return ret;
	}

	qsort(&names[0],
		names.size(),
		sizeof(Ent),
		(int (__cdecl *)(const void *,const void *))	& Ent::cmpansi
	);


	//printf("Dump pos\n");
	Array<int> nodepos(0);
	int curSize = NodeDataSize;
	int curPos = -1;
	for (int i = 0; i < names.size(); i ++) {
		if (curSize + names[i].len > NodeDataSize) {
			//printf("Next [%i][]\n", NodeDataSize - curSize);
			curPos ++;
			nodepos.resize(curPos + 1);
			nodepos[curPos] = i;
			curSize = 0;
		}
		curSize += names[i].len;
		//printf("[%i][%i][%i] %s\n", curPos, i, curSize, names[i].name);
	}
	//fgetc(stdin); // !!!
	ret = CreateOptimizedEntries(names, nodepos, 0, nodepos.size(), iNewPBL);
	if (ret == PBLMI_OK) {
		ret = UpdateOptimizedEntries(names, iNewPBL);
	}
	if (ret == PBLMI_OK) {
		int nBufSize = 0;
		ret = this->ReadComments(NULL, &nBufSize);
		ByteBuffer buf(nBufSize);
		ret = this->ReadComments(buf.buffer(), &nBufSize);
		ret = iNewPBL->WriteComments(buf.buffer());
	}
	if (ret == PBLMI_OK) {
		int nBufSize = 0;
		ret = this->ReadSCCData(NULL, &nBufSize);
		ByteBuffer buf(nBufSize);
		ret = this->ReadSCCData(buf.buffer(), &nBufSize);
		ret = iNewPBL->WriteSCCData(buf.buffer(), nBufSize);
	}
	if (ret != PBLMI_OK) {
		iNewPBL->Close();
		return ret;
	}
	iNewPBL->Close();
	return PBLMI_OK;
}
PBLMI_Result PBLMI_PBL::CreateOptimizedEntries
(
	Array<Ent> & names,
	Array<int> & nodepos,
	int posIndex1,
	int posIndex2,
	IPBLMI_PBL* aiNewPBL
)
{
	PBLMI_Result ret;
	if (posIndex1 >= posIndex2)
		return PBLMI_OK; // skip empty node
//	if (posIndex1 == posIndex2 - 1) {
//	}
	int posIndexM = (posIndex1 + posIndex2 - 1) / 2;
	int entryIndex1 = nodepos[posIndexM];
	int entryIndex2 = names.size();
	if (posIndexM < nodepos.size() - 1)
		entryIndex2 = nodepos[posIndexM + 1];
	//printf("Create Opt Node %i-%i(%i)\n", entryIndex1+1, entryIndex2, entryIndex2 - entryIndex1);
	for (int i = entryIndex1; i < entryIndex2; i ++) {
		PBL_ENTRYINFO ei;
		ret = aiNewPBL->SeekEntry(names[i].name, &ei, TRUE);
		if (ret != PBLMI_OK)
			return ret;
	}
	//printf("End Create Opt Node %i-%i(%i)\n", entryIndex1+1, entryIndex2, entryIndex2 - entryIndex1);

	ret = CreateOptimizedEntries(names, nodepos, posIndex1, posIndexM, aiNewPBL);
	if (ret != PBLMI_OK)
		return ret;
	ret = CreateOptimizedEntries(names, nodepos, posIndexM + 1, posIndex2,  aiNewPBL);
	if (ret != PBLMI_OK)
		return ret;

	return PBLMI_OK;
}

PBLMI_Result PBLMI_PBL::UpdateOptimizedEntries
(
	Array<Ent> & names,
	IPBLMI_PBL* aiNewPBL
)
{
	PBLMI_Result ret;
	PBL_ENT_U eb;
	for (int i = 0; i < names.size(); i ++) {
		PBL_ENTRYINFO src_ei, tgt_ei;
		ret = ReadData(&eb, m_EntFixedLen, names[i].offset);
		if (ret != PBLMI_OK)
			return ret;
		UnpackEntry(&src_ei, (PBL_ENT*)&eb.ent, (PBL_ENT_W*)&eb.ent_W);
		ret = aiNewPBL->SeekEntry(names[i].name, &tgt_ei, FALSE);
		if (ret != PBLMI_OK)
			return ret;
		ByteBuffer data(src_ei.data_len);
		ret = ReadEntryData(&src_ei, data.buffer());
		if (ret != PBLMI_OK)
			return ret;
		ret = aiNewPBL->UpdateEntryData(
			&tgt_ei,
			data.buffer(),
			src_ei.data_len - src_ei.comment_len,
			src_ei.comment_len
		);
		if (ret != PBLMI_OK)
			return ret;
		tgt_ei.mod_time = src_ei.mod_time;
		ret = aiNewPBL->SetEntryTime(&tgt_ei);
		if (ret != PBLMI_OK)
			return ret;
	}

	return PBLMI_OK;
}

void PBLMI_PBL::_dump_node_tree()
{
	//

}

void PBLMI_PBL::_print_node(FILE *aOut, int aLevel, DWORD aNode, DWORD aParentNode)
{
	if (aOut == NULL)
		aOut = stdout;
	if (aNode == 0)
		aNode = m_RootNodeOffset;

	for (int l = 0; l < aLevel; l ++)
		fprintf(aOut, "   ");
	aLevel ++;
	PBL_NOD nod;
	ReadData(&nod, sizeof(nod), aNode);
	if (nod.parent != aParentNode)
		fprintf(aOut, "[BAD] ");
	fprintf(aOut, "N: 0x%08X P: 0x%08X N: %i\n", aNode, nod.parent, nod.ent_count);
	if (nod.right)
		_print_node(aOut, aLevel, nod.right, aNode);
	if (nod.left)
		_print_node(aOut, aLevel, nod.left, aNode);

}

void WideToAnsi(const wchar_t * src, char_array &tgt) {
	int wlen = wcslen(src) + 1; //+Z
	int li_NewLen = WideCharToMultiByte(
		CP_ACP,
		0, //WC_DEFAULTCHAR,
		src,
		wlen, // incl 0
		NULL,
		0, // return buf size in bytes
		NULL, //"?", // def char
		NULL // &lb_UsedDefChar
	);

	if (li_NewLen == 0) {
		return;
	}
	tgt.resize(li_NewLen);

	WideCharToMultiByte(
		CP_ACP,
		0, //WC_DEFAULTCHAR,
		src,
		wlen, // incl 0
		&tgt[0],
		li_NewLen,
		NULL, //"?", // def char
		NULL  //&lb_UsedDefChar
	);
}
