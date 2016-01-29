
#include "stdafx.h"
#include "CFat32Partition.h"

WCHAR* g_dashLine = L"----------------------------------------------------------------";

BOOL isMP3File(wstring strName)
{
    size_t found = strName.find_last_of(_T("."));
    if (found == string::npos)
    {
        return FALSE;
    }
    else
    {
        found += 1;
        wstring strExt = strName.substr(found, strName.length() - found);
        return !(strExt.compare(_T("mp3")) && 
                 strExt.compare(_T("MP3")) && 
                 strExt.compare(_T("Mp3")));
    }
}

WCHAR* getShortName(FATDirEntry* shortEntry)
{
    char* entryName = new char[sizeof(shortEntry->DIR_Name) + 2];
    int c = 0;

    // file name : 8 bytes
    for (int i = 0; i < 8; i++)
    {
        if (shortEntry->DIR_Name[i] != 0x20)
        {
            entryName[c++] = shortEntry->DIR_Name[i];
        }
    }

    entryName[c++] = '.';

    // file extension: 3 bytes
    for (int i = 8; i < 11; i++)
    {
        if (shortEntry->DIR_Name[i] != 0x20)
        {
            entryName[c++] = shortEntry->DIR_Name[i];
        }
    }
    entryName[c++] = '\0';

    // Convert ASCII to UNICODE
    CA2W temp(entryName);
    delete[] entryName;

    int size = wcslen(temp);

    // Since the ATL macro to convert from ASCII to UNICODE is freeing the data when 
    // the buffer is out of scope - we'll copy the data into out own heap-managed buffer
    WCHAR* ret = new WCHAR[size + 1];

    // Using memcpy, cause wcscpy expect \0 in the end of the source, 
    // and the ATL macro don't put it there from some reason...
    memcpy(ret, temp, size*sizeof(WORD));
    ret[size] = '\0';
    return ret;
}
WCHAR* getLongName(LFNEntry* chainedLFNEntry, WORD numOfEntryElements)
{
    // If this is not an LFN
    if (numOfEntryElements == 0)
        return NULL;

    LFNEntry* lfnCurrEntry = chainedLFNEntry;


    // The size of the name in BYTES, for each LFN entry
    int entryNameSize = sizeof(lfnCurrEntry->LDIR_Name1) +
        sizeof(lfnCurrEntry->LDIR_Name2) +
        sizeof(lfnCurrEntry->LDIR_Name3);

    // The size of the name in BYTES, for the WHOLE dir entry
    int size = numOfEntryElements * entryNameSize;

    WCHAR* wName = new WCHAR[size / sizeof(WCHAR) + 1];

    // Adding the \0 ourselves, cause there's a chance that the file name won't have it
    // (Happens if the file name if fully populate all the bytes reserved for the name, 
    //  and there's no place left for the \0)
    wName[size / sizeof(WCHAR)] = '\0';

    // The order of the entries is reverse to the order in this entries array (numOfEntryElements-1,.., 2, 1,0)
    for (int i = numOfEntryElements - 1; i >= 0; --i)
    {
        // This is a pointer to the start position in the Name buffer for this LFN entry
        WCHAR* wCurrNamePosition = wName + (numOfEntryElements - 1 - i)*entryNameSize / sizeof(WCHAR);

        // Copies the data
        memcpy_s(wCurrNamePosition,
            sizeof(lfnCurrEntry[i].LDIR_Name1), lfnCurrEntry[i].LDIR_Name1, sizeof(lfnCurrEntry[i].LDIR_Name1));

        wCurrNamePosition += sizeof(lfnCurrEntry[i].LDIR_Name1) / sizeof(WCHAR);
        memcpy_s(wCurrNamePosition,
            sizeof(lfnCurrEntry[i].LDIR_Name2), lfnCurrEntry[i].LDIR_Name2, sizeof(lfnCurrEntry[i].LDIR_Name2));

        wCurrNamePosition += sizeof(lfnCurrEntry[i].LDIR_Name2) / sizeof(WCHAR);
        memcpy_s(wCurrNamePosition,
            sizeof(lfnCurrEntry[i].LDIR_Name3), lfnCurrEntry[i].LDIR_Name3, sizeof(lfnCurrEntry[i].LDIR_Name3));
    }

    return wName;
}

