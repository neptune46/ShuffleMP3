
// ShuffleMP3Dlg.cpp : implementation file
//

#include "stdafx.h"
#include "ShuffleMP3.h"
#include "ShuffleMP3Dlg.h"
#include "afxdialogex.h"
#include "MP3Info.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

void getFat32UsbLetters(vector<wstring> &m_fat32UsbLabels)
{
    const wstring labelList[32] =
    {
        _T("A:"), _T("B:"), _T("C:"), _T("D:"), _T("E:"), _T("F:"), _T("G:"), _T("H:"),
        _T("I:"), _T("J:"), _T("K:"), _T("L:"), _T("M:"), _T("N:"), _T("O:"), _T("P:"),
        _T("Q:"), _T("R:"), _T("S:"), _T("T:"), _T("U:"), _T("V:"), _T("W:"), _T("X:"),
        _T("Y:"), _T("Z:"), _T(" :"), _T(" :"), _T(" :"), _T(" :"), _T(" :"), _T(" :")
    };

    DWORD dwDriveMask = 0;
    DWORD dwDriverType = 0;

    dwDriveMask = GetLogicalDrives();

    for (int i = 0; i < 26; i++)
    {
        if (dwDriveMask & (1 << i))
        {
            if (GetDriveType(labelList[i].c_str()) == DRIVE_REMOVABLE)
            {
                HANDLE hDrive;
                wstring fullName = _T("\\\\.\\");
                fullName += labelList[i];

                hDrive = CreateFile(fullName.c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    0);

                if (hDrive == INVALID_HANDLE_VALUE)
                {
                    continue;
                }

                BOOL bSuccess;
                DWORD BytesReturned;
                PARTITION_INFORMATION_EX partinfo;

                bSuccess = DeviceIoControl((HANDLE)hDrive,  // handle to a partition
                    IOCTL_DISK_GET_PARTITION_INFO_EX,       // dwIoControlCode
                    (LPVOID)NULL,                           // lpInBuffer
                    (DWORD)0,                               // nInBufferSize
                    (LPVOID)&partinfo,                      // output buffer
                    sizeof(PARTITION_INFORMATION_EX),       // size of output buffer
                    (LPDWORD)&BytesReturned,                // number of bytes returned
                    NULL);                                  // OVERLAPPED structure

                if (!bSuccess)
                {
                    CloseHandle(hDrive);
                    continue;
                }

                if (((partinfo).Mbr).PartitionType == PARTITION_FAT32 ||
                    ((partinfo).Mbr).PartitionType == PARTITION_FAT32_XINT13)
                {
                    m_fat32UsbLabels.push_back(labelList[i]);
                }

                CloseHandle(hDrive);
            }
        }
    }
}

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CShuffleMP3Dlg dialog



CShuffleMP3Dlg::CShuffleMP3Dlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(IDD_SHUFFLEMP3_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CShuffleMP3Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_LIST1, m_listCtrl);
    DDX_Control(pDX, IDC_COMBO1, m_Combox);
    DDX_Control(pDX, IDC_PROGRESS2, m_loadingProgCtrl);
}

BEGIN_MESSAGE_MAP(CShuffleMP3Dlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDCANCEL, &CShuffleMP3Dlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDOK, &CShuffleMP3Dlg::OnBnClickedOk)
    ON_BN_CLICKED(IDC_BUTTON1, &CShuffleMP3Dlg::OnBnClickedButton1)
    ON_BN_CLICKED(IDC_BUTTON2, &CShuffleMP3Dlg::OnBnClickedButton2)
    ON_CBN_SELCHANGE(IDC_COMBO1, &CShuffleMP3Dlg::OnCbnSelchangeCombo1)
    ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST1, &CShuffleMP3Dlg::OnLvnItemchangedList1)
    ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, &CShuffleMP3Dlg::OnLvnColumnclickList1)
    ON_BN_CLICKED(IDC_BUTTON4, &CShuffleMP3Dlg::OnBnClickedLoad)
END_MESSAGE_MAP()


// CShuffleMP3Dlg message handlers

void CShuffleMP3Dlg::parseMP3InfoInFileList()
{
    vector<DirEntryItem>* fileList;
    wstring driveLable, filePath, strTag;

    fileList = m_fat32.getFileEntry();
    driveLable = m_fat32.getDriveLable();

    if (fileList == NULL)
    {
        return;
    }

    m_loadingProgCtrl.SetRange(0, (short)fileList->size());

    int nItem = 0, index = 0;
    for (vector<DirEntryItem>::iterator it = fileList->begin(); it != fileList->end(); ++it)
    {
        filePath = driveLable + _T("\\") + it->pEntryName;

        getMp3Info(filePath.c_str(), it->info);

        m_loadingProgCtrl.SetPos(++index);
    }
}

