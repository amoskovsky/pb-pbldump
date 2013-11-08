// PblDump.cpp: implementation of the PblDump class.
//
//////////////////////////////////////////////////////////////////////

#include "PblDump.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

PblDump::PblDump()
{
    TypePrefix[SOURCE] = "";
    TypePrefix[PCODE] = "$PCODE$\\";
    TypePrefix[RESOURCE] = "$RESOURCES$\\";
    TypePrefix[NODE] = "$NODES$\\"; 
    TypePrefix[SCCDATA] = "$SCCDATA$\\"; 
    m_bExportPcode = TRUE; // *.* exports pcode
    m_iExportCP = ECP_AUTO;  
    m_bDumpNodes = FALSE;
    m_DebugFlags = DebugNone;
    

}

PblDump::~PblDump()
{

}
int main(int argc, char ** argv) 
{
    PblDump p;
    return p.main(argc, argv);
}

int PblDump::main(int argc, char ** argv)
{
    int iRet;
    m_bExportEntryTime = FALSE;
    //list entries
    if (argc == 3 && !strnicmp(argv[1], "-v", 2)) {
        for (int i = 2; argv[1][i]; i ++) {
            if (argv[1][i] == 'n') 
                m_bDumpNodes = TRUE;
            else if (argv[1][i] == 'd') 
                m_DebugFlags = DebugPrefixNameWithNode;
            else {
                printf ("Invalid option '%c' specified\n", argv[1][i]);
                return 5;
            }
        }
        logo();
        iRet = init(argv[2]);
        if (iRet)
            return iRet;
        return listPBL();
    }
    //export entries
    else if (argc > 3 && !strnicmp(argv[1], "-e", 2)) {
        iRet = init(argv[2]);
        if (iRet)
            return iRet;
        // parse "-e..." switch
        for (int i = 2; argv[1][i]; i ++) {
            if (argv[1][i] == 'a') 
                m_iExportCP = ECP_ANSI;
            else if (argv[1][i] == 'u')
                m_iExportCP = ECP_UNICODE;
            else if (argv[1][i] == 's')
                m_bExportPcode = FALSE;
            else if (argv[1][i] == 't')
                m_bExportEntryTime = TRUE;
            else {
                printf ("Invalid option '%c' specified\n", argv[1][i]);
                return 5;
            }
        }
        return exportEntries(argc - 3, argv + 3);
    }
    // delete entries
    else if (argc > 3 && !strnicmp(argv[1], "-d", 2)) {
        iRet = init(argv[2]);
        if (iRet)
            return iRet;
        // parse "-d..." switch
        for (int i = 2; argv[1][i]; i ++) {
            if (argv[1][i] == 's')
                m_bExportPcode = FALSE;
            else {
                printf ("Invalid option '%c' specified\n", argv[1][i]);
                return 5;
            }
        }
        return deleteEntries(argc - 3, argv + 3);
    }
    else {
        logo();
        usage();
        return 1;
    }
    return 0;
}

void  PblDump::logo()
{
   printf(
        "PowerBuilder Library Dumper " PBLDUMP_VERSION "\n"
        "Copyright (C) Anatoly Moskovsky <avm@sqlbatch.com>, 2000-2009.\n"
        "This software is freeware.\n"
    );
}
void  PblDump::usage()
{
   printf(
   "USAGE: pbldump <options> <library_file> [library_entries ...]\n"
   "List of options: \n"
   "  -v\tlist entries\n"
   "  -e[a|u][s][t]\textract entries, use PBL's encoding\n"
    "    a\tforce ANSI output encoding\n"
    "    u\tforce Unicode(UTF-16LE) output encoding\n"
    "    s\textract source only (w/o resources & p-code) if *.* given\n"
    "    t\tuse entry time to set file modification time\n"
    "  <library_entry> is \n"
    "     entryname.type (e.g. w_test.srw) or\n"
    "     *.* for all entries or\n"
    "     @<listfile>\n"
    "  Default output encoding depends on PBL version:\n"
    "  - for PB9 and below ANSI is used\n"
    "  - for PB10 and higher Unicode is used\n"
    "Examples:\n"
    "1) Extract all source entries using Unicode: | 3) List entries:\n"
    "   pbldump -esu test.pbl *.*                 |    pbldump -v test.pbl\n"
    "2) Extract entries listed in the file:       |\n"
    "   pbldump -e test.pbl @entries.txt          |\n"
   );
}