WCHAR* getName(FATDirEntry* shortEntry, LFNEntry* chainedLFNEntry, WORD numOfEntryElements)
{
    WCHAR* longName = getLongName(chainedLFNEntry, numOfEntryElements);
    if (longName != NULL)
        return longName;
    else
        return getShortName(shortEntry);
}

bool isFolderEntry(FATDirEntryUn aEntryToCheck)
{
    return ((aEntryToCheck.ShortEntry.DIR_Attr & ATTR_DIRECTORY) == ATTR_DIRECTORY);
}

bool isLFNEntry(FATDirEntryUn aEntryToCheck)
{
    return ((aEntryToCheck.LongEntry.LDIR_ATT & ATTR_LONG_NAME_MASK) == ATTR_LONG_NAME);
}

bool isSpecialEntry(FATDirEntryUn aEntryToCheck)
{
    // Checks that this is a Volume label entry
    if (((aEntryToCheck.LongEntry.LDIR_ATT & ATTR_LONG_NAME_MASK) != ATTR_LONG_NAME) && (aEntryToCheck.LongEntry.LDIR_Ord != 0xE5))
    {
        if ((aEntryToCheck.ShortEntry.DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_VOLUME_ID)
        {
            return true;
        }
        // If the first byte is the 0x2E (".") than this is probably one of the "." or ".." entries 
        // In the start of each folder
        else if (((aEntryToCheck.ShortEntry.DIR_Attr & (ATTR_DIRECTORY | ATTR_VOLUME_ID)) == ATTR_DIRECTORY) &&
            (aEntryToCheck.ShortEntry.DIR_Name[0] == (char)0x2E))
        {
            return true;
        }
    }

    return false;
}

bool isDeletedEntry(FATDirEntry aEntryToCheck)
{
    // The entry is considered "deleted" if the first byte is 0xE5
    return (aEntryToCheck.DIR_Name[0] == (char)0xE5);
}

bool lockAndDismount(HANDLE hDevice)
{
    DWORD dwReturned;
    BOOL bRes = DeviceIoControl(hDevice, FSCTL_DISMOUNT_VOLUME, 0, 0, 0, 0, &dwReturned, 0);

    if (!bRes)
    {
        printf("Error dismounting the volume (Error=0x%X)\n", GetLastError());
        return false;
    }
    else
    {
        bRes = DeviceIoControl(hDevice, FSCTL_LOCK_VOLUME, 0, 0, 0, 0, &dwReturned, 0);
        if (!bRes)
        {
            printf("Error locking the volume (Error=0x%X)\n", GetLastError());
            return false;
        }
        else
        {
            printf("Done!\n");
            return true;
        }
    }
}

bool compFileName1(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.pEntryName, b.pEntryName) >= 0) ? 0 : 1;
}

bool compFileName2(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.pEntryName, b.pEntryName) > 0) ? 1 : 0;
}

bool compTile1(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.info.tag[TAG_TITLE].c_str(), b.info.tag[TAG_TITLE].c_str()) >= 0) ? 0 : 1;
}

bool compTile2(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.info.tag[TAG_TITLE].c_str(), b.info.tag[TAG_TITLE].c_str()) > 0) ? 1 : 0;
}

bool compArtist1(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.info.tag[TAG_ARTIST].c_str(), b.info.tag[TAG_ARTIST].c_str()) >= 0) ? 0 : 1;
}

bool compArtist2(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.info.tag[TAG_ARTIST].c_str(), b.info.tag[TAG_ARTIST].c_str()) > 0) ? 1 : 0;
}

bool compAlbum1(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.info.tag[TAG_ALBUM].c_str(), b.info.tag[TAG_ALBUM].c_str()) >= 0) ? 0 : 1;
}

bool compAlbum2(const DirEntryItem &a, const DirEntryItem &b)
{
    return (wcscmp(a.info.tag[TAG_ALBUM].c_str(), b.info.tag[TAG_ALBUM].c_str()) > 0) ? 1 : 0;
}

