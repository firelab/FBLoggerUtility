
// TCLoggerGUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FBLoggerUtility.h"
#include "FBLoggerUtilityDlg.h"
#include "afxdialogex.h"
#include "StringUtility.h"
#include "DirectoryReaderUtility.h"

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{

}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// CTCLoggerGUIDlg dialog

CFBLoggerUtilityDlg::CFBLoggerUtilityDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CFBLoggerUtilityDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFBLoggerUtilityDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DATADIRBROWSE, m_dataDirBrowser);
    DDX_Control(pDX, IDC_CONFIGFILEBROWSE, m_configFileBrowser);
}

BEGIN_MESSAGE_MAP(CFBLoggerUtilityDlg, CDialogEx)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
    ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
    ON_EN_CHANGE(IDC_DATADIRBROWSE, &CFBLoggerUtilityDlg::OnEnChangeDataDirBrowse)
    ON_EN_CHANGE(IDC_CONFIGFILEBROWSE, &CFBLoggerUtilityDlg::OnEnChangeConfigFileBrowse)
    ON_BN_CLICKED(IDCONVERT, &CFBLoggerUtilityDlg::OnBnClickedConvert)
END_MESSAGE_MAP()

// CTCLoggerGUIDlg message handlers
BOOL CFBLoggerUtilityDlg::OnToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    // update the tool tip text
    m_pToolTipCtrl.UpdateTipText(m_dataPath, &m_dataDirBrowser);
    m_pToolTipCtrl.Update();

    m_pToolTipCtrl.UpdateTipText(m_configFilePath, &m_configFileBrowser);
    m_pToolTipCtrl.Update();

    // OnNeedText should only be called for TTN_NEEDTEXT notifications!
    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);     // need to handle both ANSI and UNICODE versions of the message
    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;

    CString strTipText;
    UINT_PTR nID = pNMHDR->idFrom;

    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool
        nID = ((UINT)(WORD)::GetDlgCtrlID((HWND)nID));
    }

    if (nID != 0) // will be zero on a separator
    {
        strTipText.LoadString((UINT)nID);
    }

    // Handle conditionally for both UNICODE and non-UNICODE apps
#ifndef _UNICODE
    if (pNMHDR->code == TTN_NEEDTEXTA)
        lstrcpyn(pTTTA->szText, strTipText, _countof(pTTTA->szText));
    else
        _mbstowcsz(pTTTW->szText, strTipText, _countof(pTTTW->szText));
#else
    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        _wcstombsz(pTTTA->szText, strTipText, _countof(pTTTA->szText));
    }
    else
    {
        lstrcpyn(pTTTW->szText, strTipText, _countof(pTTTW->szText));
    }
#endif

    *pResult = 0;

    // Bring tooltip to front
    ::SetWindowPos(pNMHDR->hwndFrom,
        HWND_TOP,
        0, 0, 0, 0,
        SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOMOVE);

    return TRUE;    // message was handled
}


void CFBLoggerUtilityDlg::ResetDataDirIniFile()
{
    std::ofstream outfile(m_dataDirIniPath);

    outfile << "tc_data_path=" << "\n";
    m_dataDirBrowser.SetWindowTextW(NULL);
    outfile.close();
}

void CFBLoggerUtilityDlg::ProcessIniFiles()
{
    m_dataDirIniPath = m_appPath + _T("\\data_dir.ini");
    m_configFileIniPath = m_appPath + _T("\\config_file.ini");

    if (!PathFileExists(m_dataDirIniPath))
    {
        ResetDataDirIniFile();
    }
    else
    {
        ReadSingleIniFile(m_dataDirIniPath);
        m_TCLoggerDataReader.SetDataPath(NarrowCStringToStdString(m_dataPath));
    }

    if (!PathFileExists(m_configFileIniPath))
    {
        m_TCLoggerDataReader.SetDataPath(NarrowCStringToStdString(m_configFileIniPath));
        ResetConfigIniFile();
    }
    else
    {
        ReadSingleIniFile(m_configFileIniPath);
        m_TCLoggerDataReader.SetConfigFile(NarrowCStringToStdString(m_configFileIniPath));
    }
}

void CFBLoggerUtilityDlg::UpdateDataDirIniFile()
{

}

void CFBLoggerUtilityDlg::ResetConfigIniFile()
{
    std::ofstream outfile(m_configFileIniPath);

    outfile << "tc_config_file=" << "\n";
    m_configFileBrowser.SetWindowTextW(NULL);
    outfile.close();
}

void CFBLoggerUtilityDlg::UpdateConfigIniFile()
{

}

