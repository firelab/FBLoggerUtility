
// TCLoggerGUIDlg.h : header file
//
#pragma once

#include "afxeditbrowsectrl.h"
#include "FBLoggerDataReader.h"
#include "ProgressBarDlg.h"

UINT DatFileProcessRoutine(LPVOID lpParameter);

// CTCLoggerGUIDlg dialog
class CFBLoggerUtilityDlg : public CDialogEx
{
// Construction
public:
    CFBLoggerUtilityDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_FBLOGGERUTILITY_DIALOG};

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
    BOOL CFBLoggerUtilityDlg::PreTranslateMessage(MSG* pMsg);
	DECLARE_MESSAGE_MAP()

    CToolTipCtrl m_pToolTipCtrl;
    CProgressBarDlg* pProgressBarDlg;
    HANDLE m_ThreadID;

    CWinThread* m_hThread;
    HANDLE m_hKillEvent;

public:
    afx_msg void OnEnChangeDataDirBrowse();
    afx_msg void OnEnChangeConfigFileBrowse();
    afx_msg void OnBnClickedConvert();
    afx_msg void OnBnClickedCancel();

    BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
    CMFCEditBrowseCtrl m_dataDirBrowser;
    CMFCEditBrowseCtrl m_configFileBrowser;

    UINT ProcessAllDatFiles();

private:
    void ResetDataDirIniFile();
    void ResetConfigIniFile();
    void ReadSingleIniFile(CString iniFilePath);
    void ProcessIniFiles();
    void InitProgressBarDlg();

    CString m_appPath;
    CString m_dataDirIniPath;
    CString m_configFileIniPath;
    CString m_dataPath;
    CString m_configFilePath;

    volatile bool m_waitForThread;
    volatile bool m_safeToExit;

    int m_numFilesProcessed;
    int m_numInvalidFiles;
    int m_numFilesConverted;
};