bool compDuration1(const DirEntryItem &a, const DirEntryItem &b)
{
    int x = a.info.length_minutes * 60 + a.info.length_seconds;
    int y = b.info.length_minutes * 60 + b.info.length_seconds;
    return (x >= y) ? 0 : 1;
}

bool compDuration2(const DirEntryItem &a, const DirEntryItem &b)
{
    int x = a.info.length_minutes * 60 + a.info.length_seconds;
    int y = b.info.length_minutes * 60 + b.info.length_seconds;
    return (x > y) ? 1 : 0;
}

CFat32Partition::CFat32Partition(TCHAR* pDriveLetter)
{
    m_hDevice = 0;
    m_FAT1Data = NULL;
    m_FAT2Data = NULL;
    m_pRootDirData = NULL;

    wstring label = pDriveLetter;
    label += _T(":");
    _tcsncpy_s(m_pDriveLable, label.c_str(), 2);

    _tcsncpy_s(m_pVolume, _T("\\\\.\\"), 7);
    _tcsncat_s(m_pVolume, pDriveLetter, 1);
    _tcsncat_s(m_pVolume, _T(":"), 1);
}

CFat32Partition::~CFat32Partition()
{
    cleanup();
}

void CFat32Partition::setDrivePath(const TCHAR* pPath)
{
    _tcsncpy_s(m_pDriveLable, pPath, 2);

    _tcsncpy_s(m_pVolume, _T("\\\\.\\"), 7);
    _tcsncat_s(m_pVolume, pPath, 2);
}

DWORD CFat32Partition::openDeviceAndLock()
{
    m_hDevice = CreateFile(m_pVolume,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);

    // If we connected successfully to the drive
    if (FAILED(m_hDevice))
    {
        return FALSE;
    }

    if (!lockAndDismount(m_hDevice))
    {
        if (m_hDevice != NULL)
            CloseHandle(m_hDevice);
        return FALSE;
    }

    return TRUE;
}

