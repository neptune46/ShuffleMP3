
// ShuffleMP3Dlg.h : header file
//

#pragma once
#include "afxcmn.h"
#include "CFat32Partition.h"
#include "afxwin.h"

// CShuffleMP3Dlg dialog
class CShuffleMP3Dlg : public CDialogEx
{
// Construction
public:
    CShuffleMP3Dlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_SHUFFLEMP3_DIALOG };
#endif

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

private:
    void parseMP3InfoInFileList();
    void updateMP3FileList();
    void refreshCurrentViewList();

// Implementation
protected:
    HICON m_hIcon;

    // Generated message map functions
    virtual BOOL OnInitDialog();
    afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
    afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
    DECLARE_MESSAGE_MAP()
public:
    CListCtrl m_listCtrl;
    CFont m_font;
    CFat32Partition m_fat32;
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedOk();
    afx_msg void OnBnClickedButton1();
    afx_msg void OnBnClickedButton2();
    vector<wstring> m_fat32UsbLabels;
    CComboBox m_Combox;
    DWORD m_idxCurSel;
    afx_msg void OnCbnSelchangeCombo1();
    afx_msg void OnLvnItemchangedList1(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnLvnColumnclickList1(NMHDR *pNMHDR, LRESULT *pResult);
    afx_msg void OnBnClickedLoad();
    CProgressCtrl m_loadingProgCtrl;
};
