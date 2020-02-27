
// FBLoggerUtilityDlg.cpp : implementation file
//

#include "stdafx.h"
#include "FBLoggerUtility.h"
#include "FBLoggerUtilityDlg.h"
#include "afxdialogex.h"
#include "StringUtility.h"
#include "FileNameUtility.h"
#include "DirectoryReaderUtility.h"

#include <omp.h>

#include <memory>

#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>          // std::mutex, std::unique_lock, std::defer_lock

#include "stdlib.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

std::mutex mtx;           // mutex for critical section

UINT DatFileProcessRoutine(LPVOID lpParameter)
{   
    CFBLoggerUtilityDlg* pThis = static_cast<CFBLoggerUtilityDlg*>(lpParameter);
    return pThis->ProcessAllDatFiles();
}

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

// CFBLoggerGUIDlg dialog

CFBLoggerUtilityDlg::CFBLoggerUtilityDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CFBLoggerUtilityDlg::IDD, pParent),
    m_pProgressBarDlg(NULL)
{
    m_workerThread = NULL;
    m_ThreadID = NULL;
    m_numInvalidFiles = 0;
    m_numFilesProcessed = 0;
    m_numFilesConverted = 0;
    m_hKillEvent = NULL;
    m_createRawTicked = false;
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CFBLoggerUtilityDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_DATADIRBROWSE, m_dataDirBrowser);
    DDX_Control(pDX, IDC_CONFIGFILEBROWSE, m_configFileBrowser);
    DDX_Control(pDX, IDCONVERT, m_btnConvert);
    DDX_Control(pDX, IDC_CHECKRAW, m_ctlCheckRaw);
    DDX_Control(pDX, IDC_BURNNAMEEDIT, m_burnNameEdit);
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
    ON_BN_CLICKED(IDCANCEL, &CFBLoggerUtilityDlg::OnBnClickedCancel)
    ON_MESSAGE(CANCEL_PROCESSING, &CFBLoggerUtilityDlg::OnCancelProcessing)
    ON_MESSAGE(WORKER_THREAD_RUNNING, &CFBLoggerUtilityDlg::OnWorkerThreadRunning)
    ON_MESSAGE(WORKER_THREAD_DONE, &CFBLoggerUtilityDlg::OnWorkerThreadDone)
    ON_BN_CLICKED(IDC_CHECKRAW, &CFBLoggerUtilityDlg::OnBnClickedCreateRaw)
    ON_EN_CHANGE(IDC_BURNNAMEEDIT, &CFBLoggerUtilityDlg::OnEnChangeBurnNameEdit)
END_MESSAGE_MAP()

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
    m_appPath = m_appPath + _T("\\");
    CT2CA pszConvertedAnsiStringAppPath(m_appPath); // Convert a TCHAR string to a LPCSTR
    std::string strAppPath(pszConvertedAnsiStringAppPath);
   
    m_appIniPath = m_appPath + _T("FBLoggerUtility.ini");

    m_windTunnelDataFileName = "TG201811071345.txt";

    CString windTunnelDataTablePathTempCString = m_appPath + m_windTunnelDataFileName;
    CT2CA pszConvertedAnsiStringWindTunnelDataTablePath(windTunnelDataTablePathTempCString);
    std::string windTunnelDataTablePathTempStdString(pszConvertedAnsiStringWindTunnelDataTablePath);
    m_windTunnelDataTablePath = windTunnelDataTablePathTempStdString;

    if (!PathFileExists(windTunnelDataTablePathTempCString))
    {
        CString missingWindTunnelFileText = _T("Fatal Error: Missing file ") + m_windTunnelDataFileName + _T("\nContact RMRS Fire Lab for support\nExiting application");
        AfxMessageBox(missingWindTunnelFileText);
        PostQuitMessage(0);
    }
   
    if (!PathFileExists(m_appIniPath))
    {
        std::ofstream outfile(m_appIniPath);
        if (outfile.fail())
        {
            AfxMessageBox(_T("Fatal Error creating ini file, Exiting application"));
            outfile.close();
            PostQuitMessage(0);
        }
        else
        {
            m_dataPath = "";
            m_dataDirBrowser.SetWindowTextW(NULL);
            m_configFilePath = "";
            m_configFileBrowser.SetWindowTextW(NULL);

            outfile << "fb_data_path=\n";
            outfile << "fb_config_file=\n";
            outfile << "fb_create_raw=\n";
            outfile.close();
        }
    }
    ProcessIniFile();

    if (!m_pToolTipCtrl.Create(this, TTS_ALWAYSTIP))
    {
        TRACE(_T("Unable To create ToolTip\n"));
        return FALSE;
    }

    m_hKillEvent = NULL;
    m_workerThread = NULL;

    m_pToolTipCtrl.AddTool(&m_dataDirBrowser, m_dataPath);
    m_pToolTipCtrl.AddTool(&m_configFileBrowser, m_configFilePath);
    m_pToolTipCtrl.SetDelayTime(TTDT_AUTOPOP, 300000); // stay up for 5 minutes
    m_pToolTipCtrl.Activate(TRUE);

    EnableToolTips(TRUE);

    m_workerThreadCount.store(0);
    m_waitForWorkerThread.store(false);
    m_hKillEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

