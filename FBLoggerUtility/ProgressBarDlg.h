
// TickCounterDlg.h : header file
//

#pragma once
#include <afxwin.h>
#include <afxcmn.h>
#include "Resource.h"


class CReconstructionView;
#define UPDATE_PROGRESSS_BAR  (WM_USER + 100)
#define CLOSE_PROGRESSS_BAR   (WM_USER + 101)
#define CANCEL_PROCESSING     (WM_USER + 102)

// CTickCounterDlg dialog
class CProgressBarDlg : public CDialogEx
{
// Construction
public:
	CProgressBarDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_PROGRESS_BAR_DLG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;
    CWnd* m_pParent;
    HWND  m_hParent;

public:
	
	CStatic m_ApplicationTime;
	CStatic m_Progress;
	CProgressCtrl m_ProgressBar;
	unsigned int ComputerTime;
	DWORD m_nTimer;
	// Generated message map functions
	virtual BOOL OnInitDialog();
    DECLARE_MESSAGE_MAP()
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
    afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg LRESULT OnUpdateProgressBar(WPARAM wparam, LPARAM lparam);
	afx_msg LRESULT OnCloseProgressBar(WPARAM, LPARAM);
	afx_msg void OnCancel();
	afx_msg void OnOK();
	afx_msg void PostNcDestroy();
	afx_msg void OnClose();
    afx_msg void OnBnClickedCancel();
};
