//this file is part of notepad++
//Copyright (C)2003 Don HO ( donho@altern.org )
//
//This program is free software; you can redistribute it and/or
//modify it under the terms of the GNU General Public License
//as published by the Free Software Foundation; either
//version 2 of the License, or (at your option) any later version.
//
//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.
//
//You should have received a copy of the GNU General Public License
//along with this program; if not, write to the Free Software
//Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#include "stdafx.h"
#include "GoToLineDlg.h"
#include "PluginDefinition.h"

extern NppData nppData;

BOOL CALLBACK DemoDlg::run_dlgProc(UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message) 
	{
		case WM_COMMAND : 
		{
			switch (wParam)
			{
				case IDOK :
				{
					return TRUE;
				}
			}
				return FALSE;
		}
		break;

		case WM_SIZE:
			{
				if (dlg)
				{
					// 调整预览对话框的大小
					WTL::CString str;
					int x = ((int)(short)LOWORD(lParam));
					int y = ((int)(short)HIWORD(lParam));
					dlg->ReSize(x, y);
					dlg->ShowWindow(SW_SHOW);
				}
				return TRUE;
			}

			break;
		default :
			return DockingDlgInterface::run_dlgProc(message, wParam, lParam);
	}
}