void CFBLoggerUtilityDlg::InitProgressBarDlg()
{
    m_pProgressBarDlg = new CProgressBarDlg(this);
    BOOL ret = m_pProgressBarDlg->Create(IDD_PROGRESS_BAR_DLG);
    if (!ret)   //Create failed.
    {
        AfxMessageBox(_T("Error creating Dialog"));
        delete m_pProgressBarDlg;
    }
    m_pProgressBarDlg->ShowWindow(SW_SHOW);
    m_pProgressBarDlg->CenterWindow(AfxGetMainWnd());
}

void CFBLoggerUtilityDlg::UpdateIniFile()
{
    if (PathFileExists(m_appIniPath))
    {
        std::ofstream outfile(m_appIniPath);
        outfile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        // convert a TCHAR string to a LPCSTR
        CT2CA pszConvertedAnsiStringDataPath(_T("fb_data_path=") + m_dataPath);
        // construct a std::string using the LPCSTR input
        std::string strStdDataPath(pszConvertedAnsiStringDataPath);
        // convert a TCHAR string to a LPCSTR
        CT2CA pszConvertedAnsiStringConfigFilePath(_T("fb_config_file=") + m_configFilePath);
        // construct a std::string using the LPCSTR input
        std::string strStdConfigFilePath(pszConvertedAnsiStringConfigFilePath);

        if (PathFileExists(m_dataPath))
        {
            outfile << strStdDataPath << "\n";
        }
        else
        {
            outfile << "fb_data_path=\n";
        }

        if (PathFileExists(m_configFilePath))
        {
            outfile << strStdConfigFilePath << "\n";
        }
        else
        {
            outfile << "fb_config_file=\n";
        }

        if (m_createRawTicked)
        {
            outfile << "fb_create_raw=true";
        }
        else
        {
            outfile << "fb_create_raw=false";
        }

        outfile.close();
    }
}

// CFBLoggerGUIDlg message handlers
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
        lstrcpyn(pTTTA->szText, strTipText, ::std::size(pTTTA->szText));
    else
        _mbstowcsz(pTTTW->szText, strTipText, ::std::size(pTTTW->szText));