DWORD CFat32Partition::loadRootDirEntry()
{
    cleanup();

    if (openDeviceAndLock() != TRUE)
    {
        return FALSE;
    }

    // Initialize mandatory data needed for the communication with the volume
    DISK_GEOMETRY_EX lDiskGeo;
    DWORD lBytes;

    if (DeviceIoControl(m_hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &lDiskGeo, sizeof(DISK_GEOMETRY_EX), &lBytes, NULL))
    {
        m_sectorSize = lDiskGeo.Geometry.BytesPerSector;
    }

    // Initializing the boot sector data member
    readBootSector();
    readFatsData();

    // Init the cluster size
    m_clusterSizeBytes = m_bootSector.PBP_SecPerClus * m_sectorSize;

    DWORD dwChainedClustersSizeBytes = 0;

    // First - Gets the size of the data
    if (!readChainedClusters(m_bootSector.BPB_RootClus, NULL, &dwChainedClustersSizeBytes))
    {
        _tprintf(_T("The root folder is probably corrupted, because no data was found on this folder\n"));
    }
    else if (dwChainedClustersSizeBytes == 0)
    {
        // The size is 0 if there's a corruption in the folder
        //TCHAR* name = getName();
        _tprintf(_T("The root folder is probably corrupted, because no data was found on this folder\n"));
        //delete[] name;
    }
    else
    {
        BYTE* bData = new BYTE[dwChainedClustersSizeBytes];
        if (!readChainedClusters(m_bootSector.BPB_RootClus, bData, &dwChainedClustersSizeBytes))
        {
            printf("Couldn't load the folder's content for root directory, Code: 0x%X\n", GetLastError());
        }
        else
        {
            // We got all the sub-folders and files inside the bData, lets populate the lists..
            DWORD dwCurrDataPos = 0;
            DirEntryItem entryItem;

            memset(&entryItem, 0, sizeof(DirEntryItem));

            // Read as long as we have more dir entries to read AND
            // We haven't passes the whole table
            while (dwChainedClustersSizeBytes - dwCurrDataPos >= sizeof(FATDirEntry) &&
                bData[dwCurrDataPos] != 0x00)
            {
                FATDirEntryUn fatCurrEntry;

                // Read the curr dir entry from bData
                memcpy(&fatCurrEntry, bData + dwCurrDataPos, sizeof(FATDirEntry));
                dwCurrDataPos += sizeof(FATDirEntry);

                DWORD dataSize = sizeof(FATDirEntry);
                entryItem.pShortOrLong = L"Short";
                entryItem.pStatus = L"Normal";
                entryItem.dwEntrySize = dataSize;
                entryItem.pEntryData = new BYTE[dataSize];
                memcpy_s(entryItem.pEntryData, dataSize, &fatCurrEntry, dataSize);
                entryItem.pEntryName = getName(&fatCurrEntry.ShortEntry, NULL, 0);

                // In case we're reading any special entry, like the volume id, or the "."\".." entries
                if (isSpecialEntry(fatCurrEntry))
                {
                    entryItem.type = SPECIAL_FOLDER;
                    entryItem.pFileOrDir = L"Dir ";
                    m_vecDirEnrty[SPECIAL_FOLDER].push_back(entryItem);
                }
                else
                {
                    LFNEntry* fatLFNEntries = NULL;
                    WORD wNumOfLFNOrds = 0;

                    // In case this is a LFN Entry - Load the LFN Entries to fatLFNEntries 
                    // If the file is deleted - we'll not treat it like LFN if it was, and just
                    // load each ord from the LFN as a short entry
                    if (isLFNEntry(fatCurrEntry) && !isDeletedEntry(fatCurrEntry.ShortEntry))
                    {
                        // The first entry should contain the last Ord entry
                        if (!(fatCurrEntry.LongEntry.LDIR_Ord & LAST_LONG_ENTRY))
                        {
                            // Error! this is not a valid first lfn entry
                        }
                        else
                        {
                            // Get the last Ord, w/o the last entry mask
                            wNumOfLFNOrds = fatCurrEntry.LongEntry.LDIR_Ord & (LAST_LONG_ENTRY ^ 0xFF);
                            fatLFNEntries = new LFNEntry[wNumOfLFNOrds + 1];
                            fatLFNEntries[0] = fatCurrEntry.LongEntry;

                            // Read this LFN's rest of the parts 
                            for (WORD wCurrOrd = 1;
                            (wCurrOrd < wNumOfLFNOrds) &&
                                (dwChainedClustersSizeBytes - dwCurrDataPos >= sizeof(LFNEntry));
                                ++wCurrOrd)
                            {
                                memcpy(&fatLFNEntries[wCurrOrd], bData + dwCurrDataPos, sizeof(LFNEntry));
                                dwCurrDataPos += sizeof(LFNEntry);
                            }
                        }

                        // The next entry, after the LFNs, must be the short file entry
                        // We are making sure that the fatCurrEntry holds the short file entry
                        memcpy(&fatCurrEntry, bData + dwCurrDataPos, sizeof(FATDirEntry));
                        dwCurrDataPos += sizeof(FATDirEntry);

                        entryItem.pShortOrLong = L"Long";
                        entryItem.dwEntrySize = wNumOfLFNOrds * sizeof(LFNEntry) + sizeof(FATDirEntry);
                        if (entryItem.pEntryData)
                        {
                            delete[] entryItem.pEntryData;
                        }
                        entryItem.pEntryData = new BYTE[entryItem.dwEntrySize];

                        // copy long part entry data
                        dataSize = wNumOfLFNOrds * sizeof(LFNEntry);
                        memcpy_s(entryItem.pEntryData, dataSize, &fatLFNEntries[0], dataSize);

                        // copy short  part entry data
                        memcpy_s(entryItem.pEntryData + dataSize, sizeof(FATDirEntry), &fatCurrEntry, sizeof(FATDirEntry));

                        if (entryItem.pEntryName)
                        {
                            delete[] entryItem.pEntryName;
                        }
                        entryItem.pEntryName = getName(NULL, fatLFNEntries, wNumOfLFNOrds);

                        delete[] fatLFNEntries;
                    }

                    if (isFolderEntry(fatCurrEntry))
                    {
                        entryItem.pFileOrDir = L"Dir ";
                        if (isDeletedEntry(fatCurrEntry.ShortEntry))
                        {
                            entryItem.type = DELETE_FOLDER;
                            entryItem.pStatus = L"Del   ";
                            m_vecDirEnrty[DELETE_FOLDER].push_back(entryItem);
                        }
                        else
                        {
                            entryItem.type = NORMAL_FOLDER;
                            m_vecDirEnrty[NORMAL_FOLDER].push_back(entryItem);
                        }
                    }
                    else
                    {
                        entryItem.pFileOrDir = L"File";
                        if (isDeletedEntry(fatCurrEntry.ShortEntry))
                        {
                            entryItem.type = DELETE_FILE;
                            entryItem.pStatus = L"Del   ";
                            m_vecDirEnrty[DELETE_FILE].push_back(entryItem);
                        }
                        else
                        {
                            if (isMP3File((entryItem).pEntryName))
                            {
                                entryItem.type = NORMAL_FILE_MP3;
                                m_vecDirEnrty[NORMAL_FILE_MP3].push_back(entryItem);
                            } 
                            else
                            {
                                entryItem.type = NORMAL_FILE_OTHER;
                                m_vecDirEnrty[NORMAL_FILE_OTHER].push_back(entryItem);
                            }
                            
                        }
                    }
                }
            } // while
        }
        delete[] bData;
    }

    // We close the handle here, because we are going to read MP3 file for parsing tag info later.
    // If the handle is not closed, MP3 file reading will fail.
    // Note: we have to re-open this handle when need to flash new root directory to device

    if (m_hDevice != NULL)
    {
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
    }

    return 0;
}

