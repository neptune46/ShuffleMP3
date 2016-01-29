
// ShuffleMP3.h : main header file for the PROJECT_NAME application
//

#pragma once

#ifndef __AFXWIN_H__
    #error "include 'stdafx.h' before including this file for PCH"
#endif

#include "resource.h"		// main symbols


// CShuffleMP3App:
// See ShuffleMP3.cpp for the implementation of this class
//

class CShuffleMP3App : public CWinApp
{
public:
    CShuffleMP3App();

// Overrides
public:
    virtual BOOL InitInstance();

// Implementation

    DECLARE_MESSAGE_MAP()
};

extern CShuffleMP3App theApp;