#else
    if (pNMHDR->code == TTN_NEEDTEXTA)
    {
        _wcstombsz(pTTTA->szText, strTipText, (ULONG)::std::size(pTTTA->szText));
    }
    else
    {
        lstrcpyn(pTTTW->szText, strTipText, (int)::std::size(pTTTW->szText));
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

UINT CFBLoggerUtilityDlg::ProcessAllDatFiles()
{
    bool aborted = false;
    bool isGoodWindTunnelTable = false;
    PostMessage(WORKER_THREAD_RUNNING, 0, 0);
    clock_t startClock = clock();

    unique_ptr<LoggerDataReader> globalLoggerDataReader( new LoggerDataReader(NarrowCStringToStdString(m_dataPath), NarrowCStringToStdString(m_burnName)));
    unique_ptr<FBLoggerLogFile> logFile(new FBLoggerLogFile (NarrowCStringToStdString(m_dataPath)));
    globalLoggerDataReader->SetConfigFile(NarrowCStringToStdString(m_configFilePath));
    m_createRawTicked = (m_ctlCheckRaw.GetCheck() == TRUE) ? true : false;
    globalLoggerDataReader->SetPrintRaw(m_createRawTicked);

    int totalNumberOfFiles = 0;

    vector<string> logFileLinesPerFile;

    // Shared accumulated data
    int totalInvalidInputFiles = 0;
    int totalFilesProcessed = 0;
    int totalNumErrors = 0;
    float flProgress = 0;

    // Check log file, check config file, process input files, create output files 
    if (logFile->IsLogFileGood())
    { 
        globalLoggerDataReader->CheckConfig();
        // Load data in from wind tunnel data table
        globalLoggerDataReader->SetWindTunnelDataTablePath(m_windTunnelDataTablePath);
        isGoodWindTunnelTable = globalLoggerDataReader->CreateWindTunnelDataVectors();
        SharedData sharedData;
        globalLoggerDataReader->GetSharedData(sharedData);
        logFile->logFileLines_ = globalLoggerDataReader->GetLogFileLines();

        if (globalLoggerDataReader->IsConfigFileValid() && isGoodWindTunnelTable)
        {
            totalNumberOfFiles = globalLoggerDataReader->GetNumberOfInputFiles();
            int i = 0;
            // check kill
            DWORD dwRet = WaitForSingleObject(m_hKillEvent, 0);
            if (dwRet == WAIT_OBJECT_0)
            {
                aborted = true;
            }
            if (totalNumberOfFiles > 0)
            {   
                for (int i = 0; i < totalNumberOfFiles; i++)
                {
                    logFileLinesPerFile.push_back("");
                }

#pragma omp parallel for schedule(dynamic, 1) shared(i, aborted, totalFilesProcessed, totalInvalidInputFiles, totalNumErrors, flProgress, logFile)      
                for (i = 0; i < totalNumberOfFiles; i++)
                {
                    if (!aborted)
                    {
                        // Need to pass needed maps and vectors to a FBLoggerDataReader
                        unique_ptr<LoggerDataReader> localLoggerDataReader(new LoggerDataReader(sharedData));

                        localLoggerDataReader->ProcessSingleDataFile(i);
                       
                        // critical section
#pragma omp critical(updateLogAndProgress)
                        {
                            totalFilesProcessed += localLoggerDataReader->GetNumFilesProcessed();
                            totalInvalidInputFiles += localLoggerDataReader->GetNumInvalidFiles();
                            totalNumErrors += localLoggerDataReader->GetNumErrors();
                            logFileLinesPerFile[i] = localLoggerDataReader->GetLogFileLines();

                            flProgress = (static_cast<float>(totalFilesProcessed) / totalNumberOfFiles) * (static_cast<float>(100.0));
                            ::PostMessage(m_pProgressBarDlg->GetSafeHwnd(), UPDATE_PROGRESSS_BAR, (WPARAM)static_cast<int>(flProgress), (LPARAM)0);
                            if (i == totalNumberOfFiles - 1)
                            {
                                ::PostMessage(m_pProgressBarDlg->GetSafeHwnd(), UPDATE_PROGRESSS_BAR, (WPARAM)100, (LPARAM)0);
                                ::PostMessage(m_pProgressBarDlg->GetSafeHwnd(), CLOSE_PROGRESSS_BAR, (WPARAM)0, (LPARAM)0);
                            }

                            // check kill
                            DWORD dwRet = WaitForSingleObject(m_hKillEvent, 0);
                            if (dwRet == WAIT_OBJECT_0)
                            {
                                aborted = true;        
                            }
                        }
                        // end critical section
                    }
                }
            }
            else
            {   
                ::PostMessage(m_pProgressBarDlg->GetSafeHwnd(), CLOSE_PROGRESSS_BAR, (WPARAM)0, (LPARAM)0);
            }
        }
    }

    if (aborted)
    {
        ::PostMessage(m_pProgressBarDlg->GetSafeHwnd(), CLOSE_PROGRESSS_BAR, (WPARAM)0, (LPARAM)0);

        double totalTimeInSeconds = ((double)clock() - (double)startClock) / (double)CLOCKS_PER_SEC;

        globalLoggerDataReader->ReportAbort();
        logFile->PrintFinalReportToLogFile(totalTimeInSeconds, totalInvalidInputFiles, totalFilesProcessed, totalNumErrors);
        string tempText;
        string logFilePath(logFile->GetLogFilePath());
        CString numFilesConvertedCString;
        CString numInvalidFilesCString;
        CString totalTimeInSecondsCString;

        int numFilesConverted = totalFilesProcessed - totalInvalidInputFiles;
        numFilesConvertedCString.Format(_T("%d"), numFilesConverted);
        numInvalidFilesCString.Format(_T("%d"), (int)totalInvalidInputFiles);
        totalTimeInSecondsCString.Format(_T("%.2f"), totalTimeInSeconds);

        tempText = "Conversion aborted by user\n";
        tempText += NarrowCStringToStdString(numFilesConvertedCString);
        tempText += " .DAT files were converted to .csv files into\n" + NarrowCStringToStdString(m_dataPath) + "\n";
        tempText += " in " + NarrowCStringToStdString(totalTimeInSecondsCString) + " seconds\n\n";
        tempText += "A log file was generated at\n" + logFilePath;
        CString text(tempText.c_str());
        CString caption = _T("");
        caption = ("Aborted Conversion");

        MessageBox(text, caption, MB_OK);
    }
    else
    {
        double totalTimeInSeconds = ((double)clock() - (double)startClock) / (double)CLOCKS_PER_SEC;

        //globalLoggerDataReader.PrintStatsFile();

        CString numFilesConvertedCString;
        CString numInvalidFilesCString;
        CString totalTimeInSecondsCString;

        int numFilesConverted = totalFilesProcessed - totalInvalidInputFiles;
        numFilesConvertedCString.Format(_T("%d"), numFilesConverted);
        numInvalidFilesCString.Format(_T("%d"), (int)totalInvalidInputFiles);
        totalTimeInSecondsCString.Format(_T("%.2f"), totalTimeInSeconds);
        CString text = _T("");
        CString caption = _T("");

        for (int i = 0; i < logFileLinesPerFile.size(); i++)
        {
            logFile->AddToLogFile(logFileLinesPerFile[i]);
        }
        logFile->PrintFinalReportToLogFile(totalTimeInSeconds, totalInvalidInputFiles, totalFilesProcessed, totalNumErrors);

        if (!isGoodWindTunnelTable)
        {
            text = _T("There are errors in the wind tunnel data table file");
            caption = ("Error: Invalid wind tunnel data table file");
        }

        if (!globalLoggerDataReader->IsConfigFileValid())
        {
            text = _T("There are errors in config file at\n");
            text += m_dataPath + _T("\\config.csv\n\n");
            caption = ("Error: Invalid config file");
        }
        else if (totalFilesProcessed == 0)
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
            tempText += " .DAT files to .csv files to\n" + NarrowCStringToStdString(m_dataPath) + "\n";
            tempText += " in " + NarrowCStringToStdString(totalTimeInSecondsCString) + " seconds\n\n";
            text += tempText.c_str();

            caption = ("Done converting files");
        }

        if (totalInvalidInputFiles > 0)
        {
            if (globalLoggerDataReader->GetNumInvalidFiles() == 1)
            {
                text += numInvalidFilesCString + _T(" invalid file was unable to be converted\n");
            }
            else
            {
                text += numInvalidFilesCString + _T(" invalid files were unable to be converted\n");
            }
        }

        if (totalFilesProcessed != 0)
        {
            CString statsFilePath(globalLoggerDataReader->GetStatsFilePath().c_str());
            text += _T("A summary of max and min sensor values was generated at\n") + statsFilePath + _T("\n\n");
        }
        else
        {
            ::PostMessage(m_pProgressBarDlg->GetSafeHwnd(), CLOSE_PROGRESSS_BAR, (WPARAM)0, (LPARAM)0);
        }

        CString logFilePath(logFile->GetLogFilePath().c_str());
        text += _T("A log file was generated at\n") + logFilePath;

        MessageBox(text, caption, MB_OK);
    }

    m_waitForWorkerThread.store(false);
    m_workerThreadCount.fetch_add(-1);

    PostMessage(WORKER_THREAD_DONE, 0, 0);
    // normal thread exit
    return 0;
}