DWORD CFat32Partition::shuffleMP3Files()
{
    random_shuffle(m_vecDirEnrty[NORMAL_FILE_MP3].begin(), m_vecDirEnrty[NORMAL_FILE_MP3].end());
    return 0;
}

void CFat32Partition::sortMP3Files(SortType byType)
{
    bool(*compFun)(const DirEntryItem &, const DirEntryItem &) = NULL;

    switch (byType)
    {
    case BY_FILENAME:
        compFun = (m_sortDescent[BY_FILENAME]) ? compFileName1 : compFileName2;
        m_sortDescent[BY_FILENAME] = !m_sortDescent[BY_FILENAME];
        break;

    case BY_TITLE:
        compFun = (m_sortDescent[BY_TITLE]) ? compTile1 : compTile2;
        m_sortDescent[BY_TITLE] = !m_sortDescent[BY_TITLE];
        break;

    case BY_ARTIST:
        compFun = (m_sortDescent[BY_ARTIST]) ? compArtist1 : compArtist2;
        m_sortDescent[BY_ARTIST] = !m_sortDescent[BY_ARTIST];
        break;

    case BY_ALBUM:
        compFun = (m_sortDescent[BY_ALBUM]) ? compAlbum1 : compAlbum2;
        m_sortDescent[BY_ALBUM] = !m_sortDescent[BY_ALBUM];
        break;

    case BY_DURATION:
        compFun = (m_sortDescent[BY_DURATION]) ? compDuration1 : compDuration2;
        m_sortDescent[BY_DURATION] = !m_sortDescent[BY_DURATION];
        break;

    default:
        break;
    }

    if (compFun)
    {
        sort(m_vecDirEnrty[NORMAL_FILE_MP3].begin(), m_vecDirEnrty[NORMAL_FILE_MP3].end(), compFun);
    }
}

