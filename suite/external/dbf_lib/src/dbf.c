/* dbf.c */
/* Copyright (c) 2007-2016 by Troels K. All rights reserved. */
/* License: wxWindows Library Licence, Version 3.1 - see LICENSE.txt */
/* Partially based on MFC source code by www.pablosoftwaresolutions.com 2002   */
/* Partially based on Turbo C source code by Mark Sadler.      */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <ctype.h>
#include <math.h>
#ifdef __BORLANDC__
#include <ctime>
#endif

#include "../include/bool.h"
#include "../include/dbf.h"
#include "../include/ioapi/zlib.h"
#include "../include/ioapi/ioapi.h"
#include "../include/stdint-ms.h"
#ifdef __linux__
#include <limits.h>
#endif

#define _max(a,b) ((a)>(b)?(a):(b))
#define _min(a,b) ((a)<(b)?(a):(b))

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#define snprintf _snprintf
#elif defined(__BORLANDC__)
#define strcasecmp stricmp
#define snprintf snprintf
#endif
#ifndef PATH_MAX
  #ifndef __linux__
  #define PATH_MAX _MAX_PATH
  #endif
#endif
#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof((array)[0]))
#endif
#define C_ASSERT_(n,e) typedef char __C_ASSERT__##n[(e)?1:-1]
#ifndef offsetof
#define offsetof(s,m)   ((size_t)&(((s *)0)->m))
#endif

#define CPM_TEXT_TERMINATOR 0x1A
#define NOT_FOUND (-1)
#define TIME_ERR ((time_t)-1)
/* M        */
#define MAGIC_DBASE3      0x03   /* 00000011 */
#define MAGIC_DBASE3_MEMO 0x83   /* 10000011 */
#define MAGIC_DBASE3_MEMO_2 0x8B /* 10001011 */
#define MAGIC_DBASE4      0x04   /* 00000100 */
#define MAGIC_DBASE4_MEMO 0x84   /* 10000100 */
#define MAGIC_DBASE7      0x0C   /* 00001100 */
#define MAGIC_DBASE7_MEMO 0x8C   /* 10001100 */
#define MAGIC_FOXPRO      0x30   /* 00110000 */
#define MAGIC_DBASE_DEFAULT      MAGIC_DBASE3
#define MAGIC_DBASE_DEFAULT_MEMO MAGIC_DBASE3_MEMO

#define MEMO_BLOCK_SIZE   512         /*  memo block size (dBase III) */
#define MAGIC_MEMO_BLOCK 0x0008FFFF
#define FIELDTERMINATOR '\r'
#define FIELDTERMINATOR_LEN 1
#define RECORD_POS_DELETED 0
#define RECORD_POS_DATA    1
#define RECORD_DELETED_MARKER '*'
#define RECORD_INCREMENT_BIT 0x80
#define FIELD_FILL_CHAR ' '
#define HEADER_OFFSET_INDEX_FLAG 28

static void  strcpy_dos2host(char* buf, const char* src, size_t buf_len, enum dbf_charconv);
static void  strcpy_host2dos(char* buf, const char* src, size_t buf_len, enum dbf_charconv);
static char* strdup_host2dos(const char* src, size_t len, enum dbf_charconv, char* dup);

#pragma pack(1)
typedef struct _DBF_FILEHEADER_TIME
{
    uint8_t yy;
    uint8_t mm;
    uint8_t dd;
} DBF_FILEHEADER_TIME;

typedef struct _DBF_FILEHEADER_3
{
    uint8_t  flags;
    DBF_FILEHEADER_TIME lastupdate;
    uint32_t recordcount;
    uint16_t headerlength;
    uint16_t recordlength;

    uint16_t unused0;
    uint8_t  incomplete;
    uint8_t  crypt;

    uint8_t  unused1[16];
} DBF_FILEHEADER_3;

typedef struct _DBF_FILEHEADER_4
{
    uint8_t  flags;
    DBF_FILEHEADER_TIME lastupdate;
    uint32_t recordcount;
    uint16_t headerlength;
    uint16_t recordlength;

    uint16_t unused0;
    uint8_t  incomplete;
    uint8_t  crypt;

    uint8_t  unused1[16];
    uint8_t  unused2[36];
} DBF_FILEHEADER_4;

typedef union _DBF_FILEHEADER
{
    uint8_t flags;
    DBF_FILEHEADER_3 v3;
    DBF_FILEHEADER_4 v4;
} DBF_FILEHEADER;

typedef struct _DBT_FILEHEADER
{
    uint32_t next;
    uint8_t  unused0[4];
    char     title[8];
    uint8_t  flag;
    uint8_t  unused1[3];
    uint16_t blocksize;

    uint8_t  unused2[MEMO_BLOCK_SIZE-22];
} DBT_FILEHEADER;

typedef struct _DBF_FILEFIELD_3
{
    char      name[DBF_DBASE3_FIELDNAMELENGTH];   // field name in ASCII zero-filled
    char      type;        // field type in ASCII
    uint8_t   unused_0[4];
    uint8_t   length;     // field length in binary
    uint8_t   deccount;   // field decimal count in binary
    uint8_t   unused_1[14];
} DBF_FILEFIELD_3;

typedef struct _DBF_FILEFIELD_4
{
    char      name[DBF_DBASE4_FIELDNAMELENGTH];   // field name in ASCII zero-filled
    char      type;        // field type in ASCII
    uint8_t   length;     // field length in binary
    uint8_t   deccount;   // field decimal count in binary

    uint8_t   unused_0[2];
    uint8_t   mdx;
    uint8_t   unused_1[2];
    uint32_t  autoincrement;
    uint8_t   unused_2[4];
} DBF_FILEFIELD_4;

typedef union _DBF_FILEFIELD
{
    char name[1];
    DBF_FILEFIELD_3 v3;
    DBF_FILEFIELD_4 v4;
} DBF_FILEFIELD;

typedef struct _DBF_MEMO_BLOCK_3
{
    char text[MEMO_BLOCK_SIZE];
} DBF_MEMO_BLOCK_3;

typedef struct _DBF_MEMO_BLOCK_4
{
    uint32_t reserved; /* MAGIC_MEMO_BLOCK */
    uint32_t len;
    char     text[MEMO_BLOCK_SIZE-8];
} DBF_MEMO_BLOCK_4;

typedef union _DBF_MEMO_BLOCK
{
    DBF_MEMO_BLOCK_3 v3;
    DBF_MEMO_BLOCK_3 headerless;
    DBF_MEMO_BLOCK_4 v4;
    DBF_MEMO_BLOCK_4 normal;
} DBF_MEMO_BLOCK;

#pragma pack()

C_ASSERT_(1, sizeof(DBF_MEMO_BLOCK) == MEMO_BLOCK_SIZE);
C_ASSERT_(2, sizeof(DBF_FILEHEADER_3) == 32);
C_ASSERT_(3, sizeof(DBF_FILEHEADER_4) == 68);
C_ASSERT_(4, sizeof(DBT_FILEHEADER) == MEMO_BLOCK_SIZE);
C_ASSERT_(5, sizeof(DBF_FILEFIELD_3) == 32);
C_ASSERT_(6, sizeof(DBF_FILEFIELD_4) == 48);

typedef struct _DBF_MEMO_DATA
{
    void* stream;
    dbf_uint nextfreeblock;
    DBF_MEMO_BLOCK block;
    DBT_FILEHEADER header;
} DBF_MEMO_DATA;

typedef struct _DBF_DATA
{
    char tablename[40];

    dbf_uint currentrecord;         // current record in memory
    enum dbf_charconv charconv;
    enum dbf_editmode editmode;

    void* stream;
    zlib_filefunc_def api;

    uint8_t diskversion;
    dbf_uint recordcount;
    size_t recordlength;
    size_t headerlength;
    time_t lastupdate;

    BOOL modified;            // has database contents changed?
    BOOL index_file;
    struct _DBF_FIELD_DATA* fieldarray;         // linked list with field structure
    dbf_uint fieldcount;         // number of fields
    char* recorddataptr;         // pointer to current record struct

    DBF_MEMO_DATA memo;

    int  lasterror;
    char lasterrormsg[NAME_MAX];

    char* dup;
} DBF_DATA;

typedef uint32_t strhash_t;

