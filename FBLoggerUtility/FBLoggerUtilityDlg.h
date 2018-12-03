
// TCLoggerGUIDlg.h : header file
//
#pragma once

#include "afxeditbrowsectrl.h"
#include "LoggerDataReader.h"
#include "FBLoggerLogFile.h"
#include "ProgressBarDlg.h"

#include <atomic>

#define WORKER_THREAD_RUNNING     (WM_USER + 103)
#define WORKER_THREAD_DONE        (WM_USER + 104)

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
    CProgressBarDlg* m_pProgressBarDlg;
    HANDLE m_ThreadID;

    CWinThread* m_workerThread;
    HANDLE m_hKillEvent;

public:
    afx_msg LRESULT OnCancelProcessing(WPARAM, LPARAM);
    afx_msg LRESULT OnWorkerThreadDone(WPARAM, LPARAM);
    afx_msg LRESULT OnWorkerThreadRunning(WPARAM, LPARAM);
    afx_msg void OnBnClickedCancel();
    afx_msg void OnBnClickedConvert();
    afx_msg void OnBnClickedCreateRaw();
    afx_msg void OnEnChangeBurnNameEdit();
    afx_msg void OnEnChangeConfigFileBrowse();
    afx_msg void OnEnChangeDataDirBrowse();

    BOOL OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult);
    CButton m_btnConvert;
    CButton m_ctlCheckRaw;
  
    CMFCEditBrowseCtrl m_dataDirBrowser;
    CMFCEditBrowseCtrl m_configFileBrowser;
    CEdit m_burnNameEdit;

    UINT ProcessAllDatFiles();

private:
    void ProcessIniFile();
    void InitProgressBarDlg();
    void UpdateIniFile();

    CString m_appPath;
    CString m_appIniPath;
    CString m_dataPath;
    CString m_burnName;
    CString m_configFilePath;
    CString m_windTunnelDataFileName;

    string m_windTunnelDataTablePath;

    int m_numFilesProcessed;
    int m_numInvalidFiles;
    int m_numFilesConverted;

    bool m_createRawTicked;

    atomic<bool> m_waitForWorkerThread;
    atomic<int> m_workerThreadCount;
};