DWORD CFat32Partition::flushRootDirToDevice()
{
    m_dwDirDataSize = 0;

    if (openDeviceAndLock() != TRUE)
    {
        return FALSE;
    }

    for (int i = 0; i < MAX_TYPE; i++)
    {
        for (vector<DirEntryItem>::iterator it = m_vecDirEnrty[i].begin(); it != m_vecDirEnrty[i].end(); ++it)
        {
            m_dwDirDataSize += it->dwEntrySize;
        }
    }

    m_pRootDirData = new BYTE[m_dwDirDataSize];
    if (m_pRootDirData)
    {
        memset(m_pRootDirData, 0, m_dwDirDataSize);
    }
    else
    {
        return FALSE;
    }

    DWORD dwOffset = 0;
    for (int i = 0; i < MAX_TYPE; i++)
    {
        for (vector<DirEntryItem>::iterator it = m_vecDirEnrty[i].begin(); it != m_vecDirEnrty[i].end(); ++it)
        {
            memcpy_s(m_pRootDirData + dwOffset, it->dwEntrySize, it->pEntryData, it->dwEntrySize);
            dwOffset += it->dwEntrySize;
        }
    }

    bool success = writeChainedClusters(m_bootSector.BPB_RootClus, m_pRootDirData, m_dwDirDataSize);

    // Close device handle (release device) in case other programs want access this device.
    if (m_hDevice != NULL)
    {
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
    }

    if (!success)
    {
        _tprintf(_T("Error while flushing the data to the device.\n"));
        return FALSE;
    }
    else
    {
        _tprintf(_T("Flush data successfully.\n"));
        return TRUE;
    }
}

DWORD CFat32Partition::dumpRootDirEntry(CHAR* fileName)
{
    m_logFile.open(fileName);
    m_logFile.imbue(locale(std::locale(), new codecvt_utf8_utf16<wchar_t, 0x10ffff, codecvt_mode(consume_header | generate_header)>));

    m_logFile << g_dashLine << endl;
    m_logFile << "Type\t" << "Format\t" << "Status\t" << "DirSize\t\t" << "Name\t" << endl;
    m_logFile << g_dashLine << endl;

    string name = fileName; name += ".bin";
    m_dataFile.open(name.c_str(), ios::out | ios::binary);

    WCHAR wfileName[512];
    for (int i = 0; i < MAX_TYPE; i++)
    {
        for (vector<DirEntryItem>::iterator it = m_vecDirEnrty[i].begin(); it != m_vecDirEnrty[i].end(); ++it)
        {
            swprintf_s(wfileName, 256, _T("%s\t %s\t %s\t %04d\t\t %s\n"), 
                it->pFileOrDir, it->pShortOrLong, it->pStatus, it->dwEntrySize, it->pEntryName);
            m_logFile << wfileName;

            m_dataFile.write((const char*)it->pEntryData, it->dwEntrySize);
        }
    }
    m_logFile << g_dashLine << endl;

    m_logFile.close();
    m_dataFile.close();

    return 0;
}

void CFat32Partition::readBootSector()
{
    DWORD lRet = SetFilePointer(m_hDevice, 0, NULL, FILE_BEGIN);

    if (lRet == INVALID_SET_FILE_POINTER)
    {
        printf("SetPointer FAILED! 0x%X\n", GetLastError());
    }
    else
    {
        DWORD lBytesRead;

        BYTE* lTemp = new BYTE[m_sectorSize];

        // only in sector multiplications!
        if (!ReadFile(m_hDevice, lTemp, m_sectorSize, &lBytesRead, NULL))
        {
            printf("Error reading from the file 0x%X\n", GetLastError());
        }

        memcpy_s(&m_bootSector, sizeof(m_bootSector), lTemp, sizeof(m_bootSector));

        delete [] lTemp;
    }
}


void CFat32Partition::freeDirEntryItems()
{
    for (int i = 0; i < MAX_TYPE; i++)
    {
        for (vector<DirEntryItem>::iterator it = m_vecDirEnrty[i].begin(); it != m_vecDirEnrty[i].end(); ++it)
        {
            if (it->pEntryData)
            {
                delete[] it->pEntryData;
                it->pEntryData = NULL;
                it->dwEntrySize = 0;
            }

            if (it->pEntryName)
            {
                delete[] it->pEntryName;
                it->pEntryName = NULL;
            }
        }
        m_vecDirEnrty[i].clear();
    }
}

void CFat32Partition::cleanup()
{
    if (m_hDevice != NULL)
    {
        CloseHandle(m_hDevice);
        m_hDevice = NULL;
    }

    if (m_FAT1Data)
    {
        delete[] m_FAT1Data;
        m_FAT1Data = NULL;
    }

    if (m_FAT2Data)
    {
        delete[] m_FAT2Data;
        m_FAT2Data = NULL;
    }

    if (m_pRootDirData)
    {
        delete[] m_pRootDirData;
        m_pRootDirData = NULL;
    }

    freeDirEntryItems();
}