BOOL PblDump::DirCallback(PBL_ENTRYINFO *pEntry)
{
    // this callback proc is used in many ways
    if (m_dirType == ListDir) {
        BOOL isUTF = m_iPbl->isUnicode();
        struct tm *newtime;
        time_t aclock;
        m_nEntCount ++;
        int len = strlen(pEntry->entry_name);
        EntryType et = getEntryType(pEntry->entry_name);
        if (et == SOURCE) {
            pEntry->data_len += isUTF ? 2 + 2 * (18 + len) : (18 + len);
            if (pEntry->comment_len)
                pEntry->data_len += isUTF ? 2 * 20 : 20;
        }
        static char name[1024];
        strcpy(name, TypePrefix[et]);
        strcat(name, pEntry->entry_name);
        aclock = pEntry->mod_time;
        newtime = localtime( &aclock );
        printf("%-50s%09u %02i.%02i.%04i %02i:%02i:%02i\n",
            name, pEntry->data_len,
            newtime->tm_mday, newtime->tm_mon + 1, newtime->tm_year + 1900,
            newtime->tm_hour, newtime->tm_min, newtime->tm_sec
        );

    }
    else if (m_dirType == ExportDir ) {
        BOOL bExport = TRUE;
        if (!m_bExportPcode) {
            EntryType et = getEntryType(pEntry->entry_name);
            if (et != SOURCE) {
                bExport = FALSE;
            }
        }
        if (bExport) {
            m_nEntCount ++;
            printf("%i %s\n", m_nEntCount, pEntry->entry_name);
            int res = exportEntry(pEntry, pEntry->entry_name);
            if (res)
                return FALSE;
        }
    }
    else if (m_dirType == DeleteDir ) {
        BOOL bDelete = TRUE;
        if (!m_bExportPcode) {
            EntryType et = getEntryType(pEntry->entry_name);
            if (et != SOURCE) {
                bDelete = FALSE;
            }
        }
        if (bDelete) {
            m_nEntCount ++;
            printf("%i %s\n", m_nEntCount, pEntry->entry_name);
            int res = deleteEntry(pEntry, pEntry->entry_name);
            if (res)
                return FALSE;
        }
    }
    
    return TRUE;
}

PblDump::EntryType PblDump::getEntryType( const char * szEntryName) 
{
    static const char * s_ext[] = 
        {".srd", ".srs", ".srw", ".sru", ".srf",  
         ".srm", ".srx", ".srj", ".srp", ".srq", ".sra", NULL};
    static const char * p_ext[] = 
        {".dwo", ".str", ".win", ".udo", ".fun",
         ".men", ".xxy", /*".srj", ".srp", ".srq", */ 
         ".apl", ".pra", ".bin", ".exe", NULL};
    int len = strlen(szEntryName);
    if (len < 5)
        return RESOURCE;
    int i;
    for (i = 0; s_ext[i]; i ++) {
        if (!strcmp(szEntryName + len - 4, s_ext[i]))
            return SOURCE;
    }
    for (i = 0; p_ext[i]; i ++) {
        if (!strcmp(szEntryName + len - 4, p_ext[i]))
            return PCODE;
    }
    if (!strnicmp(szEntryName, TypePrefix[SCCDATA], strlen(TypePrefix[SCCDATA]))) {
        return SCCDATA;
    }
    return RESOURCE;
}


int PblDump::exportEntries(int e_num, char * e_list [])
{
    int i;
    m_nEntCount = 0;
            
    for (i = 0; i < e_num; i ++) {
        if (!strcmp(e_list[i], "*.*")) { // wildcard export
            PBLMI_Result ret;   
            printf("Exporting entries...\n--\n");
            m_nEntCount = 0;
            m_dirType = ExportDir;
            ret = m_iPbl->Dir(this);
            if (ret != PBLMI_OK) {
                printf("Can't list library contents\n");
                return 4;
            }
            printf("--\nTotal objects: %i\n", m_nEntCount);

            return 0;
        }
        else if (e_list[i][0] == '@') {  // list file
            e_list[i]++;
            FILE *f = fopen(e_list[i], "rt");
            if (!f) {
                printf("Can't open '%s'", e_list[i]);
                return 12;
            }
            static char line[MAXFILEPATH];
            char *p;
            while(fgets(line, MAXFILEPATH, f)) {
                int len = strlen(line);
                // remove trailing spaces
                for(p = line + len - 1; 
                    p >= line && 
                    (*p == '\n' || *p == '\t' || *p == ' '); 
                     p --) 
                {
                    *p = 0;
                }
                // remove leading spaces
                for(p = line; *p && (*p == '\t' || *p == ' '); p ++);
                if (!*p)
                    continue; // empty line
                int ret = exportEntry(p);
                if (ret) {
                    fclose(f);
                    return ret;
                }
            }
            fclose(f);
        }
        else { // entry name
            int ret = exportEntry(e_list[i]);
            if (ret)
                return ret;
        }
    }
    return 0;
}

