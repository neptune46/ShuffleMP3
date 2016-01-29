
#pragma once

#include "stdafx.h"
#include "MP3Info.h"

#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  0x0F
#define ATTR_LONG_NAME_MASK     (ATTR_READ_ONLY | \
                                 ATTR_HIDDEN    | \
                                 ATTR_SYSTEM    | \
                                 ATTR_VOLUME_ID | \
                                 ATTR_DIRECTORY | \
                                 ATTR_ARCHIVE) 

#define MAX_LENGTH      256
#define LAST_LONG_ENTRY 0x40


#pragma pack(push, 1)

typedef struct _FATBootSector
{
    char BS_jmpBoot[3];
    char BS_OEMName[8];
    WORD BPB_BytsPerSec;
    BYTE PBP_SecPerClus;
    WORD BPB_RsvdSecCnt;
    BYTE BPB_NumFATs;
    WORD BPB_RootEntCnt;
    WORD BPB_TotSec16;
    BYTE BPB_Media;
    WORD BPB_FATsz16;
    WORD BPB_SecPerTrk;
    WORD BPB_NumHeads;
    DWORD BPB_HiddSec;
    DWORD BPB_TotSec32;

    DWORD BPB_FATsz32;
    WORD BPB_ExtFlags;
    WORD BPB_FSVer;
    DWORD BPB_RootClus;
    WORD BPB_FSInfo;
    WORD BPB_BkBootSec;
    BYTE BPB_Reserved[12];
    BYTE BS_DrvNum;
    BYTE BS_Reserverd1;
    BYTE BS_BootSig;
    DWORD BS_VolID;
    BYTE BS_VolLab[11];
    char BS_FilSysType[8];
} FATBootSector;

typedef struct _FATDirEntry
{
    char DIR_Name[11];
    BYTE DIR_Attr;
    BYTE DIR_NTRes;
    BYTE DIR_CrtTimeTenth;
    WORD DIR_CrtTime;
    WORD DIR_CrtDate;
    WORD DIR_LstAccDate;
    WORD DIR_FstClusHi;
    WORD DIR_WrtTime;
    WORD DIR_WrtDate;
    WORD DIR_FstClusLo;
    DWORD DIR_FileSize;
} FATDirEntry;

typedef struct _LFNEntry
{
    BYTE LDIR_Ord;
    WORD LDIR_Name1[5];
    BYTE LDIR_ATT;
    BYTE LDIR_Type;
    BYTE LDIR_Chksum;
    WORD LDIR_Name2[6];
    WORD LDIR_FstClusLO;
    WORD LDIR_Name3[2];
} LFNEntry;

typedef union _FATDirEntryUn
{
    FATDirEntry ShortEntry;
    LFNEntry    LongEntry;
    BYTE        RawData[0x20];
} FATDirEntryUn;

#pragma pack(pop)

enum EntryType
{
    INVALID_TYPE    = 0,
    SPECIAL_FOLDER,
    NORMAL_FOLDER,
    NORMAL_FILE_MP3,
    NORMAL_FILE_OTHER,
    DELETE_FOLDER,
    DELETE_FILE,
    MAX_TYPE
};

enum SortType
{
    BY_FILENAME = 0,
    BY_TITLE,
    BY_ARTIST,
    BY_ALBUM,
    BY_DURATION,
    MAX_SORT_TYPE
};

struct DirEntryItem
{
    EntryType type;
    BYTE* pEntryData;
    DWORD dwEntrySize;
    WCHAR* pEntryName;
    WCHAR* pFileOrDir;
    WCHAR* pShortOrLong;
    WCHAR* pStatus;
    MP3Info info;
};

class CFat32Partition
{
public:
    CFat32Partition(TCHAR* pDriveLetter);
    CFat32Partition::CFat32Partition() 
    {
        m_hDevice = 0;
        m_FAT1Data = NULL;
        m_FAT2Data = NULL;
        m_pRootDirData = NULL;
    }
    ~CFat32Partition();

public:
    void    setDrivePath(const TCHAR* pPath);
    TCHAR*  getDriveLable() { return m_pDriveLable; }
    DWORD   loadRootDirEntry();
    DWORD   shuffleMP3Files();
    void    sortMP3Files(SortType byType);
    DWORD   flushRootDirToDevice();
    DWORD   dumpRootDirEntry(CHAR* fileName);
    vector<DirEntryItem>* getFileEntry() { return &m_vecDirEnrty[NORMAL_FILE_MP3]; }

private:
    static const DWORD FAT_END_CHAIN = 0x0FFFFFFF;
    void  readBootSector();
    void  readFatsData();
    DWORD openDeviceAndLock();

    bool  goToSector(DWORD aSectorNum);
    DWORD getSectorNumFromCluster(DWORD adwClusterNum);

    bool readBytesFromDeviceSector  (BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartSector);
    bool readBytesFromDeviceCluster(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartCluster);
    bool writeBytesToDeviceCluster(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartCluster);
    bool writeBytesToDeviceSector(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartSector);

    bool readChainedClusters(DWORD aStartClusterNum, BYTE* aoChainedClustersData, DWORD* aoSizeOfData);
    bool writeChainedClusters(DWORD aStartClusterNum, BYTE* aiChainedClustersData, DWORD aSizeOfData);

    void freeDirEntryItems();
    void cleanup();

private:
    TCHAR           m_pVolume[9];
    TCHAR           m_pDriveLable[3];
    HANDLE          m_hDevice;
    DWORD           m_sectorSize;
    FATBootSector   m_bootSector;
    DWORD           m_clusterSizeBytes;
    DWORD*          m_FAT1Data;
    DWORD*          m_FAT2Data;
    vector<DirEntryItem> m_vecDirEnrty[MAX_TYPE];
    BYTE*           m_pRootDirData;
    DWORD           m_dwDirDataSize;
    BOOL            m_sortDescent[MAX_SORT_TYPE];

    wofstream       m_logFile;
    ofstream        m_dataFile;
};

// utility methods
bool isLFNEntry(FATDirEntryUn aEntryToCheck);
bool isFolderEntry(FATDirEntryUn aEntryToCheck);
bool isSpecialEntry(FATDirEntryUn aEntryToCheck);
bool isDeletedEntry(FATDirEntry aEntryToCheck);