void CFat32Partition::readFatsData()
{
    _tprintf(_T("Reading FATs Data..."));

    // Calc the size in BYTES!
    long lFatTableSize = m_bootSector.BPB_FATsz32 * m_sectorSize;

    // The start sector of FAT1 is the first sector available.
    m_FAT1Data = (DWORD*)new BYTE[lFatTableSize];
    readBytesFromDeviceSector((BYTE*)m_FAT1Data, lFatTableSize, 0);

    // The FAT32 start sector is right after the FAT1's
    m_FAT2Data = (DWORD*)new BYTE[lFatTableSize];
    readBytesFromDeviceSector((BYTE*)m_FAT2Data, lFatTableSize, m_bootSector.BPB_FATsz32);

    _tprintf(_T("DONE!\n"));
}

// Sets the device's pointer to the argumented cluster number
bool CFat32Partition::goToSector(DWORD aSectorNum)
{
    // Compute the absolute position in the Device. 
    // The argumented sector is relative to the first available sector, which is after the reserved sectors
    // (Actually, it's also after the hidden sectors, but when opening the partition as we did - 
    //  there's no need to compute the hidden sectors number)
    LARGE_INTEGER liPos;
    liPos.QuadPart = (static_cast<LONGLONG>(m_bootSector.BPB_RsvdSecCnt) + aSectorNum)*m_sectorSize;

    DWORD lRet = SetFilePointer(m_hDevice, liPos.LowPart, &liPos.HighPart, FILE_BEGIN);

    if (lRet == INVALID_SET_FILE_POINTER)
    {
        printf("Failed accessing sector number %d - SetPointer FAILED! 0x%X\n", aSectorNum, GetLastError());
        return false;
    }

    // All is swell!
    return true;
}

DWORD CFat32Partition::getSectorNumFromCluster(DWORD adwClusterNum)
{
    //FDT start sector = 
    //sector number in each FAT * FAT number + 
    //(FDT start cluster - 2) * num sectors per cluster
    DWORD dwSectorNum = m_bootSector.BPB_NumFATs * m_bootSector.BPB_FATsz32 +
        (adwClusterNum - 2) * m_bootSector.PBP_SecPerClus;

    // The num of reserved sectors also should be added to get the exact position
    // but we already add it in the read method

    return dwSectorNum;
}

bool CFat32Partition::readBytesFromDeviceSector(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartSector)
{
    if (!goToSector(aStartSector))
    {
        // If unsuccessful.. :(
        return false;
    }
    else
    {
        DWORD lBytesRead;

        // only in sector multiplications!
        if (!ReadFile(m_hDevice, aBuffer, aSizeOfData, &lBytesRead, NULL))
        {
            printf("Error reading from the device, Code: 0x%X\n", GetLastError());
            return false;
        }
        return true;
    }
}


// Reads The entire Cluster chain data, starting from the argumented cluster num
// The reading will use the FAT tables, to find each time the next cluster
bool CFat32Partition::readChainedClusters(DWORD aStartClusterNum, BYTE* aoChainedClustersData, DWORD* aoSizeOfData)
{
    // Gets only the size of the buffer needed
    if (aoChainedClustersData == NULL)
    {
        DWORD dwChainTotalNumClusters = 0;
        DWORD dwNextClusterNum = aStartClusterNum;

        // As long we didn't reached the end of the chain
        // Or reached to an empty spot, which means that this place had a folder there and it got delete,
        // but it's record in the root table restored w/o restoring the entry in the FAT table
        while (dwNextClusterNum != FAT_END_CHAIN && dwNextClusterNum != 0)
        {
            ++dwChainTotalNumClusters;
            dwNextClusterNum = m_FAT1Data[dwNextClusterNum];
        }

        // The end of the chain is 0 if there's a corruption in the FAT tables 
        // (like deleted folder that came to life in the FDT only)
        if (dwNextClusterNum == 0)
        {
            *aoSizeOfData = 0;
        }
        else
        {
            // Calc the size in bytes for all the sectors found
            *aoSizeOfData = dwChainTotalNumClusters * m_clusterSizeBytes;
        }
    }
    else
    {
        DWORD dwNextClusterNum = aStartClusterNum;
        DWORD dwNumClustersRead = 0;

        // As long we didn't reached the end of the chain
        while (dwNextClusterNum != FAT_END_CHAIN)
        {
            // Reads from the device the whole cluster, and put in the next free position in the buffer 
            if (!readBytesFromDeviceCluster(aoChainedClustersData + (dwNumClustersRead*m_clusterSizeBytes),
                m_clusterSizeBytes,
                dwNextClusterNum))
            {
                return false;
            }
            dwNextClusterNum = m_FAT1Data[dwNextClusterNum];
            ++dwNumClustersRead;
        }
    }
    return true;
}