int PblDump::deleteEntries(int e_num, char * e_list [])
{
    int i;
    m_nEntCount = 0;
            
    for (i = 0; i < e_num; i ++) {
        if (!strcmp(e_list[i], "*.*")) { // wildcard export
            PBLMI_Result ret;   
            printf("Deleting entries...\n--\n");
            m_nEntCount = 0;
            m_dirType = DeleteDir;
            ret = m_iPbl->Dir(this);
            if (ret != PBLMI_OK) {
                printf("Can't list library contents\n");
                return 4;
            }
            printf("--\nTotal objects: %i\n", m_nEntCount);

            return 0;
        }
        else if (e_list[i][0] == '@') {  // list file
            e_list[i]++;
            FILE *f = fopen(e_list[i], "rt");
            if (!f) {
                printf("Can't open '%s'", e_list[i]);
                return 12;
            }
            static char line[MAXFILEPATH];
            char *p;
            while(fgets(line, MAXFILEPATH, f)) {
                int len = strlen(line);
                // remove trailing spaces
                for(p = line + len - 1; 
                    p >= line && 
                    (*p == '\n' || *p == '\t' || *p == ' '); 
                     p --) 
                {
                    *p = 0;
                }
                // remove leading spaces
                for(p = line; *p && (*p == '\t' || *p == ' '); p ++);
                if (!*p)
                    continue; // empty line
                int ret = deleteEntry(p);
                if (ret) {
                    fclose(f);
                    return ret;
                }
            }
            fclose(f);
        }
        else { // entry name
            int ret = deleteEntry(e_list[i]);
            if (ret)
                return ret;
        }
    }
    return 0;
}

int PblDump::init(const char * szPbl, bool bReadWrite)
{
    m_iPbLMI = PBLMI_GetInterface();
    if (!m_iPbLMI) {
        printf("Internal error: PBLMI_GetInterface failed\n");
        return 2;
    }
    m_iPbLMI->SetTargetCodePage(PBLMI_ANSI);
    if (szPbl) {
        m_iPbl = m_iPbLMI->OpenLibrary(szPbl, bReadWrite);
        if (!m_iPbl) {
            printf("Invalid or missing library (%s)\n", szPbl);
            return 3;
        }
    }
    else {
        m_iPbl = NULL;
    }
    return 0;
}
int PblDump::cleanup()
{
    if (m_iPbLMI)
        m_iPbLMI->Release(), m_iPbLMI = NULL;
    if (m_iPbl)
        m_iPbl->Close(), m_iPbl = NULL;
    
    // let OS clean up the rest
    return 0;
}
int PblDump::listPBL()
{
    PBLMI_Result ret;   
    printf("Listing entries...\n--\n");
    m_nEntCount = 0;
    m_dirType = ListDir;
    ret = m_iPbl->Dir(this);
    if (ret != PBLMI_OK) {
        printf("Can't list library contents\n");
        return 4;
    }
    if (m_bDumpNodes) {
        Node * pRootNode = ((PBLMI_PBL*)m_iPbl)->DumpNodes();
        if (pRootNode) {
            ListNodes(pRootNode);
            ((PBLMI_PBL*)m_iPbl)->FreeNodeDump(pRootNode);
        }
    }
    int sccDataSize = getSCCDataSize();
    if (sccDataSize > 0) {
        m_nEntCount ++;
        char str[1000];
        strcpy(str, TypePrefix[SCCDATA]);
        strcat(str, "sccdata");
        time_t aclock = time(NULL);
        struct tm *newtime = localtime(&aclock);
        printf("%-50s%09u %02i.%02i.%04i %02i:%02i:%02i\n",
            str, sccDataSize,
            newtime->tm_mday, newtime->tm_mon + 1, newtime->tm_year + 1900,
            newtime->tm_hour, newtime->tm_min, newtime->tm_sec
        );
    }

    printf("--\nTotal objects: %i\n", m_nEntCount);
    return 0;
}


