
// TickCounterDlg.cpp : implementation file
//

#include "stdafx.h"
#include "afxdialogex.h"
#include <windows.h>
#include "ProgressBarDlg.h"
#include <processthreadsapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CProgressBarDlg::CProgressBarDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(CProgressBarDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CProgressBarDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);

	DDX_Control(pDX, IDC_PROGRESS1, m_ProgressBar);
	DDX_Control(pDX, IDC_STATIC_ELAPSED_TIME, m_ApplicationTime);
	DDX_Control(pDX, IDC_STATIC_PROGRESS, m_Progress);
}

BEGIN_MESSAGE_MAP(CProgressBarDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_TIMER()
	ON_WM_CLOSE()
	ON_MESSAGE(UPDATE_PROGRESSS_BAR, OnUpdateProgressBar)
	ON_MESSAGE(CLOSE_PROGRESSS_BAR, OnCloseProgressBar)	
END_MESSAGE_MAP()


// CTickCounterDlg message handlers

BOOL CProgressBarDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	
	// TODO: Add extra initialization here
	// TODO: Add extra initialization here
	ComputerTime = GetTickCount();
	m_ProgressBar.SetRange(0, 100);
	m_ProgressBar.SetPos(0);
	//m_nTimer = SetTimer(1234, 1000, NULL);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CProgressBarDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
		CDialogEx::OnSysCommand(nID, lParam);
	
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CProgressBarDlg::OnPaint()
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
HCURSOR CProgressBarDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CProgressBarDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: Add your message handler code here and/or call default
	unsigned long CurTickValue = GetTickCount();
	unsigned int Difference = CurTickValue - ComputerTime;

	unsigned int ComputerHours, ComputerMinutes, ComputerSeconds;
	unsigned int ApplicationHours, ApplicationMinutes, ApplicationSeconds;

	ComputerHours = (CurTickValue / (3600 * 999)) % 24;
	ComputerMinutes = (CurTickValue / (60 * 999)) % 60;
	ComputerSeconds = (CurTickValue / 999) % 60;
	ApplicationHours = (Difference / (3600 * 999)) % 24;
	ApplicationMinutes = (Difference / (60 * 999)) % 60;
	ApplicationSeconds = (Difference / 999) % 60;
	CString val2;	
	val2.Format(_T("%d:%d:%d"),
	ApplicationHours, ApplicationMinutes, ApplicationSeconds);	
	m_ApplicationTime.SetWindowTextW(val2);
	UpdateData(FALSE);
	CDialogEx::OnTimer(nIDEvent);
}

LRESULT CProgressBarDlg::OnUpdateProgressBar(WPARAM wparam, LPARAM lparam)
{
	m_ProgressBar.SetPos(wparam);
	CString val2;
	val2.Format(_T("%d "),wparam);
	val2 += _T("% ");
	m_Progress.SetWindowTextW(val2);
	return 0;
}

void CProgressBarDlg::OnCancel()
{
	
	
}

void CProgressBarDlg::OnOK()
{	
	
}

void CProgressBarDlg::PostNcDestroy()
{	
	delete this;
}
void CProgressBarDlg::OnClose()
{
	
}


LRESULT CProgressBarDlg::OnCloseProgressBar(WPARAM, LPARAM)
{
	if (this)
	 this->DestroyWindow();  // destroy it

	return 0;
}