bool CFat32Partition::readBytesFromDeviceCluster(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartCluster)
{
    // We need to compute the position in the file, so first - let's convert the cluster num -> sector num
    DWORD startSectorNum = getSectorNumFromCluster(aStartCluster);

    return readBytesFromDeviceSector(aBuffer, aSizeOfData, startSectorNum);
}

bool CFat32Partition::writeChainedClusters(DWORD aStartClusterNum, BYTE* aiChainedClustersData, DWORD aSizeOfData)
{
    DWORD dwNextClusterNum = aStartClusterNum;
    DWORD dwNumClustersPassed = 0;

    // As long we didn't reached the end of the chain
    while ((aSizeOfData - (dwNumClustersPassed*m_clusterSizeBytes)) >= m_clusterSizeBytes)
    {
        if (dwNextClusterNum == FAT_END_CHAIN)
        {
            _tprintf(_T("Error getting the next cluster while writing data. Error code: 0x%X\n"), GetLastError());
            return false;
        }

        // Reads from the device the whole cluster, and put in the next free position in the buffer 
        if (!writeBytesToDeviceCluster(aiChainedClustersData + (dwNumClustersPassed*m_clusterSizeBytes),
            m_clusterSizeBytes,
            dwNextClusterNum))
        {
            return false;
        }
        dwNextClusterNum = m_FAT1Data[dwNextClusterNum];
        ++dwNumClustersPassed;
    }

    // If we have left more data, and the size of it is less then a cluster
    if ((aSizeOfData - (dwNumClustersPassed*m_clusterSizeBytes)) < m_clusterSizeBytes)
    {
        // Create a new cluster data buffer, and fill it with 0's
        BYTE* clusterComplete = new BYTE[m_clusterSizeBytes];
        memset(clusterComplete, 0, m_clusterSizeBytes);

        // Copy the cluster data that left to out new buffer
        memcpy(clusterComplete,
            aiChainedClustersData + (dwNumClustersPassed*m_clusterSizeBytes),
            aSizeOfData - (dwNumClustersPassed*m_clusterSizeBytes));

        // Write our data. The cluster contains 0's after the rest of the data we have left
        bool success = (writeBytesToDeviceCluster(clusterComplete,
            m_clusterSizeBytes,
            dwNextClusterNum));
        delete[] clusterComplete;
        if (!success)
        {
            return false;
        }
    }

    return true;
}

bool CFat32Partition::writeBytesToDeviceCluster(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartCluster)
{
    // We need to compute the position in the file, so first - let's convert the cluster num -> sector num
    DWORD startSectorNum = getSectorNumFromCluster(aStartCluster);

    return writeBytesToDeviceSector(aBuffer, aSizeOfData, startSectorNum);
}

bool CFat32Partition::writeBytesToDeviceSector(BYTE* aBuffer, DWORD aSizeOfData, DWORD aStartSector)
{
    // First - go to the appropriate position in the device
    if (!goToSector(aStartSector))
    {
        // If unsuccessful.. :(
        return false;
    }
    else
    {
        DWORD lBytesRead;

        // only in sector multiplications!
        if (!WriteFile(m_hDevice, aBuffer, aSizeOfData, &lBytesRead, NULL))
        {
            printf("Error writing to the device, Code: 0x%X\n", GetLastError());
            return false;
        }
        return true;
    }
}