void CFBLoggerUtilityDlg::ProcessIniFile()
{
    bool isDataPathValid;
    bool isConfigurationTypeValid;

   

    ifstream infile(m_appIniPath);
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
            if (token == "fb_data_path")
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
                    m_dataPath = "";
                    m_dataDirBrowser.SetWindowTextW(NULL);
                }
            }
            if (token == "fb_config_file")
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
                        CString text = _T("Error: No config file exists at path \n\"") + m_configFilePath + _T("\"\n");
                        CString caption("Error: Previous Config File Entry Invalid");
                        MessageBox(text, caption, MB_OK);
                    }
                    m_configFilePath = "";
                    m_configFileBrowser.SetWindowTextW(NULL);
                }
            }
            if (token == "fb_create_raw")
            {
                getline(lineStream, token);
                
                if (token == "true")
                {
                    m_ctlCheckRaw.SetCheck(TRUE);
                }
                else if (token == "false")
                {
                    m_ctlCheckRaw.SetCheck(FALSE);
                    m_createRawTicked = false;
                }
                else
                {
                    m_ctlCheckRaw.SetCheck(FALSE);
                    m_createRawTicked = false;
                }
            }
        }
    }
    infile.close();
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
    else if ((nID & 0xFFF0) == SC_CLOSE)
    {
        UpdateIniFile();
        if (m_waitForWorkerThread.load())
        {
            SetEvent(m_hKillEvent); // Kill the worker thread
        }
        else
        {
            PostQuitMessage(0);
        }
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
}