int PblDump::exportEntry(char * e_name)
{
    EntryType et = getEntryType(e_name);
    if (et == SCCDATA) 
        return exportSCCData(e_name);
    int prefixLen = strlen(TypePrefix[et]);
    if (prefixLen)
        if (!strncmp(e_name, TypePrefix[et], prefixLen)) 
            e_name += prefixLen;
    m_nEntCount ++;
                
    printf("%i %s\n", m_nEntCount, e_name);
    PBLMI_Result ret;
    PBL_ENTRYINFO entry;
    ret = m_iPbl->SeekEntry(e_name, &entry, FALSE);
    if (ret == PBLMI_NOTFOUND) {
        printf("Entry '%s' not found\n", e_name);
        return 6;
    }
    else if (ret != PBLMI_OK) {
        printf("Can't export entry '%s'\n", e_name);
        return 7;
    }
    return exportEntry(&entry, e_name);
}
int PblDump::deleteEntry(char * e_name)
{
    EntryType et = getEntryType(e_name);
    int prefixLen = strlen(TypePrefix[et]);
    if (prefixLen)
        if (!strncmp(e_name, TypePrefix[et], prefixLen)) 
            e_name += prefixLen;
    m_nEntCount ++;
                
    printf("%i %s\n", m_nEntCount, e_name);
    PBLMI_Result ret;
    PBL_ENTRYINFO entry;
    ret = m_iPbl->SeekEntry(e_name, &entry, FALSE);
    if (ret == PBLMI_NOTFOUND) {
        printf("Entry '%s' not found\n", e_name);
        return 6;
    }
    else if (ret != PBLMI_OK) {
        printf("Can't delete entry '%s'\n", e_name);
        return 11;
    }
    return deleteEntry(&entry, e_name);
}
int PblDump::deleteEntry(PBL_ENTRYINFO *pEntry, char * e_name)
{
    PBLMI_Result ret;
    ret = m_iPbl->DeleteEntry(pEntry);
    if (ret == PBLMI_OK) {
        m_iPbl->Flush();
    }
    if (ret != PBLMI_OK) {
        printf("Can't delete entry '%s'\n", e_name);
        return 11;
    }
    return 0;
}
int PblDump::exportEntry(PBL_ENTRYINFO *pEntry, char * e_name)
{
    PBLMI_Result ret;
    int res;
    ByteBuffer buf(pEntry->data_len);
    ret = m_iPbl->ReadEntryData(pEntry, buf.buffer());
    if (ret != PBLMI_OK) {
        printf("Can't export entry '%s'\n", e_name);
        return 7;
    }
    int namepos = FindEndOfDir(e_name);
    e_name += namepos;
    EntryType et = getEntryType(e_name);
    int len = strlen(e_name);
    FILE * f = fopen (e_name, "wb+");
    if(!f) {
        printf("Can't open '%s' for writing\n", e_name);
        return 8;
    }
    BOOL isUTF = m_iPbl->isUnicode();
    
    
    if (et == SOURCE) {
        // convert encoding
        BOOL bHexUsed = FALSE;
        if (isUTF && m_iExportCP == ECP_ANSI) {
            convertToANSI(buf, pEntry->comment_len, pEntry->data_len, bHexUsed);
            isUTF = FALSE;
            if (bHexUsed) {
                u_fprintf(f, "HA");
            }
        }
        else if (!isUTF && m_iExportCP == ECP_UNICODE) {
            convertToUTF(buf, pEntry->comment_len, pEntry->data_len);
            isUTF = TRUE;
        }

        // write header
        char *e_name_u = NULL;
        if (isUTF) {
            fwrite("\xFF\xFE", 2, 1, f); // UTF16LE signature
            e_name_u = MultiToWide(e_name, len, CP_ACP);
        }
        u_fprintf1(f, "$PBExportHeader$%s\r\n", (isUTF?e_name_u:e_name));
        if(e_name_u)
            delete [] e_name_u; 
        if (pEntry->comment_len) {

            u_fprintf(f, "$PBExportComments$");
            int inc = isUTF ? 2 : 1;
            for (int i = 0; i < pEntry->comment_len; i += inc) {
                WORD c;
                if (isUTF) {
                    c = buf[i] + ((WORD)buf[i+1] << 8);
                    //wprintf(L"{%x}'%c'", buf[i], buf[i]);
                    switch(c) {
                    case L'\r':
                        fwprintf(f, L"~r");
                        break;
                    case L'\n':
                        fwprintf(f, L"~n");
                        break;
                    case L'\t':
                        fwprintf(f, L"~t");
                        break;
                    case L'~':
                        fwprintf(f, L"~~");
                        break;
                    default: 
                        fwprintf(f, L"%c", c);
                    }
                }
                else {
                    c = buf[i];
                    switch(c) {
                    case '\r':
                        fprintf(f, "~r");
                        break;
                    case '\n':
                        fprintf(f, "~n");
                        break;
                    case '\t':
                        fprintf(f, "~t");
                        break;
                    case '~':
                        fprintf(f, "~~");
                        break;
                    default: 
                        fprintf(f, "%c", c);
                    }
                }

            }
            u_fprintf(f, "\r\n");
        }

    }
    res = fwrite(buf.buffer() + pEntry->comment_len, 
                  pEntry->data_len - pEntry->comment_len, 1, f);
    if(!res) {
        printf("Can't write to '%s'\n", e_name);
        fclose(f);
        return 10;
    }

    if (et == SOURCE) {
        // write footer
        
        // check binary part
        int extpos = strchr(e_name, '.') - (e_name)+ 1;
        char binname[MaxEntryNameSize];
        strncpy(binname, e_name + namepos, extpos);
        strcpy(binname + extpos, "bin");
        PBL_ENTRYINFO bentry;
        ret = m_iPbl->SeekEntry(binname, &bentry, FALSE);
        if (ret == PBLMI_OK) {
            //printf("");
            res = writeBinaryData(f, &bentry, e_name + namepos, binname, isUTF);
            if (res) {
                fclose(f);
                return res;
            }
        }
    }
    fclose(f);
    //timeb tm;
    //if (ftime(&tm)
    if (m_bExportEntryTime) {
        utimbuf tm;
        tm.actime = time(NULL);
        tm.modtime = pEntry->mod_time;
        utime(e_name, &tm);
    }
    return 0;
}


