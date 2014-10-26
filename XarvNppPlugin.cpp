// XarvNppPlugin.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


#include "PluginDefinition.h"
#include "menuCmdID.h"
#include "PreViewDlg.h"
#include "GoToLineDlg.h"


//
// The plugin data that Notepad++ needs
//
FuncItem funcItem[nbFunc];

//
// The data of Notepad++ that you can use in your plugin commands
//
NppData nppData;
extern CPreviewDlg* dlg;
DemoDlg _goToLine;

// 打开程序是否初始化好预览对话框
BOOL bReady = FALSE;
//
// Initialize your plugin data here
// It will be called while plugin loading   
void pluginInit(HANDLE hModule)
{
	_goToLine.init((HINSTANCE)hModule, NULL);
}

//
// Here you can do the clean up, save the parameters (if any) for the next session
//
void pluginCleanUp()
{
}

void ShowPreviewDlg()
{
	_goToLine.setParent(nppData._nppHandle);
	tTbData	data = {0};

	if (!_goToLine.isCreated())
	{
		_goToLine.create(&data);

		// define the default docking behaviour
		data.uMask = DWS_DF_CONT_RIGHT;

		data.pszModuleName = _goToLine.getPluginFileName();

		// the dlgDlg should be the index of funcItem where the current function pointer is
		// in this case is DOCKABLE_DEMO_INDEX
		data.dlgID = 2;
		::SendMessage(nppData._nppHandle, NPPM_DMMREGASDCKDLG, 0, (LPARAM)&data);
	}
	_goToLine.display();
	bReady = TRUE;
	
}

//
// Initialization of your plugin commands
// You should fill your plugins commands here
void commandMenuInit()
{

	//--------------------------------------------//
	//-- STEP 3. CUSTOMIZE YOUR PLUGIN COMMANDS --//
	//--------------------------------------------//
	// with function :
	// setCommand(int index,                      // zero based number to indicate the order of command
	//            TCHAR *commandName,             // the command name that you want to see in plugin menu
	//            PFUNCPLUGINCMD functionPointer, // the symbol of function (function pointer) associated with this command. The body should be defined below. See Step 4.
	//            ShortcutKey *shortcut,          // optional. Define a shortcut to trigger this command
	//            bool check0nInit                // optional. Make this menu item be checked visually
	//            );
	setCommand(0, TEXT("Show Markdown Preview Dialog"), ShowPreviewDlg, NULL, false);
}

//
// Here you can do the clean up (especially for the shortcut)
//
void commandMenuCleanUp()
{
	// Don't forget to deallocate your shortcut here
}


//
// This function help you to initialize your plugin commands
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit) 
{
	if (index >= nbFunc)
		return false;

	if (!pFunc)
		return false;

	lstrcpy(funcItem[index]._itemName, cmdName);
	funcItem[index]._pFunc = pFunc;
	funcItem[index]._init2Check = check0nInit;
	funcItem[index]._pShKey = sk;

	return true;
}