void CFBLoggerUtilityDlg::OnBnClickedConvert()
{
    bool configFileExists = false;

    if (!m_waitForWorkerThread.load())
    {
        m_waitForWorkerThread.store(true);

        ResetEvent(m_hKillEvent); // Don't kill Worker thread

        if (m_burnName == "")
        {
            m_burnName = "burn";
        }
        else
        {
            ReplaceIllegalCharacters(m_burnName);
        }

        UpdateIniFile();

        if (PathFileExists(m_dataPath))
        {
            if (PathFileExists(m_configFilePath))
            {
                configFileExists = true;
            }
            else
            {
                CString text = _T("");
                CString caption = _T("");
                if (m_configFilePath == "")
                {
                    text += _T("Error: No config file selected, no conversion performed\n");
                }
                else
                {
                    text += _T("Error: No config file exists at path \"") + m_configFilePath + _T("\", No conversion performed\n");
                }

                m_configFileBrowser.SetWindowTextW(NULL);
                m_waitForWorkerThread.store(false);
                MessageBox(text, caption, MB_OK);
            }

            if (configFileExists && m_workerThreadCount.load() < 1)
            {

                //std::ifstream configFile(m_configFilePath, std::ios::in | std::ios::binary);
                //FBLoggerDataReader loggerDataReader(NarrowCStringToStdString(m_dataPath), NarrowCStringToStdString(m_burnName));
                //bool configIsValid = loggerDataReader.CheckConfigFileFormatIsValid(configFile);
                bool configIsValid = true;
                if (configIsValid)
                {
                    m_workerThreadCount.fetch_add(1);
                    InitProgressBarDlg();
                    m_workerThread = AfxBeginThread(DatFileProcessRoutine, this);
                }
                else
                {
                    CString text = _T("");
                    CString caption = _T("");
                    text += _T("Error: Config file at path \"") + m_configFilePath + _T("\", is invalid. No conversion performed\n");

                    m_configFileBrowser.SetWindowTextW(NULL);
                    m_waitForWorkerThread.store(false);
                    MessageBox(text, caption, MB_OK);
                }
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

            m_dataDirBrowser.SetWindowTextW(NULL);
            m_waitForWorkerThread.store(false);
            MessageBox(text, caption, MB_OK);
        }
    }
    m_burnNameEdit.SetWindowTextW(NULL);
}

void CFBLoggerUtilityDlg::OnBnClickedCancel()
{
    if (m_waitForWorkerThread.load())
    {
        SetEvent(m_hKillEvent); // Kill the worker thread
        DWORD dwRet = WaitForSingleObject(m_workerThread->m_hThread, INFINITE); // Wait for worker thread to shutdown
    }

    CDialogEx::OnCancel();

    std::ofstream outfile(m_appIniPath);
    // convert a TCHAR string to a LPCSTR
    CT2CA pszConvertedAnsiStringDataPath(_T("fb_data_path=") + m_dataPath);
    // construct a std::string using the LPCSTR input
    std::string strStdDataPath(pszConvertedAnsiStringDataPath);

    // convert a TCHAR string to a LPCSTR
    CT2CA pszConvertedAnsiStringConfigFilePath(_T("fb_config_file=") + m_configFilePath);
    // construct a std::string using the LPCSTR input
    std::string strStdConfigFilePath(pszConvertedAnsiStringConfigFilePath);

    if (PathFileExists(m_dataPath))
    {
        outfile << strStdDataPath << "\n";
    }
    else
    {
        outfile << "fb_data_path=\n";
    }

    if (PathFileExists(m_configFilePath))
    {
        outfile << strStdConfigFilePath << "\n";
    }
    else
    {
        outfile << "fb_config_file=\n";
    }

    if (m_createRawTicked)
    {
        outfile << "fb_create_raw=true";
    }
    else
    {
        outfile << "fb_create_raw=false";
    }

    outfile.close();
}

LRESULT CFBLoggerUtilityDlg::OnCancelProcessing(WPARAM, LPARAM)
{
    if (m_waitForWorkerThread.load())
    {
        SetEvent(m_hKillEvent); // Kill the worker thread
        DWORD dwRet = WaitForSingleObject(m_workerThread->m_hThread, 0); // Wait for worker thread to safely shutdown
    }
    return 0;
}

LRESULT CFBLoggerUtilityDlg::OnWorkerThreadRunning(WPARAM, LPARAM)
{
    m_btnConvert.ShowWindow(FALSE);
    m_ctlCheckRaw.ShowWindow(FALSE);
    return 0;
}

LRESULT CFBLoggerUtilityDlg::OnWorkerThreadDone(WPARAM, LPARAM)
{
    m_btnConvert.ShowWindow(TRUE);
    m_ctlCheckRaw.ShowWindow(TRUE);
    return 0;
}

void CFBLoggerUtilityDlg::OnBnClickedCreateRaw()
{
    // TODO: Add your control notification handler code here
    m_createRawTicked = (m_ctlCheckRaw.GetCheck() == TRUE) ? true : false;
}


void CFBLoggerUtilityDlg::OnEnChangeBurnNameEdit()
{
    // TODO:  If this is a RICHEDIT control, the control will not
    // send this notification unless you override the CDialogEx::OnInitDialog()
    // function and call CRichEditCtrl().SetEventMask()
    // with the ENM_CHANGE flag ORed into the mask.

    // TODO:  Add your control notification handler code here
    m_burnNameEdit.GetWindowTextW(m_burnName);
}