int PblDump::FindEndOfDir(const char *szPath)
{
   int i,p = 0;
   for (i = 0; szPath[i]; i ++) {
      if (szPath[i] == '\\' || szPath[i] == '/') {
         p = i + 1;
      }
   }
   return p;
}

int PblDump::writeBinaryData(FILE *f, PBL_ENTRYINFO *pEntry, char * e_name, char * binName, BOOL isUTF)
{
    PBLMI_Result ret;
   int hexBufLen = pEntry->data_len;
   if (pEntry->data_len % BinDataBlockSize) 
      hexBufLen += BinDataBlockSize - pEntry->data_len % BinDataBlockSize;
   ByteBuffer hexBuf(hexBufLen);
    // read bin data
    ret = m_iPbl->ReadEntryData(pEntry, hexBuf.buffer());
    if (ret != PBLMI_OK) {
        printf("Can't export entry's binary data \n");
        return 11;
    }
    // pad the buffer with zeroes
   memset(hexBuf.buffer() + pEntry->data_len, 0, hexBufLen - pEntry->data_len);
    
    int nlen = strlen(e_name);
    char *binName_u = NULL;
   char timeString[30];
   if (isUTF) {
        binName_u = MultiToWide(binName, nlen, CP_ACP);
        swprintf((unsigned short *)timeString, L"%u", pEntry->mod_time);
    }
    else {
        sprintf(timeString, "%u", pEntry->mod_time);
    }
   char digit = '7';
   if (!stricmp(e_name + nlen - 3, "sru")) 
      digit = '4';
    if (isUTF) {
        fwprintf(
            f, 
            L"\r\nStart of PowerBuilder Binary Data Section"
            L" : "
            L"Do NOT Edit\r\n"
            L"0%c%s %i %s\r\n"
            , digit, binName_u, pEntry->data_len, timeString
        );
    }
    else {
        fprintf(
            f, 
            "\r\nStart of PowerBuilder Binary Data Section"
            " : "
            "Do NOT Edit\r\n"
            "0%c%s %i %s\r\n"
            , digit, binName, pEntry->data_len, timeString
        );
    }
   for (int off = 0; off < (int)pEntry->data_len; off += BinDataBlockSize) {
      u_fprintf(f, "20");
      for (int off2 = 0; off2 < BinDataBlockSize; off2 += 4) {
         u_fprintf1(f, "%08x", *((DWORD*) (hexBuf.buffer() + off + off2)));
      }
      u_fprintf(f, "\r\n");
   }
    if(isUTF) {
        fwprintf(
            f, 
            L"1%c%s %i %s\r\n"
            L"End of PowerBuilder Binary Data Section"
            L" : "
            L"No Source Expected After This Point\r\n"
            , digit, binName_u, pEntry->data_len, timeString
        );
    }
    else {
        fprintf(
            f, 
            "1%c%s %i %s\r\n"
            "End of PowerBuilder Binary Data Section"
            " : "
            "No Source Expected After This Point\r\n"
            , digit, binName, pEntry->data_len, timeString
        );
    }
    if(binName_u)
        delete [] binName_u;
   return 0;
}

