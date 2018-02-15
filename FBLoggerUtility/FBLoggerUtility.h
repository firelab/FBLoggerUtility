
// FBLoggerUtility.h : main header file for the Fire Behavior Logger application
//

#pragma once

#ifndef __AFXWIN_H__
	#error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CFBLoggerUtilityApp:
// See FBLoggerUtility.cpp for the implementation of this class
//

class CFBLoggerUtilityApp : public CWinApp
{
public:
    CFBLoggerUtilityApp();

// Overrides
public:
	virtual BOOL InitInstance();

// Implementation

	DECLARE_MESSAGE_MAP()
};

extern CFBLoggerUtilityApp theApp;