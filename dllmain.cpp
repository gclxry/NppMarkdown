// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "PluginDefinition.h"
#include "PreViewDlg.h"
#include "GoToLineDlg.h"
#include <atlfile.h>
#include <vector>
#include <fstream>
#include <sstream>
#include <process.h>
#include "markdown.h"
#include <atlconv.h>
#include <atlctrls.h>
using namespace std;

extern FuncItem funcItem[nbFunc];
extern NppData nppData;
CAppModule _Module;

HMODULE g_Module;

extern DemoDlg _goToLine;
extern BOOL bReady;




class CPerformanceWatch
{
public:
	CPerformanceWatch()
	{
		QueryPerformanceFrequency(&m_liPerfFreq);
		Start();
	}

	// ∑µªÿ÷¥––√Î
	__int64 Now() const
	{
		LARGE_INTEGER liPerfNow;
		QueryPerformanceCounter(&liPerfNow);
		return (((liPerfNow.QuadPart - m_liPerfStart.QuadPart) * 1000) / m_liPerfFreq.QuadPart);
	}
	// ∑µªÿ÷¥––Œ¢√Ó
	__int64 NowInMicro() const
	{
		LARGE_INTEGER liPerfNow;
		QueryPerformanceCounter(&liPerfNow);
		return (((liPerfNow.QuadPart - m_liPerfStart.QuadPart) * 1000000) / m_liPerfFreq.QuadPart);
	}

private:
	LARGE_INTEGER m_liPerfFreq;
	LARGE_INTEGER m_liPerfStart;

	void Start()
	{
		QueryPerformanceCounter(&m_liPerfStart);
	}
};



BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		pluginInit(hModule);
		g_Module = hModule;
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		commandMenuCleanUp();
		pluginCleanUp();
		break;
	}
	return TRUE;
}

extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData)
{
	nppData = notpadPlusData;

	commandMenuInit();
}

extern "C" __declspec(dllexport) const TCHAR * getName()
{
	return NPP_PLUGIN_NAME;
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int *nbF)
{
	*nbF = nbFunc;
	return funcItem;
}



void vSplitString( wstring& strSrc , vector<wstring>& vecDest , char cSeparator)
{
	if( strSrc.empty() )
		return ;

	wstring::size_type size_pos = 0;
	wstring::size_type size_prev_pos = 0;

	while( (size_pos = strSrc.find_first_of( cSeparator , size_pos)) != wstring::npos)
	{
		wstring strTemp = strSrc.substr(size_prev_pos , size_pos - size_prev_pos );
		vecDest.push_back(strTemp);
		size_prev_pos = ++size_pos;
		MessageBox(NULL, strTemp.c_str(), L"", 0);
	}
}


unsigned _stdcall ThreadProc(void* param)
{

	return 0;
}


extern "C" __declspec(dllexport) void beNotified(SCNotification *notifyCode)
{
	if (notifyCode->nmhdr.code == SCN_MODIFIED && bReady)
//	if (notifyCode->line)
	{
		if (_goToLine.dlg)
		{
			CButton check;
			HWND hwndCheck = ::GetDlgItem(_goToLine.dlg->m_hWnd, IDC_CHECK_LIVE_PREVIEW);
			check.Attach(hwndCheck);
			BOOL bCheck = check.GetCheck();
			if (!bCheck)
			{
				return;
			}
			_goToLine.dlg->Tans();
		}
	}
}


// Here you can process the Npp Messages 
// I will make the messages accessible little by little, according to the need of plugin development.
// Please let me know if you need to access to some messages :
// http://sourceforge.net/forum/forum.php?forum_id=482781
//
extern "C" __declspec(dllexport) LRESULT messageProc(UINT Message, WPARAM wParam, LPARAM lParam)
{/*
 if (Message == WM_MOVE)
 {
 ::MessageBox(NULL, "move", "", MB_OK);
 }
 */
	return TRUE;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode()
{
	return TRUE;
}
#endif