int PblDump::convertToUTF(ByteBuffer& buf, INT16 &comm_len, INT32 &data_len) 
{
    char * newComm = MultiToWide((char*)buf.buffer(), comm_len, CP_ACP); 
    char * newSrc = MultiToWide((char*)buf.buffer()+ comm_len, data_len - comm_len, CP_ACP); 
    int commLen = 2 * wcslen((wchar_t*)newComm);
    int srcLen = 2 * wcslen((wchar_t*)newSrc);
    buf.FreeBuffer();
    buf.CreateBuffer(commLen + srcLen);
    memcpy(buf.buffer(), newComm, commLen); 
    memcpy(buf.buffer() + commLen, newSrc, srcLen);
    delete [] newComm;
    delete [] newSrc;
    comm_len = commLen;
    data_len    = commLen + srcLen;
    return 0;
}

BOOL UTFToANSI(ByteBuffer& src, ByteBuffer& tgt)
{
    BOOL bUsedDefChar = FALSE;
    int li_NewLen = WideCharToMultiByte(
        CP_ACP,
        WC_COMPOSITECHECK | WC_DEFAULTCHAR,
        (LPCWSTR) src.buffer(),
        src.size() / 2, 
        NULL, 
        0, // return buf size in bytes
        NULL, //"?", // def char
        &bUsedDefChar
    );

    if (bUsedDefChar)
        return FALSE;

    tgt.FreeBuffer();
    tgt.CreateBuffer(li_NewLen);
    int li_Len = WideCharToMultiByte(
        CP_ACP,
        WC_COMPOSITECHECK | WC_DEFAULTCHAR,
        (LPCWSTR) src.buffer(),
        src.size() / 2, 
        (char*) tgt.buffer(), 
        li_NewLen, // return buf size in bytes
        NULL, //"?", // def char
        &bUsedDefChar
    );
    ByteBuffer tmp(src.size());
    li_Len = MultiByteToWideChar(
        CP_ACP,
        MB_PRECOMPOSED,
        (LPCTSTR)tgt.buffer(),
        li_NewLen,
        (LPWSTR)tmp.buffer(),
        tmp.size() / 2
    );
    if (memcmp(src.buffer(), tmp.buffer(), tmp.size()))
        return FALSE;
    return !bUsedDefChar && li_Len == li_NewLen;
}
int countNonAscii(WORD *p, int max) 
{
    int i;
    for(i = 0; i < max && p[i] >= 128; i ++);
    return i;
}
void UTFToHexAscii(ByteBuffer& src, ByteBuffer& tgt)
{
    int ascii_n = 0;
    int nonascii_block_n = 0;
    int nonascii_n = 0;

    // calc hexascii size
    BOOL bLastAscii = TRUE;
    WORD *p = (WORD*) src.buffer();
    for ( int i = 0; i < src.size() / 2; i ++) {
        if (p[i] >= 0 && p[i] < 128) { // ascii
            if (!bLastAscii) {
                nonascii_block_n ++;
            }
            bLastAscii = TRUE;
            ascii_n ++;
        }
        else {
            nonascii_n ++;
            bLastAscii = FALSE;
        }
    }
    if (!bLastAscii) {
        nonascii_block_n ++;
    }

    int newSize = ascii_n + 
        nonascii_n * 4 + nonascii_block_n * (8 + 10);
    tgt.FreeBuffer();
    tgt.CreateBuffer(newSize);
    char * t = (char *)tgt.buffer();
    bLastAscii = TRUE;
    for (i = 0; i < src.size() / 2; i ++) {
        if (p[i] >= 0 && p[i] < 128) { // ascii
            if (!bLastAscii) {
                strcpy(t, "$$ENDHEX$$");
                t += 10;
            }
            *t = (char) p[i];
            t++;
            bLastAscii = TRUE;
        }
        else {
            if (bLastAscii) {
                int n = sprintf(t, "$$HEX%i$$", countNonAscii(p + i, src.size() / 2 - i));
                t += n;
            }
            sprintf(t, "%02x%02x", p[i] & 0xFF, (p[i] >> 8) & 0xFF);
            t += 4;
            bLastAscii = FALSE;
        }
    }
    if (!bLastAscii) {
        strcpy(t, "$$ENDHEX$$");
        t += 10;
    }

}
int PblDump::convertToANSI(ByteBuffer& buf, INT16 &comm_len, INT32 &data_len, BOOL &bHexUsed) 
{
    ByteBuffer comm(buf.buffer(), comm_len);
    ByteBuffer src(buf.buffer() + comm_len, data_len - comm_len);
    ByteBuffer comm_a, src_a;
    if (UTFToANSI(comm, comm_a) && UTFToANSI(src, src_a)) {
        bHexUsed = FALSE;
    }
    else {
        UTFToHexAscii(comm, comm_a);
        UTFToHexAscii(src, src_a);
        bHexUsed = TRUE;
    }
    buf.FreeBuffer();
    buf.CreateBuffer(comm_a.size() + src_a.size());
    memcpy(buf.buffer(), comm_a.buffer(), comm_a.size());
    memcpy(buf.buffer() + comm_a.size(), src_a.buffer(), src_a.size());
    comm_len = comm_a.size();
    data_len = comm_len + src_a.size();
    return 0;
}