void CFBLoggerUtilityDlg::ReadSingleIniFile(CString iniFilePath)
{
    bool isDataPathValid;
    bool isConfigurationTypeValid;

    ifstream infile(iniFilePath);
    string line;
    string dataPath;
    std::stringstream lineStream;
    string token = "";
    string configFilePath;

    lineStream.str("");
    lineStream.clear();
    token = "";
    dataPath = "";
    configFilePath = "";

    isDataPathValid = false;
    isConfigurationTypeValid = false;

    while (getline(infile, line))
    {
        // Reset variables for new loop iteration
        lineStream.str("");
        lineStream.clear();
        token = "";

        // Parse tokens from a single line in config file
        lineStream << line;

        while (getline(lineStream, token, '='))
        {
            // remove any whitespace or control chars from token
            token.erase(std::remove(token.begin(), token.end(), ' '), token.end());
            token.erase(std::remove(token.begin(), token.end(), '\n'), token.end());
            token.erase(std::remove(token.begin(), token.end(), '\r'), token.end());
            token.erase(std::remove(token.begin(), token.end(), '\r\n'), token.end());
            token.erase(std::remove(token.begin(), token.end(), '\t'), token.end());
            if (token == "tc_data_path")
            {
                getline(lineStream, token, '=');
                m_dataPath = token.c_str();
                m_dataPath.Replace(_T("\""), _T(""));
                if (PathFileExists(m_dataPath))
                {
                    m_dataDirBrowser.SetWindowTextW(m_dataPath);
                }
                else
                {
                    if (m_dataPath != "")
                    {
                        CString text = _T("Error: No directory exists at path \n\"") + m_dataPath + _T("\"\n");
                        CString caption("Error: Previous Data Directory Entry Invalid");
                        MessageBox(text, caption, MB_OK);
                    }
                    ResetDataDirIniFile();
                    m_dataDirBrowser.SetWindowTextW(NULL);
                }
            }
            else if (token == "tc_config_file")
            {
                getline(lineStream, token, '=');
                m_configFilePath = token.c_str();
                m_configFilePath.Replace(_T("\""), _T(""));
                if (PathFileExists(m_configFilePath))
                {
                    m_configFileBrowser.SetWindowTextW(m_configFilePath);
                }
                else
                {
                    if (m_configFilePath != "")
                    {
                        CString text = _T("Error: No config file exists at path \n\"") + m_dataPath + _T("\"\n");
                        CString caption("Error: Previous Config File Entry Invalid");
                        MessageBox(text, caption, MB_OK);
                    }
                    ResetConfigIniFile();
                    m_configFileBrowser.SetWindowTextW(NULL);
                }
            }
        }
    }
    infile.close();
}

BOOL CFBLoggerUtilityDlg::OnInitDialog()
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

	// TODO: Add extra initialization here
    wchar_t buffer[MAX_PATH];
    GetModuleFileName(NULL, buffer, MAX_PATH);
    m_appPath = buffer;
    m_appPath = m_appPath.Left(m_appPath.ReverseFind(_T('\\')));

    // Convert a TCHAR string to a LPCSTR
    CT2CA pszConvertedAnsiString(m_appPath);
    std::string strAppPath(pszConvertedAnsiString);
    m_TCLoggerDataReader.SetAppPath(strAppPath);
    ProcessIniFiles();

    if (!m_pToolTipCtrl.Create(this, TTS_ALWAYSTIP))
    {
        TRACE(_T("Unable To create ToolTip\n"));
        return FALSE;
    }

    m_pToolTipCtrl.AddTool(&m_dataDirBrowser, m_dataPath);
    m_pToolTipCtrl.AddTool(&m_configFileBrowser, m_configFilePath);
    m_pToolTipCtrl.SetDelayTime(TTDT_AUTOPOP, 300000); // stay up for 5 minutes
    m_pToolTipCtrl.Activate(TRUE);

    EnableToolTips(TRUE);

	return TRUE;  // return TRUE  unless you set the focus to a control
}

BOOL CFBLoggerUtilityDlg::PreTranslateMessage(MSG* pMsg)
{
    if (pMsg->message == WM_LBUTTONDOWN
        || pMsg->message == WM_LBUTTONUP ||
        pMsg->message == WM_MOUSEMOVE)
    {
        m_pToolTipCtrl.RelayEvent(pMsg);
    }
    return CDialog::PreTranslateMessage(pMsg);
}

void CFBLoggerUtilityDlg::OnSysCommand(UINT nID, LPARAM lParam)
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

void CFBLoggerUtilityDlg::OnPaint()
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
HCURSOR CFBLoggerUtilityDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CFBLoggerUtilityDlg::OnEnChangeDataDirBrowse()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // update data path
    m_dataDirBrowser.GetWindowTextW(m_dataPath); 
}