typedef struct _DBF_FIELD_DATA
{
    char     m_Name[DBF_DBASE4_FIELDNAMELENGTH+1];
    enum dbf_data_type type;
    size_t   m_Length;
    dbf_uint m_DecCount;
    char*    ptr;
    strhash_t namehash;
} DBF_FIELD_DATA;

static uint32_t strhash(const char* str, BOOL case_sensitive)
{
    strhash_t hash = 5381;

    for (; *str; str++)
    {
        int c = case_sensitive ? *str : toupper(*str);

        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

int dbf_getlasterror(DBF_HANDLE handle)
{
    return handle->lasterror;
}

const char* dbf_getlasterror_str(DBF_HANDLE handle)
{
    return handle->lasterrormsg;
}

static BOOL dbf_memo_attach(DBF_HANDLE handle, void* stream)
{
    BOOL ok = (stream != NULL);

    if (ok)
    {
        handle->memo.stream = stream;
        ZSEEK(handle->api, stream, 0, ZLIB_FILEFUNC_SEEK_SET);
        ZREAD(handle->api, stream, &handle->memo.header, sizeof(handle->memo.header));
        if (0 == handle->memo.header.blocksize)
            handle->memo.header.blocksize = MEMO_BLOCK_SIZE; /* OpenOffice2 */
    }
    return ok;
}

EXTERN_C DBF_HANDLE dbf_alloc(void)
{
    DBF_DATA* handle = (DBF_DATA*)malloc(sizeof(DBF_DATA));

    handle->stream          = NULL;
    handle->fieldcount      = 0;
    handle->memo.nextfreeblock = 1;
    handle->memo.stream     = NULL;

    memset(&handle->memo.header, 0, sizeof(handle->memo.header));
    handle->memo.header.blocksize  = MEMO_BLOCK_SIZE;
    handle->memo.header.flag = MAGIC_DBASE3;
    handle->memo.header.next = 1;

    handle->fieldarray      = NULL;
    handle->modified        = FALSE;
    handle->recorddataptr   = NULL;
    handle->editmode        = dbf_editmode_readonly;
    handle->currentrecord   = (dbf_uint)-1;
    handle->lastupdate      = time(NULL);
    handle->recordcount     = 0;
    handle->recordlength    = 0;
    handle->headerlength    = 0;
    handle->lasterror       = DBASE_SUCCESS;
    handle->diskversion     = MAGIC_DBASE_DEFAULT;
    handle->dup             = NULL;
    *handle->lasterrormsg   = 0;
    *handle->tablename      = 0;
    fill_fopen_filefunc(&handle->api);
    handle->charconv = dbf_charconv_compatible;
    return handle;
}

static BOOL sanity_check_3(const DBF_FILEHEADER_3* header)
{
    BOOL ok =    ((header->lastupdate.mm >= 1) && (header->lastupdate.mm <= 12))
                 && ((header->lastupdate.dd >= 1) && (header->lastupdate.dd <= 31))
    ;
    return ok;
}

static BOOL sanity_check_4(const DBF_FILEHEADER_4* header)
{
    BOOL ok =    ((header->lastupdate.mm >= 1) && (header->lastupdate.mm <= 12))
                 && ((header->lastupdate.dd >= 1) && (header->lastupdate.dd <= 31))
    ;
    return ok;
}

DBF_HANDLE dbf_attach(void* stream, const zlib_filefunc_def* api, enum dbf_editmode editmode, enum dbf_charconv charconv, void* memo, const char* tablename)
{
    DBF_HANDLE handle = NULL;
    size_t len = 0;
    size_t read_len;
    DBF_FILEHEADER header;
    BOOL ok;

    ZSEEK(*api, stream, 0, ZLIB_FILEFUNC_SEEK_SET);
    read_len = sizeof(header.flags);
    len+=ZREAD(*api, stream, &header.flags, read_len);
    ok = (len == read_len);

    if (ok) switch (header.flags)
        {
            case MAGIC_DBASE4:
            case MAGIC_DBASE4_MEMO:
            case MAGIC_DBASE7:
            case MAGIC_DBASE7_MEMO:
                read_len = sizeof(header.v4) - sizeof(header.flags);
                len+=ZREAD(*api, stream, &header.v4.lastupdate, read_len);
                ok = sanity_check_4(&header.v4) && (len == sizeof(header.v4));
                break;
            case MAGIC_DBASE3:
            case MAGIC_DBASE3_MEMO:
            case MAGIC_DBASE3_MEMO_2:
            case MAGIC_FOXPRO:
            default:
                read_len = sizeof(header.v3) - sizeof(header.flags);
                len+=ZREAD(*api, stream, &header.v3.lastupdate, read_len);
                ok = sanity_check_3(&header.v3) && (len == sizeof(header.v3));
                break;
        }

    if (ok) switch (header.flags)
        {
            case MAGIC_DBASE3:
            case MAGIC_DBASE3_MEMO:
            case MAGIC_DBASE3_MEMO_2:
            case MAGIC_DBASE4:
            case MAGIC_DBASE4_MEMO:
            case MAGIC_FOXPRO:
            case MAGIC_DBASE7:
            case MAGIC_DBASE7_MEMO:
            {
                struct tm tm;
                size_t field_len;
                size_t header_len;
                size_t name_len;
                uint8_t index_file;

                handle = dbf_alloc();
                handle->stream = stream;
                handle->api = *api;
                tm.tm_sec   = 0;
                tm.tm_min   = 0;
                tm.tm_hour  = 0;
                tm.tm_wday  = 0;
                tm.tm_yday  = 0;
                tm.tm_isdst = 0;
                switch (header.flags)
                {
                    case MAGIC_DBASE4:
                    case MAGIC_DBASE4_MEMO:
                    case MAGIC_DBASE7:
                    case MAGIC_DBASE7_MEMO:
                        header_len = sizeof(DBF_FILEHEADER_4);
                        field_len  = sizeof(DBF_FILEFIELD_4);
                        name_len   = DBF_DBASE4_FIELDNAMELENGTH;
                        handle->diskversion  = header.v4.flags;
                        handle->recordcount  = header.v4.recordcount;
                        handle->recordlength = header.v4.recordlength;
                        handle->headerlength = header.v4.headerlength;
                        tm.tm_mday  = header.v4.lastupdate.dd;
                        tm.tm_mon   = header.v4.lastupdate.mm - 1;
                        tm.tm_year  = header.v4.lastupdate.yy;
                        break;
                    default:
                        header_len = sizeof(DBF_FILEHEADER_3);
                        field_len  = sizeof(DBF_FILEFIELD_3);
                        name_len   = DBF_DBASE3_FIELDNAMELENGTH;
                        handle->diskversion  = header.v3.flags;
                        handle->recordcount  = header.v3.recordcount;
                        handle->recordlength = header.v3.recordlength;
                        handle->headerlength = header.v3.headerlength;
                        tm.tm_mday  = header.v3.lastupdate.dd;
                        tm.tm_mon   = header.v3.lastupdate.mm - 1;
                        tm.tm_year  = header.v3.lastupdate.yy;
                        break;
                }
                ZSEEK(handle->api, stream, HEADER_OFFSET_INDEX_FLAG, ZLIB_FILEFUNC_SEEK_SET);
                ZREAD(handle->api, stream, &index_file, sizeof(index_file));
                handle->index_file = index_file ? TRUE : FALSE;

                if ((handle->recordcount == 0) && handle->recordlength)
                {
                    ZSEEK(handle->api, stream, 0, ZLIB_FILEFUNC_SEEK_END);
                    handle->recordcount = (dbf_uint)((ZTELL(handle->api, stream) - handle->headerlength) / handle->recordlength);
                }

                handle->lastupdate = mktime(&tm);
                handle->fieldcount = (uint8_t)((handle->headerlength - (header_len + FIELDTERMINATOR_LEN)) / field_len);
                handle->recorddataptr = (char*)malloc(handle->recordlength + 1); // + zeroterm.
                if (handle->recorddataptr)
                {
                    dbf_uint i;
                    size_t fielddata_pos = RECORD_POS_DATA;

                    ZSEEK(handle->api, stream, (ZPOS_T)header_len, ZLIB_FILEFUNC_SEEK_SET);

                    handle->fieldarray = (DBF_FIELD*)realloc(handle->fieldarray, sizeof(DBF_FIELD) * handle->fieldcount);
                    for (i = 0; i < handle->fieldcount; i++)
                    {
                        DBF_FILEFIELD temp;
                        DBF_FIELD* field = handle->fieldarray + i;

                        ZREAD(handle->api, stream, &temp, (uLong)field_len);

                        if ( (*temp.name >= 0) && (*temp.name <= ' '))
                        {
                            handle->fieldcount = i;
                            break;
                        }
                        strncpy(field->m_Name, temp.name, name_len);
                        field->m_Name[name_len] = 0;

                        switch (header.flags)
                        {
                            case MAGIC_DBASE4:
                            case MAGIC_DBASE4_MEMO:
                            case MAGIC_DBASE7:
                            case MAGIC_DBASE7_MEMO:
                                field->type       = temp.v4.deccount ? DBF_DATA_TYPE_FLOAT : dbf_gettype_int2ext(temp.v4.type);
                                field->m_Length   = temp.v4.length  ;
                                field->m_DecCount = temp.v4.deccount;
                                break;
                            default:
                                field->type       = temp.v3.deccount ? DBF_DATA_TYPE_FLOAT : dbf_gettype_int2ext(temp.v3.type);
                                field->m_Length   = temp.v3.length  ;
                                field->m_DecCount = temp.v3.deccount;
                                break;
                        }

                        field->ptr        = handle->recorddataptr + fielddata_pos;
                        field->namehash   = strhash(field->m_Name, FALSE);

                        fielddata_pos += field->m_Length;
                    }
                    handle->modified = FALSE;
                    handle->editmode = editmode;
                    handle->charconv = charconv;
                    if (tablename)
                        strncpy(handle->tablename, tablename, _countof(handle->tablename));

                    switch (header.flags)
                    {
                        case MAGIC_DBASE3_MEMO:
                        case MAGIC_DBASE3_MEMO_2:
                        case MAGIC_DBASE4_MEMO:
                        case MAGIC_DBASE7_MEMO:
                            dbf_memo_attach(handle, memo);
                            break;
                        default:
                            break;
                    }

                }
                else
                {
                    strncpy(handle->lasterrormsg, "Out of memory", _countof(handle->lasterrormsg));
                    handle->lasterror = DBASE_OUT_OF_MEM;
                    dbf_detach(&handle);
                }
                break;
            }
            default:
                break;
        }
    return handle;
}

void dbf_close_memo(DBF_HANDLE handle)
{
    ZCLOSE(handle->api, handle->memo.stream);
    handle->memo.stream = NULL;
}

void dbf_write_header(DBF_HANDLE handle)
{
    const time_t now     = time(NULL);
    const struct tm* ptm = localtime(&now);

    DBF_FILEHEADER_3 header;
    memset(&header, 0, sizeof(header));
    header.flags = handle->memo.stream ? MAGIC_DBASE_DEFAULT_MEMO : MAGIC_DBASE_DEFAULT;
    header.lastupdate.dd = (uint8_t) ptm->tm_mday;
    header.lastupdate.mm = (uint8_t)(ptm->tm_mon+1);
    header.lastupdate.yy = (uint8_t) ptm->tm_year;
    header.recordcount   = (uint16_t)handle->recordcount;
    header.headerlength  = (uint16_t)handle->headerlength;
    header.recordlength  = (uint16_t)handle->recordlength;

    // set file pointer to begin of file
    ZSEEK(handle->api, handle->stream, 0, ZLIB_FILEFUNC_SEEK_SET);

    ZWRITE(handle->api, handle->stream, &header, sizeof(header));

    // set pointer to end of file
    ZSEEK(handle->api, handle->stream, 0, ZLIB_FILEFUNC_SEEK_END);
    // write eof character
    //      ZWRITE(m_api, handle->stream, "\0x1a", 1);
}

void* dbf_detach(DBF_HANDLE* handle_ptr)
{
    DBF_HANDLE handle = *handle_ptr;
    void* stream = handle->stream;

    *handle_ptr = NULL;

    if (handle->modified)
        dbf_write_header(handle);
    if (handle->memo.stream)
        dbf_close_memo(handle);

    free(handle->dup);
    free(handle->fieldarray);
    free(handle->recorddataptr);
    free(handle);
    return stream;
}

void dbf_close(DBF_HANDLE* handle_ptr)
{
    DBF_HANDLE handle = *handle_ptr;
    zlib_filefunc_def api = handle->api;
    void* stream = dbf_detach(handle_ptr);

    if (stream)
        ZCLOSE(api, stream);
}

void dbf_getmemofilename(const char* file_dbf, char* buf, size_t buf_len)
{
    char name[PATH_MAX];
    size_t len;

    strncpy(name, file_dbf, sizeof(name));
    len = strlen(name) - 1;
    if (len)
        switch(name[len])
        {
            case 'F':
                name[len] = 'T'; break;
            default:
            case 'f':
                name[len] = 't'; break;
        }
    strncpy(buf, name, buf_len);
}

DBF_HANDLE dbf_open(const char* file, const DBF_OPEN* parm)
{
    DBF_HANDLE handle = NULL;
    void* stream = NULL;
    zlib_filefunc_def temp;
    const zlib_filefunc_def *api;
    int openmode;
    DBF_OPEN parm_default;

    if (parm == NULL)
    {
        parm_default.api = NULL;
        parm_default.charconv = dbf_charconv_default;
        parm_default.memo = TRUE;
        parm_default.tablename = NULL;
        parm_default.editmode = dbf_editmode_readonly;
        parm = &parm_default;
    }
    if (parm->api)
        api = parm->api;
    else
    {
        fill_fopen_filefunc(&temp);
        api = parm_default.api = &temp;
    }
    openmode = ZLIB_FILEFUNC_MODE_EXISTING | ( (parm->editmode == dbf_editmode_editable) ? ZLIB_FILEFUNC_MODE_WRITE : ZLIB_FILEFUNC_MODE_READ);
    stream = (*api->zopen_file)(api->opaque, file, openmode);
    if (stream)
    {
        void* memostream = NULL;
        char tmp[PATH_MAX];

        dbf_getmemofilename(file, tmp, _countof(tmp));
        if (parm->memo)
            memostream = (*api->zopen_file)(api->opaque, tmp, openmode);
        handle = dbf_attach(stream, api, parm->editmode, parm->charconv, memostream, parm->tablename);
        if (handle)
        {
            switch (handle->diskversion)
            {
                case MAGIC_DBASE3_MEMO:
                case MAGIC_DBASE3_MEMO_2:
                case MAGIC_DBASE4_MEMO:
                case MAGIC_DBASE7_MEMO:
                    break;
                default:
                    if (memostream)
                        ZCLOSE(*api, memostream);
                    break;
            }
        }
        else
        {
            ZCLOSE(*api, stream);
            if (memostream)
                ZCLOSE(*api, memostream);
        }
    }
    else
    {
        //strncpy(handle->lasterrormsg, "Unable to open file", _countof(handle->lasterrormsg));
        //handle->lasterror = DBASE_NO_FILE;
    }
    return handle;
}

dbf_uint dbf_getposition(DBF_HANDLE handle)
{
    return handle->currentrecord;
}

dbf_uint dbf_getrecordcount(DBF_HANDLE handle)
{
    return handle->recordcount;
}

dbf_uint dbf_getfieldcount(DBF_HANDLE handle)
{
    return handle->fieldcount;
}

/* Remove all trailing and leading spaces.                  */
static char* Trim(char *str, char trimchar)
{
    char* end;

    while (*str == trimchar)
        strcpy(str, str + 1);
    end = str + strlen(str) - 1;
    while ( (end >= str) && (*end == trimchar) )
        *(end--) = 0;
    return str;
}

void dbf_getinfo(DBF_HANDLE handle, DBF_INFO* info)
{
    switch (handle->diskversion & 0xF)
    {
        case MAGIC_DBASE3:
            info->version = 3;
            strncpy(info->format, DBF_FORMAT_NAME " 3", _countof(info->format));
            break;
        case MAGIC_DBASE4:
            info->version = 4;
            strncpy(info->format, DBF_FORMAT_NAME " 4", _countof(info->format));
            break;
        case MAGIC_DBASE7:
            info->version = 7;
            strncpy(info->format, DBF_FORMAT_NAME " 7", _countof(info->format));
            break;
        case 0x00:
        default:
            info->version = 0;
            strncpy(info->format, "Foxpro", _countof(info->format));
            break;
    }
    info->flags       = handle->diskversion;
    info->fieldcount  = handle->fieldcount;
    info->recordcount = handle->recordcount;
    info->lastupdate  = handle->lastupdate;
    info->memo_file   = (handle->memo.stream != NULL);
    info->index_file  = handle->index_file;
    info->editmode    = handle->editmode;
    info->modified    = handle->modified;
    strncpy(info->tablename, handle->tablename, _countof(info->tablename));
}

#define FMT_DATE     "%04d%02d%02d"
#define FMT_TIME     "%02d%02d%02d%03d"
#define FMT_DATETIME "%04d%02d%02d%02d%02d%02d%03d"

BOOL dbf_parsedate(const char *buf, struct tm* tm_ptr, int* ms_ptr, enum dbf_data_type type)
{
    BOOL ok;
    struct tm tm;
    int ms = 0;

    memset(&tm, 0, sizeof(tm));
    switch (type)
    {
        case DBF_DATA_TYPE_DATE:
            ok = (3 == sscanf(buf, FMT_DATE, &tm.tm_year, &tm.tm_mon, &tm.tm_mday));
            if (ok)
            {
                tm.tm_mon--;
                tm.tm_year-=1900;
                ok = (tm.tm_mon  >= 0) && (tm.tm_mon  <= 11)
                     && (tm.tm_mday >= 1) && (tm.tm_mday <= 31);
            }
            break;
        case DBF_DATA_TYPE_TIME:
            ok = (4 == sscanf(buf, FMT_TIME, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &ms));
            if (ok)
            {
                ok = (tm.tm_hour >= 0) && (tm.tm_hour <= 23)
                     && (tm.tm_min  >= 0) && (tm.tm_min  <= 59)
                     && (tm.tm_sec  >= 0) && (tm.tm_sec  <= 59)
                     && (ms         >= 0) && (ms         <= 999);
            }
            break;
        case DBF_DATA_TYPE_DATETIME:
            ok = (7 == sscanf(buf, FMT_DATETIME,
                              &tm.tm_year, &tm.tm_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &ms));
            if (ok)
            {
                tm.tm_mon--;
                tm.tm_year-=1900;
                ok = (tm.tm_mon  >= 0) && (tm.tm_mon  <= 11)
                     && (tm.tm_mday >= 1) && (tm.tm_mday <= 31)
                     && (tm.tm_hour >= 0) && (tm.tm_hour <= 23)
                     && (tm.tm_min  >= 0) && (tm.tm_min  <= 59)
                     && (tm.tm_sec  >= 0) && (tm.tm_sec  <= 59)
                     && (ms         >= 0) && (ms         <= 999);
            }
            break;
        default:
            ok = FALSE;
            break;
    }
    if (ok)
    {
        if (tm_ptr)
            *tm_ptr = tm;
        if (ms_ptr)
            *ms_ptr = ms;
    }
    return ok;
}

BOOL dbf_isvaliddate(const char *buf, enum dbf_data_type type)
{
    return dbf_parsedate(buf, NULL, NULL, type);
}

BOOL dbf_setposition(DBF_HANDLE handle, dbf_uint record)
{
    BOOL ok = (record < handle->recordcount);

    if (ok)
    {
        if (record != handle->currentrecord)
        {
            ok = (0 == ZSEEK(handle->api, handle->stream, (ZPOS_T)(handle->headerlength + (record - 0) * handle->recordlength), ZLIB_FILEFUNC_SEEK_SET));
            if (ok)
                ok = (handle->recordlength == ZREAD(handle->api, handle->stream, handle->recorddataptr, (uLong)handle->recordlength));
        }
    }
    if (ok)
    {
        handle->currentrecord = record;
        handle->lasterror = DBASE_SUCCESS;
    }
    else
    {
        strncpy(handle->lasterrormsg, "Invalid record", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_INVALID_RECORD;
    }
    return ok;
}

BOOL dbf_putrecord(DBF_HANDLE handle, dbf_uint record)
{
    BOOL ok = (record < handle->recordcount);

    if (ok)
    {
        ZSEEK (handle->api, handle->stream, (ZPOS_T)(handle->headerlength + (record - 0) * handle->recordlength), ZLIB_FILEFUNC_SEEK_SET);
        ZWRITE(handle->api, handle->stream, handle->recorddataptr, (uLong)handle->recordlength);
        handle->modified = TRUE;
        handle->currentrecord = record;
        handle->lasterror = DBASE_SUCCESS;
    }
    else
    {
        strncpy(handle->lasterrormsg, "Invalid record", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_INVALID_RECORD;
    }
    return ok;
}

BOOL dbf_isrecorddeleted(DBF_HANDLE handle)
{
    return (handle->recorddataptr[RECORD_POS_DELETED] == RECORD_DELETED_MARKER);
}

BOOL dbf_deleterecord(DBF_HANDLE handle, BOOL do_delete)
{
    BOOL ok = (NULL != handle->recorddataptr);

    if (ok)
    {
        handle->recorddataptr[RECORD_POS_DELETED] = (char)(do_delete ? RECORD_DELETED_MARKER : FIELD_FILL_CHAR);
        handle->modified = TRUE;
        ok = dbf_putrecord(handle, dbf_getposition(handle));
    }
    else
        handle->lasterror = DBASE_INVALID_RECORD;
    return ok;
}

BOOL dbf_addrecord(DBF_HANDLE handle)
{
    BOOL ok;

    memset(handle->recorddataptr, FIELD_FILL_CHAR, handle->recordlength);
    ZSEEK(handle->api, handle->stream, (ZPOS_T)(handle->headerlength + handle->recordcount * handle->recordlength), ZLIB_FILEFUNC_SEEK_SET);
    ok = (handle->recordlength == ZWRITE(handle->api, handle->stream, handle->recorddataptr, (uLong)handle->recordlength));
    if (ok)
    {
        handle->currentrecord = handle->recordcount++;
        handle->modified      = TRUE;
        handle->lasterror     = DBASE_SUCCESS;
    }
    else
    {
        strncpy(handle->lasterrormsg, "Error while adding new record", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_INVALID_RECORD;
    }
    return ok;
}

BOOL dbf_insertrecord(DBF_HANDLE handle, dbf_uint index)
{
    char buf[255];
    BOOL ok = (index <= handle->recordcount);

    if (ok)
    {
        dbf_uint i, j, total = handle->recordcount+1;

        /* add new record at the end of file */
        dbf_addrecord(handle);

        /* move all records one place */
        for (i = total; i > index; i--)
        {
            for (j = 0; j < handle->fieldcount; j++)
            {
                const DBF_FIELD* field;
                dbf_setposition(handle, i - 1);

                field = dbf_getfieldptr(handle, j);
                dbf_getfield(handle, field, buf, sizeof(buf), DBF_DATA_TYPE_ANY);
                dbf_setposition(handle, i);
                dbf_putfield(handle, field, buf);
                dbf_putrecord(handle, i);
            }
        }
        handle->modified = TRUE;
        handle->lasterror = DBASE_SUCCESS;
    }
    else
        handle->lasterror = DBASE_INVALID_RECORD;
    return ok;
}

const DBF_FIELD* dbf_getfieldptr(DBF_HANDLE handle, dbf_uint index)
{
    const DBF_FIELD* field = NULL;

    if (index < handle->fieldcount)
        field = handle->fieldarray + index;
    else
    {
        strncpy(handle->lasterrormsg, "Invalid field pointer", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_INVALID_FIELDNO;
    }
    return field;
}

int dbf_findfield(DBF_HANDLE handle, const char* fieldname_host)
{
    dbf_uint i;
    strhash_t namehash;
    char fieldname[20];

    strcpy_host2dos(fieldname, fieldname_host, _countof(fieldname), dbf_charconv_oem_host);
    namehash = strhash(fieldname, FALSE);

    for (i = 0; i < handle->fieldcount; i++)
    {
        const DBF_FIELD* temp = handle->fieldarray + i;

        if (   (namehash == temp->namehash)
               && (0 == strcasecmp(temp->m_Name, fieldname)))
        {
            return (int)i;
        }
    }
    strncpy(handle->lasterrormsg, "Invalid field pointer", _countof(handle->lasterrormsg));
    handle->lasterror = DBASE_INVALID_FIELDNO;
    return NOT_FOUND;
}

const DBF_FIELD* dbf_getfieldptr_name(DBF_HANDLE handle, const char* fieldname)
{
    const int index = dbf_findfield(handle, fieldname);

    return (index != NOT_FOUND) ? (handle->fieldarray + index) : NULL;
}

BOOL dbf_isnull(DBF_HANDLE handle, const DBF_FIELD* field)
{
    size_t i;

    for (i = 0; i < field->m_Length; i++)
    {
        if (field->ptr[i] != FIELD_FILL_CHAR)
            return FALSE;
    }
    return TRUE;
}

static int dotnormalize(char* str, char dot, size_t len)
{
    size_t i;

    if (0 == len)
        len = strlen(str);
    for (i = 0; i < len; i++) switch (str[i])
        {
            case ',':
            case '.':
                str[i] = (char)(dot ? dot : *localeconv()->decimal_point);
                return (int)i;
        }
    return NOT_FOUND;
}

void dbf_write_header_memo(DBF_HANDLE handle)
{
    ZSEEK (handle->api, handle->memo.stream, 0, ZLIB_FILEFUNC_SEEK_SET);
    ZWRITE(handle->api, handle->memo.stream, &handle->memo.header, sizeof(handle->memo.header));
}

BOOL dbf_putfield(DBF_HANDLE handle, const DBF_FIELD* field, const char* buf)
{
    BOOL ok = field && (handle->editmode == dbf_editmode_editable);

    if (ok)
    {
        const size_t buf_len = strlen(buf);
        char* dup = handle->dup = strdup_host2dos(buf, buf_len, handle->charconv, handle->dup);

        // check for correct type
        switch (field->type)
        {
            case DBF_DATA_TYPE_FLOAT:
            case DBF_DATA_TYPE_INTEGER:
                //case DBF_DATA_TYPE_MEMO:
            {
                char* ptr = dup;

                Trim(ptr, FIELD_FILL_CHAR);
                while (*ptr)
                {
                    if (strchr("+-.,0123456789", *ptr))
                        ptr++;
                    else
                    {
                        strncpy(handle->lasterrormsg, "Invalid type (not a FLOAT/NUMERIC)", _countof(handle->lasterrormsg));
                        handle->lasterror = DBASE_INVALID_DATA;
                        ok = FALSE;
                        break;
                    }
                }
                break;
            }
            case DBF_DATA_TYPE_DATE:
                if(!dbf_isvaliddate(dup, field->type))
                {
                    strncpy(handle->lasterrormsg, "Invalid type (not a DATE)", _countof(handle->lasterrormsg));
                    handle->lasterror = DBASE_INVALID_DATA;
                    ok = FALSE;
                }
                break;
            case DBF_DATA_TYPE_BOOLEAN:
                if (strchr("YyNnFfTt?", *dup))
                {
                }
                else switch (*dup)
                    {
                        case '0':
                            *dup = 'F'; break;
                        case '1':
                            *dup = 'T'; break;
                        default:
                            strncpy(handle->lasterrormsg, "Invalid type (not a LOGICAL)", _countof(handle->lasterrormsg));
                            handle->lasterror = DBASE_INVALID_DATA;
                            ok = FALSE;
                            break;
                    }
                break;
            default:
                break;
        }
        if (ok)
        {
            switch (field->type)
            {
                case DBF_DATA_TYPE_INTEGER:
                    snprintf(dup, buf_len + 1, "%d", atoi(dup)); // normalize
                    memset(field->ptr, FIELD_FILL_CHAR, field->m_Length);
                    strncpy(field->ptr, dup, field->m_Length);
                    break;
                case DBF_DATA_TYPE_MEMO:
                {
                    const int n = atoi(field->ptr);
                    size_t block = n;

                    ok = (NULL != handle->memo.stream);
                    if (ok) switch (n)
                        {
                            case -1:
                                break;
                            case 0:
                                block = handle->memo.header.next;
                                if (0 == block)
                                {
                                    size_t filelen;

                                    ZSEEK(handle->api, handle->memo.stream, 0, ZLIB_FILEFUNC_SEEK_END);
                                    filelen = (size_t)ZTELL(handle->api, handle->memo.stream);
                                    block =  (filelen / handle->memo.header.blocksize)
                                             + ((filelen % handle->memo.header.blocksize) ? 1 : 0);
                                }
                                handle->memo.header.next = (uint32_t)(block + 1);
                                dbf_write_header_memo(handle);
                                // fall through
                            default:
                            {
                                DBF_MEMO_BLOCK* temp = &handle->memo.block;
                                char* text = temp->normal.text;

                                ZSEEK(handle->api, handle->memo.stream, (ZPOS_T)(block * handle->memo.header.blocksize), ZLIB_FILEFUNC_SEEK_SET);
                                temp->normal.len = (uint32_t)_min(handle->memo.header.blocksize - offsetof(DBF_MEMO_BLOCK, normal.text), buf_len);
                                temp->normal.reserved = MAGIC_MEMO_BLOCK;
                                memset(text, FIELD_FILL_CHAR, temp->normal.len);
                                strncpy(text, dup, temp->normal.len);
                                ok = (sizeof(*temp) == ZWRITE(handle->api, handle->memo.stream, temp, sizeof(*temp)));
                                if (ok)
                                {
                                    memset(field->ptr, FIELD_FILL_CHAR, field->m_Length);
                                    snprintf(field->ptr, field->m_Length, "%08d", (int)block);
                                }
                                break;
                            }
                        }
                    break;
                }
                case DBF_DATA_TYPE_FLOAT:
                {
                    int sep;
                    dbf_uint i;

                    dotnormalize(dup, 0, buf_len);
                    i = snprintf(field->ptr, field->m_Length, "%f", atof(dup));
                    sep = dotnormalize(field->ptr, '.', field->m_Length);
                    if (sep != NOT_FOUND)
                    {
                        int pad = field->m_DecCount - (i - sep);
                        for (; (pad >= 0) && (i < field->m_Length); i++, pad--)
                            field->ptr[i] = '0';
                    }

                    for (; i < field->m_Length; i++)
                        field->ptr[i] = FIELD_FILL_CHAR;
                    break;
                }
                default:
                {
                    size_t nLength = _min(buf_len, field->m_Length);

                    memset(field->ptr, FIELD_FILL_CHAR, field->m_Length);
                    strncpy(field->ptr, dup, nLength);
                    break;
                }
            }
        }
    }
    else
    {
        strncpy(handle->lasterrormsg, "Not a valid field index", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_INVALID_FIELDNO;
    }
    return ok;
}

BOOL dbf_putfield_bool(DBF_HANDLE handle, const DBF_FIELD* field, BOOL b)
{
    return dbf_putfield(handle, field, b ? "T" : "F");
}

BOOL dbf_move_prepare(DBF_HANDLE handle)
{
    BOOL ok = TRUE;

    if (handle->modified)
    {
        //ok = dbf_putrecord(handle, handle->currentrecord);
    }
    return ok;
}

BOOL dbf_getfield_time(DBF_HANDLE handle, const DBF_FIELD* field, time_t* utc_ptr, int* ms_ptr)
{
    BOOL ok = (field != NULL);

    if (ok)
    {
        switch (field->type)
        {
            case DBF_DATA_TYPE_DATE:
            {
                struct tm tm;
                int ms;

                ok = dbf_getfield_tm(handle, field, &tm, &ms);
                if (ok)
                {
                    time_t utc = mktime(&tm);

                    ok = (TIME_ERR != utc);
                    if (ok)
                    {
                        if (utc_ptr)
                            *utc_ptr = utc;
                        if (ms_ptr)
                            *ms_ptr  = ms;
                    }
                }
                break;
            }
            case DBF_DATA_TYPE_INTEGER:
            case DBF_DATA_TYPE_MEMO:
            {
                long temp;

                ok = dbf_getfield_numeric(handle, field, &temp);
                if (ok)
                {
                    if (utc_ptr)
                        *utc_ptr = temp;
                    if (ms_ptr)
                        *ms_ptr  = 0;
                }
                break;
            }
            case DBF_DATA_TYPE_FLOAT:
            {
                double temp;

                ok = dbf_getfield_float(handle, field, &temp);
                if (ok)
                {
                    if (utc_ptr)
                        *utc_ptr = (int)floor(temp);
                    if (ms_ptr)
                        *ms_ptr  = (int)((temp - floor(temp)) * 1000.0);
                }
                break;
            }
            default:
                ok = FALSE;
                break;
        }
    }
    return ok;
}

BOOL dbf_putfield_time(DBF_HANDLE handle, const DBF_FIELD* field, time_t utc, int ms, enum dbf_data_type type)
{
    BOOL ok = (field != NULL);

    if (type < 0)
        type = field->type;
    if (ok) switch (type)
        {
            case DBF_DATA_TYPE_DATE:
            case DBF_DATA_TYPE_TIME:
            case DBF_DATA_TYPE_DATETIME:
            {
                struct tm* tm = localtime(&utc);

                ok = (tm != NULL);
                if (ok)
                    ok = dbf_putfield_tm(handle, field, tm, ms, type);
                break;
            }
            case DBF_DATA_TYPE_INTEGER:
            case DBF_DATA_TYPE_MEMO:
                ok = dbf_putfield_numeric(handle, field, (long)utc);
                break;
            case DBF_DATA_TYPE_FLOAT:
                ok = dbf_putfield_float(handle, field, ((double)utc) + ((double)ms)/1000);
                break;
            default:
                ok = FALSE;
                break;
        }
    return ok;
}

BOOL dbf_getfield_tm(DBF_HANDLE handle, const DBF_FIELD* field, struct tm* tm, int* ms)
{
    BOOL ok = (field != NULL);

    if (ok)
    {
        switch (field->type)
        {
            case DBF_DATA_TYPE_DATE:
            case DBF_DATA_TYPE_TIME:
            case DBF_DATA_TYPE_DATETIME:
            {
                char temp[80];

                ok = 0 != dbf_getfield(handle, field, temp, _countof(temp), DBF_DATA_TYPE_ANY);
                if (ok)
                    ok = dbf_parsedate(temp, tm, ms, field->type);
                break;
            }
            case DBF_DATA_TYPE_INTEGER:
            case DBF_DATA_TYPE_MEMO:
            case DBF_DATA_TYPE_FLOAT:
            {
                time_t utc;

                ok = dbf_getfield_time(handle, field, &utc, ms);
                if (ok)
                {
                    const struct tm* temp = localtime(&utc);

                    ok = (temp != NULL);
                    if (ok)
                    {
                        if (tm)
                            *tm = *temp;
                        if (ms)
                            *ms = 0;
                    }
                }
                break;
            }
            default:
                ok = FALSE;
                break;
        }
    }
    return ok;
}

BOOL dbf_putfield_tm(DBF_HANDLE handle, const DBF_FIELD* field, const struct tm* tm, int ms, enum dbf_data_type type)
{
    BOOL ok = tm && field;

    if (type < 0)
        type = field->type;

    if (ok) switch (type)
        {
            case DBF_DATA_TYPE_DATE:
            {
                char sz[80];

                snprintf(sz, sizeof(sz), FMT_DATE,
                         tm->tm_year + ((tm->tm_year < 1900) ? 1900 : 0),
                         tm->tm_mon + 1,
                         tm->tm_mday);
                ok = dbf_putfield(handle, field, sz);
                break;
            }
            case DBF_DATA_TYPE_TIME: /* non-standard */
            {
                char sz[80];

                snprintf(sz, sizeof(sz), FMT_TIME,
                         tm->tm_hour,
                         tm->tm_min,
                         tm->tm_sec,
                         ms % 1000);
                ok = dbf_putfield(handle, field, sz);
                break;
            }
            case DBF_DATA_TYPE_DATETIME: /* non-standard */
            {
                char sz[80];

                snprintf(sz, sizeof(sz), FMT_DATETIME,
                         tm->tm_year + ((tm->tm_year < 1900) ? 1900 : 0),
                         tm->tm_mon + 1,
                         tm->tm_mday,
                         tm->tm_hour,
                         tm->tm_min,
                         tm->tm_sec,
                         ms % 1000);
                ok = dbf_putfield(handle, field, sz);
                break;
            }
            case DBF_DATA_TYPE_INTEGER:
            case DBF_DATA_TYPE_MEMO:
            case DBF_DATA_TYPE_FLOAT:
            {
                const time_t utc = mktime((struct tm*)tm); // unconst due to bad prototype in MSVC7

                ok = (TIME_ERR != utc);
                if (ok)
                    ok = dbf_putfield_time(handle, field, utc, ms, type);
                break;
            }
            default:
                ok = FALSE;
                break;
        }
    return ok;
}

BOOL dbf_getfield_float(DBF_HANDLE handle, const DBF_FIELD* field, double* d)
{
    char buf[21];
    BOOL ok = (0 != dbf_getfield(handle, field, buf, sizeof(buf), DBF_DATA_TYPE_FLOAT));

    if (ok)
    {
        dotnormalize(buf, 0, 0);
        if (d)
            *d = atof(buf);
    }
    else
    {
        strncpy(handle->lasterrormsg, "Invalid type (not a FLOAT)", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_BAD_FORMAT;
    }
    return ok;
}

BOOL dbf_getfield_bool(DBF_HANDLE handle, const DBF_FIELD* field, BOOL* b)
{
    char buf;
    BOOL ok = (0 != dbf_getfield(handle, field, &buf, sizeof(buf), DBF_DATA_TYPE_BOOLEAN));

    if (ok)
    {
        if(b)
            *b = (strchr("YyTt", buf) != NULL);
    }
    else
    {
        strncpy(handle->lasterrormsg, "Invalid type (not a LOGICAL)", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_BAD_FORMAT;
    }
    return ok;
}

BOOL dbf_putfield_float(DBF_HANDLE handle, const DBF_FIELD* field, double value)
{
    BOOL ok = (field != NULL);

    if (ok)
    {
        char* buf = (char*)malloc(field->m_Length);

        snprintf(buf, field->m_Length, "%g", value);
        ok = dbf_putfield(handle, field, buf);
        free(buf);
    }
    return ok;
}

BOOL dbf_putfield_numeric(DBF_HANDLE handle, const DBF_FIELD* field, long value)
{
    char buf[19];

    snprintf(buf, _countof(buf), "%ld", value);
    return dbf_putfield(handle, field, buf);
}

static const char mod_type_int[] = "CNFDCCML";
C_ASSERT_(7, (sizeof(mod_type_int) - 1) == DBF_DATA_TYPE_ENUMCOUNT);

char dbf_gettype_ext2int(enum dbf_data_type type)
{
    char n;

    switch (type)
    {
        case DBF_DATA_TYPE_UNKNOWN:
            n = 0; break;
        case DBF_DATA_TYPE_ANY:
            n = '?'; break;
        default:
            n = mod_type_int[type]; break;
    }
    return n;
}

enum dbf_data_type dbf_gettype_int2ext(char type)
{
    switch (type)
    {
        case 'I':
            return DBF_DATA_TYPE_INTEGER;
        default:
        {
            const char* p = strchr(mod_type_int, type);
            return p ? (enum dbf_data_type)(p - mod_type_int) : DBF_DATA_TYPE_UNKNOWN;
        }
    }
}

enum dbf_data_type dbf_getfield_type(DBF_HANDLE handle, const DBF_FIELD* field)
{
    return field ? field->type : DBF_DATA_TYPE_UNKNOWN;
}

BOOL dbf_getfield_info(DBF_HANDLE handle, dbf_uint index, DBF_FIELD_INFO* info)
{
    BOOL ok = (index < handle->fieldcount);

    if (ok && info)
    {
        const DBF_FIELD* field = handle->fieldarray + index;

        //strncpy(info->name, field->m_Name, sizeof(info->name));
        strcpy_dos2host(info->name, field->m_Name, sizeof(info->name), handle->charconv);
        info->length   = field->m_Length;
        info->type     = field->type;
        info->decimals = field->m_DecCount;
    }
    return ok;
}

const char* dbf_gettypetext(enum dbf_data_type index)
{
    const char* const asz[] =
            {
                    "CHAR",
                    "NUMERIC",
                    "FLOAT",
                    "DATE",
                    "CHAR",
                    "CHAR",
                    "MEMO",
                    "LOGICAL"
            };
    C_ASSERT_(2, _countof(asz) == DBF_DATA_TYPE_ENUMCOUNT);
    return (index < DBF_DATA_TYPE_ENUMCOUNT) ? asz[index] : NULL;
}

BOOL dbf_memo_create(DBF_HANDLE handle, void* stream)
{
    BOOL ok = dbf_memo_attach(handle, stream);

    if (ok)
    {
        dbf_uint i;

        ZSEEK(handle->api, handle->memo.stream, 0, ZLIB_FILEFUNC_SEEK_SET);
        handle->memo.nextfreeblock = 1;

        // first 4 bytes is next block
        ZWRITE(handle->api, handle->memo.stream, &handle->memo.nextfreeblock, sizeof(handle->memo.nextfreeblock));

        // reserved space
        for (i = 4; i < 512; i++)
            ZWRITE(handle->api, handle->memo.stream, "\0", 1);

        ZSEEK (handle->api, handle->memo.stream, 16, ZLIB_FILEFUNC_SEEK_SET);
        ZWRITE(handle->api, handle->memo.stream, "\0x03", 1);

        //handle->memo.dataptr = (char*)malloc(handle->memo.blocksize);
        //ok = (handle->memo.dataptr != NULL);
        if (ok)
        {
            *handle->lasterrormsg = 0;
            handle->lasterror = DBASE_SUCCESS;
        }
        else
        {
            ZCLOSE(handle->api, handle->memo.stream);
            handle->memo.stream = NULL;
            strncpy(handle->lasterrormsg, "Out of memory", _countof(handle->lasterrormsg));
            handle->lasterror = DBASE_OUT_OF_MEM;
        }
    }
    else
    {
        *handle->lasterrormsg = 0;
        handle->lasterror = DBASE_NO_FILE;
    }
    return ok;
}

BOOL dbf_memo_open(DBF_HANDLE handle, void* stream)
{
    BOOL ok = dbf_memo_attach(handle, stream);

    if (ok)
    {
        ZSEEK(handle->api, handle->memo.stream, 0, ZLIB_FILEFUNC_SEEK_SET);
        ZREAD(handle->api, handle->memo.stream, &handle->memo.nextfreeblock, sizeof(handle->memo.nextfreeblock));
        if (!ok)
        {
            ZCLOSE(handle->api, handle->memo.stream);
            handle->memo.stream = NULL;
            strncpy(handle->lasterrormsg, "Out of memory", _countof(handle->lasterrormsg));
            handle->lasterror = DBASE_OUT_OF_MEM;
        }
    }
    else
    {
        *handle->lasterrormsg = 0;
        handle->lasterror = DBASE_NO_FILE;
    }
    return ok;
}

static int find_memo(const DBF_FIELD_INFO* array, dbf_uint array_count)
{
    dbf_uint i;

    for (i = 0; i < array_count; i++)
    {
        if (array[i].type == DBF_DATA_TYPE_MEMO)
            return (int)i;
    }
    return NOT_FOUND;
}

DBF_HANDLE dbf_create(const char* filePath, const DBF_CREATE* parm_ptr)
{
    DBF_HANDLE handle = NULL;
    void* stream;
    void* memoStream = NULL;
    zlib_filefunc_def temp;
    int openmode = ZLIB_FILEFUNC_MODE_CREATE | ZLIB_FILEFUNC_MODE_WRITE;
    DBF_CREATE parm;

    DBF_OPEN open_parm;
    open_parm.memo   = FALSE;
    open_parm.api    = NULL;
    open_parm.charconv = dbf_charconv_compatible;
    open_parm.tablename = NULL;
    open_parm.editmode = dbf_editmode_editable;

    if (parm_ptr && parm_ptr->array_count)
    {
        parm = *parm_ptr;
        if (parm.api)
            open_parm.api = parm.api;
        else
        {
            fill_fopen_filefunc(&temp);
            open_parm.api = parm.api = &temp;
        }
        stream = (*open_parm.api->zopen_file)(open_parm.api->opaque, filePath, openmode);

        open_parm.memo = (NOT_FOUND != find_memo(parm.array, parm.array_count));
        if (stream && open_parm.memo)
        {
            char tmp[PATH_MAX];

            dbf_getmemofilename(filePath, tmp, _countof(tmp));
            memoStream = (*open_parm.api->zopen_file)(open_parm.api->opaque, tmp, openmode);
            if (NULL == memoStream)
            {
                ZCLOSE(*open_parm.api, stream);
                stream = NULL;
            }
        }
        if (stream)
        {
            handle = dbf_create_attach(stream, &parm, memoStream);
            if (handle)
            {
                dbf_close(&handle); // ioapi quirk - change mode to ZLIB_FILEFUNC_MODE_EXISTING | ZLIB_FILEFUNC_MODE_WRITE
                handle = dbf_open(filePath, &open_parm);
            }
            else
                ZCLOSE(*open_parm.api, stream);
        }
    }
    return handle;
}

DBF_HANDLE dbf_create_attach(void* stream, const DBF_CREATE* parm, void* memoStream)
{
    DBF_HANDLE handle = NULL;

    if (parm && parm->array_count)
    {
        const time_t now = time(NULL);
        const struct tm* ptm = localtime(&now);
        dbf_uint i;
        DBF_FILEHEADER_3 header;
        DBF_FIELD* fieldarray;
        char* recorddataptr;
        char ch;
        const struct zlib_filefunc_def_s* api = parm->api;

        memset(&header, 0, sizeof(header));
        switch (parm->version)
        {
            case 4:
                header.flags = (uint8_t)(memoStream ? MAGIC_DBASE4_MEMO : MAGIC_DBASE4);
                break;
            case 3:
            default:
                header.flags = (uint8_t)(memoStream ? MAGIC_DBASE3_MEMO : MAGIC_DBASE3);
                break;
        }
        header.lastupdate.dd = (uint8_t)(ptm->tm_mday );
        header.lastupdate.mm = (uint8_t)(ptm->tm_mon+1);
        header.lastupdate.yy = (uint8_t)(ptm->tm_year );
        header.recordcount   = 0;
        header.headerlength  = 0;
        header.recordlength  = 1;

        fieldarray = (DBF_FIELD*)malloc(parm->array_count*sizeof(DBF_FIELD));
        for (i = 0; i < parm->array_count; i++)
        {
            const DBF_FIELD_INFO* src = parm->array + i;
            DBF_FIELD     * dst = fieldarray + i;
            const size_t len = _min(src->length, UINT8_MAX);

            //dst->index = i;
            strncpy(dst->m_Name, src->name, sizeof(dst->m_Name));
            dst->type         = src->type;
            dst->m_DecCount   = (uint8_t)src->decimals;
            dst->m_Length     = (uint8_t)len;
            dst->ptr = NULL;
            header.recordlength = (uint16_t)(header.recordlength + src->length);
        }
        recorddataptr = (char*)malloc(header.recordlength + 1); // + zeroterm.
        header.lastupdate.dd = (uint8_t) ptm->tm_mday;
        header.lastupdate.mm = (uint8_t)(ptm->tm_mon+1);
        header.lastupdate.yy = (uint8_t) ptm->tm_year;

        ZSEEK(*api, stream, 0, ZLIB_FILEFUNC_SEEK_SET);
        ZWRITE(*api, stream, &header, sizeof(header));

        header.headerlength = (uint16_t)(sizeof(DBF_FILEHEADER_3) + (parm->array_count * sizeof(DBF_FILEFIELD_3)) + FIELDTERMINATOR_LEN);
        header.recordlength = RECORD_POS_DATA;

        for (i = 0; i < parm->array_count; i++)
        {
            DBF_FIELD* field = fieldarray + i;
            DBF_FILEFIELD_3 temp;

            memset(&temp, 0, sizeof(temp));

            strcpy_host2dos(temp.name, field->m_Name, sizeof(temp.name), parm->charconv);

            temp.type     = dbf_gettype_ext2int(field->type);
            temp.length   = (uint8_t)field->m_Length;
            temp.deccount = (uint8_t)field->m_DecCount;
            field->ptr      = recorddataptr + header.recordlength;
            field->namehash = strhash(field->m_Name, FALSE);

            ZWRITE(*api, stream, &temp, sizeof(temp));
            header.recordlength = (uint16_t)(header.recordlength + field->m_Length);
        }
        ch = FIELDTERMINATOR;
        ZWRITE(*api, stream, &ch, sizeof(ch)); // crucial - reqd by OpenOffice 2
        ZSEEK (*api, stream, 0, ZLIB_FILEFUNC_SEEK_SET);
        ZWRITE(*api, stream, &header, sizeof(header));
        ZSEEK (*api, stream, 0, ZLIB_FILEFUNC_SEEK_END);
        ch = FIELDTERMINATOR;
        ZWRITE(*api, stream, &ch, sizeof(ch));
        ch = CPM_TEXT_TERMINATOR;
        ZWRITE(*api, stream, &ch, sizeof(ch));

        handle = dbf_alloc();
        handle->api          = *api;
        handle->stream       = stream;
        handle->diskversion  = header.flags;
        handle->charconv     = parm->charconv;
        handle->editmode     = dbf_editmode_editable;
        handle->fieldarray   = fieldarray;
        handle->fieldcount   = parm->array_count;
        handle->recorddataptr = recorddataptr;
        handle->recordlength = header.recordlength;
        handle->headerlength = header.headerlength;
    }
    return handle;
}

BOOL dbf_copy(DBF_HANDLE handle,
              void* stream,
              void* stream_memo,
              const zlib_filefunc_def* api,
              BOOL include_records,
              BOOL include_deleted_records)
{
    BOOL ok = FALSE;

    return ok;
}

void dbf_getfileapi(DBF_HANDLE handle, zlib_filefunc_def* api)
{
    *api = handle->api;
}

void* dbf_getmemofile(DBF_HANDLE handle)
{
    return handle->memo.stream;
}

enum dbf_editmode dbf_geteditmode(DBF_HANDLE handle)
{
    return handle->editmode;
}

BOOL dbf_ismodified(DBF_HANDLE handle)
{
    return handle->modified;
}

size_t dbf_getfield(DBF_HANDLE handle, const DBF_FIELD* field, char* buf, size_t buf_len, enum dbf_data_type type)
{
    BOOL trim = TRUE;
    size_t len = 0;

    if (field)
    {
        if ( (field->type == type) || (type == DBF_DATA_TYPE_ANY))
        {
            switch (field->type)
            {
                case DBF_DATA_TYPE_MEMO:
                {
                    const char term = field->ptr[field->m_Length];
                    int n;

                    field->ptr[field->m_Length] = 0;
                    n = atoi(field->ptr);
                    field->ptr[field->m_Length] = term;
                    if (handle->memo.stream && (n >= 1))
                    {
                        const dbf_uint block = (dbf_uint)n;
                        size_t read;
                        DBF_MEMO_BLOCK* temp = &handle->memo.block;

                        ZSEEK(handle->api, handle->memo.stream, block * handle->memo.header.blocksize, ZLIB_FILEFUNC_SEEK_SET);
                        read = ZREAD(handle->api, handle->memo.stream, temp, sizeof(*temp));
                        if (read)
                        {
                            const char* text;
                            size_t i;

                            switch (temp->normal.reserved)
                            {
                                case MAGIC_MEMO_BLOCK:
                                    text = temp->normal.text;
                                    len = read - offsetof(DBF_MEMO_BLOCK, normal.text);
                                    len = _min(len, handle->memo.block.normal.len);
                                    break;
                                default:
                                    text = temp->headerless.text;
                                    len = read;
                                    break;
                            }
                            for (i = 0; i < len; i++) switch (text[i])
                                {
                                    case CPM_TEXT_TERMINATOR:
                                        //if (temp->normal.reserved == MAGIC_MEMO_BLOCK) break;
                                    case 0:
                                        len = i;
                                        break;
                                }
                            for (; trim && len && (text[len - 1] == FIELD_FILL_CHAR); len--)
                            {
                            }
                            if (buf)
                            {
                                len = _min(len, buf_len - (buf_len ? 1 : 0));
                                strcpy_dos2host(buf, text, len, handle->charconv);
                            }
                        }
                        else
                        {
                            strncpy(handle->lasterrormsg, "An error occured while reading from memo file", _countof(handle->lasterrormsg));
                            handle->lasterror = DBASE_READ_ERROR;
                        }
                    }
                    else
                    {
                        strncpy(handle->lasterrormsg, "No memo data available", _countof(handle->lasterrormsg));
                        handle->lasterror = DBASE_NO_MEMO_DATA;
                    }
                    break;
                }
                default:
                    for (len = field->m_Length; trim && len && (field->ptr[len - 1] == FIELD_FILL_CHAR); len--)
                    {
                    }
                    if (buf)
                    {
                        switch (field->type)
                        {
                            case DBF_DATA_TYPE_FLOAT:
                                strncpy(buf, field->ptr, len);
                                dotnormalize(buf, 0, len);
                                break;
                            case DBF_DATA_TYPE_INTEGER: /* DBASE7 */
                                if (*field->ptr & RECORD_INCREMENT_BIT)
                                {
                                    int val = 0;
                                    int pos = 0;

                                    val += (uint8_t)(field->ptr[pos++] & ~RECORD_INCREMENT_BIT);
                                    val <<= 8;
                                    val += (uint8_t)(field->ptr[pos++]);
                                    val <<= 8;
                                    val += (uint8_t)(field->ptr[pos++]);
                                    val <<= 8;
                                    val += (uint8_t)(field->ptr[pos++]);

                                    snprintf(buf, len, "%d", val);
                                    break;
                                }
                                /* else fall through */
                            default:
                                strcpy_dos2host(buf, field->ptr, len, handle->charconv);
                                break;
                        }
                    }
                    break;
            }
        }
        else
        {
            strncpy(handle->lasterrormsg, "Unexpected field type", _countof(handle->lasterrormsg));
            handle->lasterror = DBASE_INVALID_FIELDNO;
        }
    }
    else
    {
        strncpy(handle->lasterrormsg, "Invalid field index", _countof(handle->lasterrormsg));
        handle->lasterror = DBASE_INVALID_FIELDNO;
    }
    if (buf && (len < buf_len) && buf_len)
    {
        dbf_uint i;

        buf[len] = 0;
        for (i = 0; len && (FIELD_FILL_CHAR == *buf); i++, len--)
            strcpy(buf, buf + 1);
    }
    return len;
}

BOOL dbf_getfield_numeric(DBF_HANDLE handle, const DBF_FIELD* field, long* n)
{
    char buf[19];
    BOOL ok =
            (   dbf_getfield(handle, field, buf, sizeof(buf), DBF_DATA_TYPE_INTEGER)
                || dbf_getfield(handle, field, buf, sizeof(buf), DBF_DATA_TYPE_DATE   ));

    if (ok)
        *n = atol(buf);
    else
    {
        ok = (0 != dbf_getfield(handle, field, buf, sizeof(buf), DBF_DATA_TYPE_MEMO)); // get field data, check for MEMO type
        if (ok)
            *n = atol(buf);
        else
        {
            strncpy(handle->lasterrormsg, "Invalid field type (not a NUMERIC)", _countof(handle->lasterrormsg));
            handle->lasterror = DBASE_INVALID_FIELDNO;
        }
    }
    return ok;
}

#ifdef _WIN32
#include <windows.h> // OemToCharBuffA
#endif

static void strcpy_dos2host(char* buf, const char* src, size_t buf_len, enum dbf_charconv mode)
{
    switch (mode)
    {
        case dbf_charconv_oem_host:
#ifdef _WIN32
            OemToCharBuffA(src, buf, (DWORD)buf_len);
            break;
#endif
        case dbf_charconv_off:
        default:
            strncpy(buf, src, buf_len);
            break;
    }
}

static void strcpy_host2dos(char* buf, const char* src, size_t buf_len, enum dbf_charconv mode)
{
    switch (mode)
    {
        case dbf_charconv_oem_host:
#ifdef _WIN32
            CharToOemBuffA(src, buf, (DWORD)buf_len);
            break;
#endif
        case dbf_charconv_off:
        default:
            strncpy(buf, src, buf_len);
            break;
    }
}

static char* strdup_host2dos(const char* src, size_t len, enum dbf_charconv mode, char* dup)
{
    dup = (char*)realloc(dup, (1 + len)*sizeof(char));
    if (len)
        strcpy_host2dos(dup, src, len + 1, mode);
    else
        *dup = 0;
    return dup;
}

BOOL dbf_setflag(DBF_HANDLE handle, enum dbf_flag flag, BOOL set)
{
    if (handle->editmode == dbf_editmode_readonly)
        return FALSE;
    switch (flag)
    {
        case DBF_FLAG_INDEXFILE:
        {
            uint8_t index_file;

            handle->index_file = set;
            index_file = handle->index_file ? 1 : 0;
            ZSEEK(handle->api, handle->stream, HEADER_OFFSET_INDEX_FLAG, ZLIB_FILEFUNC_SEEK_SET);
            ZWRITE(handle->api, handle->stream, &index_file, sizeof(index_file));
            return TRUE;
        }
        case DBF_FLAG_MEMOFILE:
        default:
            return FALSE;
    }
}