void PblDump::ListNodes(Node *apNode)
{
    char path[1024]="";
    ListNodes(apNode, path, 0, TypePrefix[NODE]);
}

void PblDump::ListNodes(Node *apNode, char *asPath, 
                                int aiPos, char *asPrefix)
{
    struct tm *newtime;
    time_t aclock = time(NULL);
    strcpy(asPath + aiPos, asPrefix);
    int pos = strlen(asPath);
    sprintf(asPath + pos, "%08X", apNode->offset);
    newtime = localtime( &aclock );
    printf("%-50s%09u %02i.%02i.%04i %02i:%02i:%02i\n",
        asPath, 100,
        newtime->tm_mday, newtime->tm_mon + 1, newtime->tm_year + 1900,
        newtime->tm_hour, newtime->tm_min, newtime->tm_sec
    );
    m_nEntCount ++;
    if (apNode->left) {
        ListNodes(apNode->left, asPath, pos, "L\\");
    }
    if (apNode->right) {
        ListNodes(apNode->right, asPath, pos, "R\\");
    }

}

int PblDump::exportSCCData(char *e_name)
{
    PBLMI_Result ret;
    int res;
    int size = getSCCDataSize();
    
    if (size <= 0)
        return 0;

    ByteBuffer buf(size);
    ret = m_iPbl->ReadSCCData(buf.buffer(), &size);
    if (ret != PBLMI_OK) {
        printf("Can't export entry '%s'\n", e_name);
        return 7;
    }

    int namepos = FindEndOfDir(e_name);
    e_name += namepos;

    FILE * f = fopen (e_name, "wb+");
    if(!f) {
        printf("Can't open '%s' for writing\n", e_name);
        return 8;
    }
    res = fwrite(buf.buffer(), buf.size(), 1, f);
    if (res != 1) { 
        printf("Can't write to '%s'\n", e_name);
        fclose(f);
        return 10;
    }
    fclose(f);
    return 0;
}

int PblDump::getSCCDataSize()
{
    int size = 0;
    PBLMI_Result ret;
    ret = m_iPbl->ReadSCCData(NULL, &size);
    if (ret != PBLMI_OK) {
        return 0;
    }
    else {
        return size;
    }
}