void CFBLoggerUtilityDlg::OnEnChangeConfigFileBrowse()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // update config file path
    m_configFileBrowser.GetWindowTextW(m_configFilePath);
    CT2CA pszConvertedAnsiString(m_configFilePath);
}

void CFBLoggerUtilityDlg::OnBnClickedConvert()
{
    if (PathFileExists(m_dataPath))
    {
        m_TCLoggerDataReader.SetDataPath(NarrowCStringToStdString(m_dataPath));
        std::ofstream outfile(m_dataDirIniPath);

        // convert a TCHAR string to a LPCSTR
        CT2CA pszConvertedAnsiString(_T("tc_data_path=") + m_dataPath);
        // construct a std::string using the LPCSTR input
        std::string strStd(pszConvertedAnsiString);

        outfile << strStd << "\n";
        outfile.close();
        if (PathFileExists(m_configFilePath))
        {
            m_TCLoggerDataReader.SetConfigFile(NarrowCStringToStdString(m_configFilePath));
            std::ofstream outfile(m_configFileIniPath);

            // convert a TCHAR string to a LPCSTR
            CT2CA pszConvertedAnsiString(_T("tc_config_file=") + m_configFilePath);
            // construct a std::string using the LPCSTR input
            std::string strStd(pszConvertedAnsiString);

            outfile << strStd << "\n";
            outfile.close();

            //m_TCLoggerDataReader.ProcessAllDataFiles();

            m_TCLoggerDataReader.PrepareToReadDataFiles(); // Check config file and input files, create output files 
            if (m_TCLoggerDataReader.IsLogFileGood())
            {
                while (!m_TCLoggerDataReader.IsDoneReadingDataFiles())
                {
                    m_TCLoggerDataReader.ProcessSingleDataFile();
                }
            }
            m_TCLoggerDataReader.EndReadingDataFiles(); // Perform Cleanup

            CString numFilesConvertedCString;
            CString numInvalidFilesCString;
            int numFilesProcessed = m_TCLoggerDataReader.GetNumFilesProcessed();
            int numInvalidFiles = m_TCLoggerDataReader.GetNumInvalidFiles();
            int numFilesConverted = numFilesProcessed - numInvalidFiles;
            numFilesConvertedCString.Format(_T("%d"), numFilesConverted);
            numInvalidFilesCString.Format(_T("%d"), numInvalidFiles);

            CString text = _T("");
            CString caption = _T("");

            if (!m_TCLoggerDataReader.IsConfigFileValid())
            {
                text = _T("There are errors in config file at\n");
                text += m_dataPath + _T("\\config.csv\n\n");
                caption = ("Error: Invalid config file");
            }
            else if (numFilesProcessed == 0)
            {
                text = _T("No data (.DAT) files were processed in directory\n\n");
                text += m_dataPath + _T("\n");
                caption = ("Error: No valid data files found");
            }
            else
            {
                string tempText;
                tempText = "Converted a total of ";
                tempText += NarrowCStringToStdString(numFilesConvertedCString);
                tempText += " .DAT files to .csv files in\n" + NarrowCStringToStdString(m_dataPath) + "\n\n";
                text += tempText.c_str();

                caption = ("Done converting files");
            }

            if (numInvalidFiles > 0)
            {
                if (m_TCLoggerDataReader.GetNumInvalidFiles() == 1)
                {
                    text += numInvalidFilesCString + _T(" invalid file was unable to be converted\n");
                }
                else
                {
                    text += numInvalidFilesCString + _T(" invalid files were unable to be converted\n");
                }
            }

            if (numFilesProcessed != 0)
            {     
                CString statsFilePath(m_TCLoggerDataReader.GetStatsFilePath().c_str());
                text += _T("A summary of max and min sensor values was generated at\n") + statsFilePath + _T("\n\n");
            }

            CString logFilePath(m_TCLoggerDataReader.GetLogFilePath().c_str());
            text += _T("A log file was generated at\n") + logFilePath;

            MessageBox(text, caption, MB_OK);
        }
        else
        {
            CString text = _T("");
            CString caption = _T("");
            if(m_configFilePath == "")
            {
                text += _T("Error: No config file selected, no conversion performed\n");
            }
            else
            {
                text += _T("Error: No config file exists at path \"") + m_configFilePath + _T("\", No conversion performed\n");
            }
            MessageBox(text, caption, MB_OK);
        }
    }
    else
    {
        CString text = _T("");
        if (m_dataPath == "")
        {
            text = _T("No valid data directory has been set");
        }
        else
        {
            text = _T("Error: directory at path \n\"") + m_dataPath + _T("\"\n does not exist");
        }
        CString caption("Error: Invalid Directory Entry");
        MessageBox(text, caption, MB_OK);
        m_dataDirBrowser.SetWindowTextW(NULL);
        ResetDataDirIniFile();
    }
}