void CShuffleMP3Dlg::refreshCurrentViewList()
{
    vector<DirEntryItem>* fileList;
    wstring driveLable, filePath, strTag;

    fileList = m_fat32.getFileEntry();
    driveLable = m_fat32.getDriveLable();

    if (fileList == NULL)
    {
        return;
    }

    int nItem = 0, index = 0;
    WCHAR str[256];
    for (vector<DirEntryItem>::iterator it = fileList->begin(); it != fileList->end(); ++it)
    {
        filePath = driveLable + _T("\\") + it->pEntryName;

        //getMp3Info(filePath.c_str(), info);

        nItem = m_listCtrl.InsertItem(index++, it->pEntryName);

        // Title
        strTag = (it->info.tag[TAG_TITLE].length()) ? it->info.tag[TAG_TITLE].c_str() : L"Unknown";
        m_listCtrl.SetItemText(nItem, 1, strTag.c_str());

        // Artist
        strTag = (it->info.tag[TAG_ARTIST].length()) ? it->info.tag[TAG_ARTIST].c_str() : L"Unknown";
        m_listCtrl.SetItemText(nItem, 2, strTag.c_str());

        // Album
        strTag = (it->info.tag[TAG_ALBUM].length()) ? it->info.tag[TAG_ALBUM].c_str() : L"Unknown";
        m_listCtrl.SetItemText(nItem, 3, strTag.c_str());

        // Duration (min:sec)
        wsprintf(str, L"%02d:%02d", it->info.length_minutes, it->info.length_seconds);
        m_listCtrl.SetItemText(nItem, 4, str);

        // Bit-rate (kbps)
        wsprintf(str, L"%d kbps", it->info.bitrate);
        m_listCtrl.SetItemText(nItem, 5, str);

        // Year
        wsprintf(str, L"%d", it->info.year);
        m_listCtrl.SetItemText(nItem, 6, str);

        // Genre
        strTag = (it->info.tag[TAG_GENRE].length()) ? it->info.tag[TAG_GENRE].c_str() : L"Unknown";
        m_listCtrl.SetItemText(nItem, 7, strTag.c_str());

    }
}

void CShuffleMP3Dlg::updateMP3FileList()
{
    m_fat32.loadRootDirEntry();

    parseMP3InfoInFileList();

    refreshCurrentViewList();
}

BOOL CShuffleMP3Dlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Add "About..." menu item to system menu.

    // IDM_ABOUTBOX must be in the system command range.
    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != NULL)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);			// Set big icon
    SetIcon(m_hIcon, FALSE);		// Set small icon

    m_font.CreatePointFont(90, _T("Microsoft YaHei"));
    m_listCtrl.SetFont(&m_font);

    // TODO: Add extra initialization here
    getFat32UsbLetters(m_fat32UsbLabels);

    vector<wstring>::iterator it;
    for (it = m_fat32UsbLabels.begin(); it != m_fat32UsbLabels.end(); ++it)
    {
        m_Combox.AddString(it->c_str());
    }

    if (m_Combox.GetCount())
    {
        it = m_fat32UsbLabels.begin();
        m_fat32.setDrivePath(it->c_str());
        m_Combox.SetCurSel(0);
        m_idxCurSel = m_Combox.GetCurSel();
    }

    // TODO: Add extra initialization here
    m_listCtrl.InsertColumn(0, _T("FileName"), LVCFMT_LEFT, 200);
    m_listCtrl.InsertColumn(1, _T("Title"), LVCFMT_LEFT, 200);
    m_listCtrl.InsertColumn(2, _T("Artist"), LVCFMT_LEFT, 100);
    m_listCtrl.InsertColumn(3, _T("Album"), LVCFMT_LEFT, 100);
    m_listCtrl.InsertColumn(4, _T("Duration"), LVCFMT_RIGHT, 100);
    m_listCtrl.InsertColumn(5, _T("Bitrate"), LVCFMT_RIGHT, 100);
    m_listCtrl.InsertColumn(6, _T("Year"), LVCFMT_RIGHT, 100);
    m_listCtrl.InsertColumn(7, _T("Genre"), LVCFMT_RIGHT, 100);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CShuffleMP3Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CShuffleMP3Dlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this); // device context for painting

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        // Center icon in client rectangle
        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        // Draw the icon
        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CShuffleMP3Dlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}



void CShuffleMP3Dlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here
    OnCancel();
}


void CShuffleMP3Dlg::OnBnClickedOk()
{
    // TODO: Add your control notification handler code here
    OnOK();
}

void CShuffleMP3Dlg::OnBnClickedButton1()
{
    // TODO: Add your control notification handler code here

#if _DEBUG
    m_fat32.dumpRootDirEntry("old.txt");
#endif

    m_fat32.shuffleMP3Files();

#if _DEBUG
    m_fat32.dumpRootDirEntry("new.txt");
#endif

    m_listCtrl.DeleteAllItems();

    refreshCurrentViewList();
}


void CShuffleMP3Dlg::OnBnClickedButton2()
{
    // TODO: Add your control notification handler code here
    if (m_fat32.flushRootDirToDevice())
    {
        MessageBoxEx(NULL, TEXT("Write to USB Partition Successfully."), TEXT("Result"), 0, 0);

#if _DEBUG
        m_fat32.dumpRootDirEntry("save.txt");
#endif

    }
    else
    {
        MessageBoxEx(NULL, TEXT("Failed to Write to USB Drive!"), TEXT("Result"), 0, 0);
    }
}


void CShuffleMP3Dlg::OnCbnSelchangeCombo1()
{
    // TODO: Add your control notification handler code here
    DWORD newSel = m_Combox.GetCurSel();
    if (newSel != m_idxCurSel)
    {
        wstring fullPath;
        fullPath = m_fat32UsbLabels[newSel].c_str();
        m_fat32.setDrivePath(fullPath.c_str());

        m_listCtrl.DeleteAllItems();

        updateMP3FileList();

        m_idxCurSel = newSel;
    }
}


void CShuffleMP3Dlg::OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: Add your control notification handler code here
    *pResult = 0;
}


void CShuffleMP3Dlg::OnLvnColumnclickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
    LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
    // TODO: Add your control notification handler code here

    m_fat32.sortMP3Files((SortType)pNMLV->iSubItem);

    m_listCtrl.DeleteAllItems();

    refreshCurrentViewList();

    *pResult = 0;
}


void CShuffleMP3Dlg::OnBnClickedLoad()
{
    // TODO: Add your control notification handler code here
    updateMP3FileList();
}
