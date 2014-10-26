/********************************************************************


purpose:	XUi 界面库的基于ie控件的封装，包括对 IDispatchEx 的智能指针的封装
*********************************************************************/
#pragma once

#include <exdispid.h>
#include <dispex.h>
#include <WinInet.h>
#include <atlstr.h>
#include <shellapi.h>

//specialization for IDispatchEx
template <>
class CComPtr<IDispatchEx> : public CComPtrBase<IDispatchEx>
{
public:
	CComPtr() throw()
	{
	}
	CComPtr(IDispatchEx* lp) throw() :
	CComPtrBase<IDispatchEx>(lp)
	{
	}
	CComPtr(const CComPtr<IDispatchEx>& lp) throw() :
	CComPtrBase<IDispatchEx>(lp.p)
	{
	}
	IDispatchEx* operator=(IDispatchEx* lp) throw()
	{
		if(*this!=lp)
		{
			return static_cast<IDispatchEx*>(AtlComPtrAssign((IUnknown**)&p, lp));
		}
		return *this;
	}
	IDispatchEx* operator=(const CComPtr<IDispatchEx>& lp) throw()
	{
		if(*this!=lp)
		{
			return static_cast<IDispatchEx*>(AtlComPtrAssign((IUnknown**)&p, lp.p));
		}
		return *this;
	}
	HRESULT GetProperty(LPCOLESTR lpszKey, int* pIntValue)
	{
		CComVariant varRet;

		if (GetProperty(lpszKey, &varRet))
		{
			if (SUCCEEDED(varRet.ChangeType(VT_I4)))
			{
				*pIntValue = varRet.lVal;
				return S_OK;
			}
		}
		return E_FAIL;
	}
	HRESULT GetProperty(LPCOLESTR lpszKey, BSTR* pBSTRValue)
	{
		CComVariant varRet;

		if (GetProperty(lpszKey, &varRet))
		{
			if (SUCCEEDED(varRet.ChangeType(VT_BSTR)))
			{
				CComBSTR bstrRet = varRet.bstrVal;
				*pBSTRValue = bstrRet.Detach();
				return S_OK;
			}
		}
		return E_FAIL;
	}
	HRESULT GetProperty(LPCOLESTR lpszKey, VARIANT* pvarRet)
	{
		return GetProperty(p, lpszKey, pvarRet);
	}
	HRESULT PutProperty(int iKey, int iValue)
	{
		WCHAR szText[MAX_PATH];
		_ltow_s(iKey, szText, MAX_PATH, 10);

		CComVariant varValue(iValue);
		return PutProperty(szText, &varValue);
	}
	HRESULT PutProperty(int iKey, LPCOLESTR lpszValue)
	{
		WCHAR szText[MAX_PATH];
		_ltow_s(iKey, szText, MAX_PATH, 10);

		CComVariant varValue(lpszValue);
		return PutProperty(szText, &varValue);
	}
	HRESULT PutProperty(LPCOLESTR lpszKey, bool bValue)
	{
		CComVariant varValue(bValue);
		return PutProperty(lpszKey, &varValue);
	}
	HRESULT PutProperty(LPCOLESTR lpszKey, int iValue)
	{
		CComVariant varValue(iValue);
		return PutProperty(lpszKey, &varValue);
	}
	HRESULT PutProperty(LPCOLESTR lpszKey, LPCOLESTR lpszValue)
	{
		CComVariant varValue(lpszValue);
		return PutProperty(lpszKey, &varValue);
	}
	HRESULT PutProperty(LPCOLESTR lpszKey, VARIANT* pvarValue)
	{
		return PutProperty(p, lpszKey, pvarValue);
	}
	HRESULT RemoveProperty(LPCOLESTR lpszKey)
	{
		ATLASSERT(p);

		if(p == NULL)
			return E_INVALIDARG;

		return p->DeleteMemberByName(CComBSTR(lpszKey), fdexNameCaseSensitive);
	}
	HRESULT RemoveAllPropertys()
	{
		ATLASSERT(p);

		if(p == NULL)
			return E_INVALIDARG;

		CSimpleArray<DISPID> propertys;

		DISPID dispID = DISPID_STARTENUM;

		while (p->GetNextDispID(fdexEnumAll, dispID, &dispID) == S_OK)
			propertys.Add(dispID);

		for (int i = 0; i < propertys.GetSize(); i++)
			p->DeleteMemberByDispID(propertys[i]);

		return S_OK;
	}
	static HRESULT PutProperty(IDispatchEx* p, LPCOLESTR lpszKey, VARIANT* pvarValue)
	{
		ATLASSERT(p);
		ATLASSERT(pvarValue != NULL);
		if (pvarValue == NULL)
			return E_POINTER;

		if(p == NULL)
			return E_INVALIDARG;

		do 
		{
			DISPID dispID = DISPID_UNKNOWN;
			if (FAILED(p->GetDispID(CComBSTR(lpszKey), fdexNameCaseSensitive | fdexNameEnsure, &dispID)))
				break;

			DISPPARAMS dispparams = {NULL, NULL, 1, 1};
			dispparams.rgvarg = pvarValue;
			DISPID dispidPut = DISPID_PROPERTYPUT;
			dispparams.rgdispidNamedArgs = &dispidPut;

			if (pvarValue->vt == VT_UNKNOWN || pvarValue->vt == VT_DISPATCH || 
				(pvarValue->vt & VT_ARRAY) || (pvarValue->vt & VT_BYREF))
			{
				HRESULT hr = p->InvokeEx(dispID, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUTREF, &dispparams, NULL, NULL, NULL);
				if (SUCCEEDED(hr))
					return hr;
			}

			if (FAILED(p->InvokeEx(dispID, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYPUT, &dispparams, NULL, NULL, NULL)))
				break;

			return S_OK;

		} while (false);

		return E_FAIL;
	}
	static HRESULT GetProperty(IDispatchEx* p, LPCOLESTR lpszKey, VARIANT* pvarRet) throw()
	{
		ATLASSERT(p);
		ATLASSERT(pvarRet != NULL);

		if (pvarRet == NULL)
			return E_POINTER;

		if(p == NULL)
			return E_INVALIDARG;

		do 
		{
			DISPID dispID = DISPID_UNKNOWN;
			if (FAILED(p->GetDispID(CComBSTR(lpszKey), fdexNameCaseSensitive, &dispID)))
				break;

			DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};

			if (FAILED(p->InvokeEx(dispID, LOCALE_USER_DEFAULT, DISPATCH_PROPERTYGET, &dispparamsNoArgs, pvarRet, NULL, NULL)))
				break;

			return S_OK;

		} while (false);

		return E_FAIL;
	}
};

class CXHtmlWnd : public CWindowImpl<CXHtmlWnd, CAxWindow>
{
public:
	DECLARE_WND_SUPERCLASS(_T("XHtmlView"), CAxWindow::GetWndClassName())

	CXHtmlWnd()
		: m_dwDocHostFlags(0)
		, m_bAllowContextMenu(FALSE)
	{

	}

	HRESULT GetHTMLDocument(IHTMLDocument2** ppHTMLDocument)
	{
		HRESULT hRet = QueryControl(IID_IHTMLDocument2, (void**)ppHTMLDocument);
		if (SUCCEEDED(hRet))
			return hRet;

		CComPtr<IWebBrowser2> spWebBrowser;
		hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser);
		if(SUCCEEDED(hRet) && spWebBrowser != NULL)
		{
			CComPtr<IDispatch> spDocument;
			hRet = spWebBrowser->get_Document(&spDocument);
			if(SUCCEEDED(hRet) && spDocument != NULL)
			{
				CComQIPtr<IHTMLDocument2> spHtmlDoc = spDocument;
				if(spHtmlDoc != NULL)
				{
					*ppHTMLDocument	= spHtmlDoc;
					spHtmlDoc.Detach();
					return S_OK;
				}
			}
		}
		return E_FAIL;
	}

	HRESULT GetScriptDispatch(IDispatch** ppScript)
	{
		if (ppScript == NULL)
			return E_INVALIDARG;

		*ppScript	= NULL;

		do 
		{
			CComPtr<IHTMLDocument2> spDoc2;

			if (FAILED(GetHTMLDocument(&spDoc2)))
				break;

			if (FAILED(spDoc2->get_Script(ppScript)))
				break;

			return S_OK;

		} while (false);
		return E_FAIL;
	}

	HRESULT CreateScriptObject(IDispatchEx** ppScritObject)
	{
		if (ppScritObject == NULL)
			return E_INVALIDARG;

		*ppScritObject	= NULL;

		do 
		{
			CComPtr<IHTMLDocument2> spDoc2;

			if (FAILED(GetHTMLDocument(&spDoc2)))
				break;

			CComPtr<IDispatch> spScript;
			if (FAILED(spDoc2->get_Script(&spScript)))
				break;

			CComVariant vRet;
			if (FAILED(spScript.Invoke1(L"eval", &CComVariant(L"new Object;"), &vRet)))
				break;

			if (vRet.vt != VT_DISPATCH || vRet.pdispVal == NULL)
				break;

			CComQIPtr<IDispatchEx> spRet(vRet.pdispVal);

			if (!spRet)
				break;

			*ppScritObject	= spRet.Detach();

			return S_OK;

		} while (false);

		return E_FAIL;
	}

	HRESULT CreateScriptString(CString strIn, IDispatchEx** ppStringObject)
	{
		if(NULL == ppStringObject)
			return E_INVALIDARG;

		do 
		{
			CComPtr<IHTMLDocument2> spDoc2;

			if (FAILED(GetHTMLDocument(&spDoc2)))
				break;

			CComPtr<IDispatch> spScript;
			if (FAILED(spDoc2->get_Script(&spScript)))
				break;

			strIn.Replace(L"\"", L"'");

			CString strExp;
			strExp.Format(L"new String(\"%s\");", strIn);


			CComVariant vRet;
			if (FAILED(spScript.Invoke1(L"eval", &CComVariant((LPCTSTR)strExp), &vRet)))
				break;

			if (vRet.vt != VT_DISPATCH || vRet.pdispVal == NULL)
				break;

			CComQIPtr<IDispatchEx> spRet(vRet.pdispVal);

			if (!spRet)
				break;

			*ppStringObject	= spRet.Detach();

			return S_OK;

		} while (false);

		return E_FAIL;
	}

	void SetFocusToHTML()
	{
		CComPtr<IHTMLDocument2> spHtmlDoc;
		HRESULT hRet = GetHTMLDocument(&spHtmlDoc);
		if(SUCCEEDED(hRet) && spHtmlDoc != NULL)
		{
			CComPtr<IHTMLWindow2> spParentWindow;
			hRet = spHtmlDoc->get_parentWindow(&spParentWindow);
			if(spParentWindow != NULL)
				spParentWindow->focus();
		}
	}
	BEGIN_MSG_MAP(CXHtmlWnd)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
	END_MSG_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);

		CComPtr<IAxWinAmbientDispatch> spHost;
		HRESULT hRet = QueryHost(IID_IAxWinAmbientDispatch, (void**)&spHost);
		if(SUCCEEDED(hRet))
		{
			hRet = spHost->put_AllowContextMenu(m_bAllowContextMenu ? VARIANT_TRUE : VARIANT_FALSE);
			ATLASSERT(SUCCEEDED(hRet));

			if (m_dwDocHostFlags)
			{
				// Set host flag to indicate that we handle themes
				hRet = spHost->put_DocHostFlags(m_dwDocHostFlags);
				ATLASSERT(SUCCEEDED(hRet));
			}
		}
		if (m_spExternal)
			SetExternalDispatch(m_spExternal);

		return lRet;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		SetExternalDispatch(NULL);

		bHandled	= FALSE;
		return 1;
	}

	LRESULT OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/)
	{
		LRESULT lRet = DefWindowProc(uMsg, wParam, lParam);
		SetFocusToHTML();

		return lRet;
	}
	HWND Create(IDispatch* pExternalDispatch, ATL::_U_STRINGorID htmlResourceID, HWND hWndParent, _U_RECT rect = NULL,
		DWORD dwStyle = 0, DWORD dwExStyle = 0,
		_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL, DWORD dwDocHostFlags = 
		DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU | /*DOCHOSTUIFLAG_SCROLL_NO |*/
		DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_NO3DOUTERBORDER |
		DOCHOSTUIFLAG_THEME | DOCHOSTUIFLAG_FLAT_SCROLLBAR, BOOL bAllowContextMenu = FALSE)
	{
		m_bAllowContextMenu	= bAllowContextMenu;
		m_spExternal		= pExternalDispatch;
		m_dwDocHostFlags	= dwDocHostFlags;
		HWND hWnd	= CWindowImpl<CXHtmlWnd, CAxWindow>::Create(hWndParent, rect, L"mshtml:<html></html>", dwStyle, dwExStyle, MenuOrID, lpCreateParam);

		CResource resHTML;
		resHTML.Load(RT_HTML, htmlResourceID);

		int count = resHTML.GetSize();
		if (count)
		{
			HGLOBAL h = GlobalAlloc(GHND, count);
			if (h)
			{
				BYTE* pBytes = (BYTE*) GlobalLock(h);
				Checked::memcpy_s(pBytes, count, resHTML.Lock(), count);
				GlobalUnlock(h);

				CComPtr<IStream> spStream;
				HRESULT hr = CreateStreamOnHGlobal(h, TRUE, &spStream);

				if (SUCCEEDED(hr))
				{
					CComPtr<IPersistStreamInit> spPSI;
					hr = QueryControl(__uuidof(IPersistStreamInit), (void**)&spPSI);
					if (SUCCEEDED(hr))
					{
						hr = spPSI->Load(spStream);
					}
				}
			}
		}
		return hWnd;
	}
	//HWND Create(IDispatch* pExternalDispatch, LPCWSTR lpURL, HWND hWndParent, _U_RECT rect = NULL,
	//	DWORD dwStyle = 0, DWORD dwExStyle = 0,
	//	_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL, DWORD dwDocHostFlags = 		
	//	DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU | /*DOCHOSTUIFLAG_SCROLL_NO |*/
	//	DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_NO3DOUTERBORDER |
	//	DOCHOSTUIFLAG_THEME | DOCHOSTUIFLAG_FLAT_SCROLLBAR, BOOL bAllowContextMenu = FALSE)
	//{
	//	m_bAllowContextMenu	= bAllowContextMenu;
	//	m_spExternal		= pExternalDispatch;
	//	m_dwDocHostFlags	= dwDocHostFlags;
	//	return CWindowImpl<CXHtmlWnd, CAxWindow>::Create(hWndParent, rect, lpURL, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	//}

// 弹出右键菜单，可以选中文本
	HWND Create(IDispatch* pExternalDispatch, LPCWSTR lpURL, HWND hWndParent, _U_RECT rect = NULL,
		DWORD dwStyle = 0, DWORD dwExStyle = 0,
		_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL, DWORD dwDocHostFlags = 		
		DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU | /*DOCHOSTUIFLAG_SCROLL_NO |*/
		DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_NO3DOUTERBORDER |
		DOCHOSTUIFLAG_THEME | DOCHOSTUIFLAG_FLAT_SCROLLBAR, BOOL bAllowContextMenu = FALSE)
	{
		m_bAllowContextMenu	= TRUE;
		m_spExternal		= pExternalDispatch;
		m_dwDocHostFlags	= dwDocHostFlags &~DOCHOSTUIFLAG_DIALOG;
		return CWindowImpl<CXHtmlWnd, CAxWindow>::Create(hWndParent, rect, lpURL, dwStyle, dwExStyle, MenuOrID, lpCreateParam);
	}
protected:
	BOOL m_bAllowContextMenu;
	DWORD m_dwDocHostFlags;
	CComPtr<IDispatch> m_spExternal;
};

class CXHtmlView : public CXHtmlWnd,
	public IDispEventSimpleImpl<0, CXHtmlView, &DIID_DWebBrowserEvents2>
{
public:
	BEGIN_MSG_MAP(CXHtmlView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		CHAIN_MSG_MAP(CXHtmlWnd)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
	END_MSG_MAP()

	static _ATL_FUNC_INFO NewWindow3_Info;
	BEGIN_SINK_MAP(CXHtmlView)
		SINK_ENTRY_INFO(0, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW3, OnEventNewWindow3, &NewWindow3_Info)
	END_SINK_MAP()

	LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		LRESULT lRet = CXHtmlWnd::OnCreate(uMsg, wParam, lParam, bHandled);
		// Connect events
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if(SUCCEEDED(hRet))
		{
			HRESULT hr = IDispEventSimpleImpl<0, CXHtmlView, &DIID_DWebBrowserEvents2>::DispEventAdvise(spWebBrowser2, &DIID_DWebBrowserEvents2);

			if(FAILED(hr))
				ATLASSERT(FALSE);
		}
		return lRet;
	}

	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
	{
		// Disconnect events
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if(SUCCEEDED(hRet))
			IDispEventSimpleImpl<0, CXHtmlView, &DIID_DWebBrowserEvents2>::DispEventUnadvise(spWebBrowser2, &DIID_DWebBrowserEvents2);

		bHandled	= FALSE;
		return 1;
	}

	void __stdcall OnEventNewWindow3(IDispatch** /*ppDisp*/, VARIANT_BOOL* Cancel, DWORD dwFlags, BSTR bstrUrlContext, BSTR bstrUrl)
	{
		*Cancel	= VARIANT_TRUE;
		::ShellExecute(NULL, NULL, bstrUrl, 0, 0, SW_SHOWNORMAL);		
	}

	LRESULT OnForwardMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		LPMSG pMsg = (LPMSG)lParam;

		if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
			(pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
			return FALSE;

		BOOL bRet = FALSE;
		// give HTML page a chance to translate this message
		if(pMsg->hwnd == m_hWnd || IsChild(pMsg->hwnd))
			bRet = (BOOL)SendMessage(WM_FORWARDMSG, 0, (LPARAM)pMsg);

		return bRet;
	}
	void Navigate(LPCWSTR lpURL)
	{
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
			return;

		CComBSTR bsUrl = lpURL;
		spWebBrowser2->Navigate(bsUrl, NULL, NULL, NULL, NULL);
	}
	void Stop()
	{
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
			return;

		spWebBrowser2->Stop();
	}
	void Refresh()
	{
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
			return;

		spWebBrowser2->Refresh();
	}
	HWND Create(IDispatch* pExternalDispatch, LPCWSTR lpURL, HWND hWndParent, _U_RECT rect = NULL,
		DWORD dwStyle = 0, DWORD dwExStyle = 0,
		_U_MENUorID MenuOrID = 0U, LPVOID lpCreateParam = NULL, DWORD dwDocHostFlags = 
		DOCHOSTUIFLAG_DIALOG | DOCHOSTUIFLAG_DISABLE_HELP_MENU | /*DOCHOSTUIFLAG_SCROLL_NO |*/
		DOCHOSTUIFLAG_DISABLE_SCRIPT_INACTIVE | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_NO3DOUTERBORDER |
		DOCHOSTUIFLAG_THEME | DOCHOSTUIFLAG_FLAT_SCROLLBAR)
	{
		HWND hWnd = CXHtmlWnd::Create(pExternalDispatch, lpURL, hWndParent, rect, dwStyle, dwExStyle, MenuOrID, lpCreateParam, dwDocHostFlags);
		if (hWnd)
		{
			CComPtr<IWebBrowser2> spWebBrowser2;
			HRESULT hRet = QueryControl(IID_IWebBrowser2, (void**)&spWebBrowser2);
			if (FAILED(hRet))
				return hWnd;
			spWebBrowser2->put_Silent(VARIANT_TRUE);
		}
		return hWnd;
	}
	//添加UserAgent信息
	HRESULT SetUserAgent(LPCSTR lpszAppendUserAgent)
	{
		// 获取当前默认的UserAgent信息
		char szDefaultAgent[1024] = {};
		DWORD dwSize = 1024;
		HRESULT hr = ObtainUserAgentString(0, szDefaultAgent, &dwSize);

		// 追加自定义信息，修改。
		CStringA strDefaultAgent(szDefaultAgent);
		if (strDefaultAgent.Find(lpszAppendUserAgent) != -1)
			return S_OK;

		int nPos = strDefaultAgent.GetLength();
		strDefaultAgent.Delete(nPos-1);
		strDefaultAgent += "; ";
		strDefaultAgent += lpszAppendUserAgent;
		strDefaultAgent += ")";

		hr = UrlMkSetSessionOption(URLMON_OPTION_USERAGENT, strDefaultAgent.GetBuffer(), strDefaultAgent.GetLength(), 0);

		strDefaultAgent.ReleaseBuffer();

		return S_OK;
	}

	HRESULT SetCookie(LPCTSTR lpszUrl, LPCTSTR lpszCookieName, LPCTSTR lpszCookieData)
	{
		if (!::InternetSetCookie(lpszUrl, lpszCookieName, lpszCookieData))
		{
			return E_FAIL;
		}
		return S_OK;
	}

	CString GetIEVersion()
	{
		const TCHAR szFilename[] = _T("mshtml.dll");
		DWORD dwMajorVersion =0, dwMinorVersion = 0;
		DWORD dwBuildNumber =0, dwRevisionNumber = 0;
		DWORD dwHandle = 0;
		DWORD dwVerInfoSize = GetFileVersionInfoSize(szFilename, &dwHandle);
		if (dwVerInfoSize)
		{
			LPVOID lpBuffer = LocalAlloc(LPTR, dwVerInfoSize);
			if (lpBuffer)
			{
				if (GetFileVersionInfo(szFilename, dwHandle, dwVerInfoSize, lpBuffer))
				{
					VS_FIXEDFILEINFO * lpFixedFileInfo = NULL;
					UINT nFixedFileInfoSize = 0;
					if (VerQueryValue(lpBuffer, TEXT("\\"), (LPVOID*)&lpFixedFileInfo, &nFixedFileInfoSize) &&(nFixedFileInfoSize))
					{
						dwMajorVersion = HIWORD(lpFixedFileInfo->dwFileVersionMS); 
						dwMinorVersion = LOWORD(lpFixedFileInfo->dwFileVersionMS); 
						dwBuildNumber = HIWORD(lpFixedFileInfo->dwFileVersionLS); 
						dwRevisionNumber = LOWORD(lpFixedFileInfo->dwFileVersionLS);
					}
				}
				LocalFree(lpBuffer);
			}
		}
		CString strVersion;
		strVersion.Format(L"%d.%d.%d.%d", dwMajorVersion, dwMinorVersion, dwBuildNumber, dwRevisionNumber);
		return strVersion;
	}
	CString GetCurrentUrl()
	{
		CString strUrl;
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl (IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (SUCCEEDED(hRet) && spWebBrowser2)
		{
// 			CComBSTR bsUrl;
// 			spWebBrowser2->get_LocationURL(&bsUrl);

			CComPtr<IDispatch> pDoc;
			hRet = spWebBrowser2->get_Document(&pDoc);
			if (SUCCEEDED(hRet) && pDoc)
			{
				CComQIPtr<IHTMLDocument2> spDoc(pDoc);
				CComBSTR bsUrl;
				spDoc->get_URL(&bsUrl);
				strUrl = bsUrl;			
			}		
		}		
		return strUrl;
	}

	ULONG GetWebPageSize()
	{
		ULONG ulSize = 0;
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl (IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
		{
			return ulSize;
		}
		CComPtr<IDispatch> pDoc;
		hRet = spWebBrowser2->get_Document(&pDoc);

		if (SUCCEEDED(hRet) && pDoc != NULL)
		{
			CComQIPtr<IHTMLDocument2> spDoc(pDoc);
			BSTR str;
			spDoc->get_fileSize(&str);
			CString strSize(str);
			ulSize = _wtoi(strSize);
		}
		return ulSize;
	}
	void SetHTMLBody(CString html)
	{
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl (IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
		{
			return;
		}
		CComPtr<IDispatch> pDoc;
		hRet = spWebBrowser2->get_Document(&pDoc);
		if (SUCCEEDED(hRet) && pDoc != NULL)
		{
			IHTMLDocument2 * iDocu;
			hRet = pDoc->QueryInterface(IID_IHTMLDocument2, (void**)&iDocu);
			IHTMLElement * iElem;
			hRet = iDocu->get_body(&iElem);
			if (FAILED(hRet))
				return;
			BSTR str = html.AllocSysString();
			iElem->put_innerHTML(str);
		}
	}

	ATL::CString GetHTMLBody()
	{
		ATL::CString sRet;
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl (IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
		{
			return sRet;
		}
		CComPtr<IDispatch> pDoc;
		hRet = spWebBrowser2->get_Document(&pDoc);
		if (SUCCEEDED(hRet) && pDoc != NULL)
		{
			IHTMLDocument2 * iDocu;
			hRet = pDoc->QueryInterface(IID_IHTMLDocument2, (void**)&iDocu);
			IHTMLElement * iElem;
			hRet = iDocu->get_body(&iElem);
			if (FAILED(hRet))
				return sRet;
			BSTR str;
			hRet = iElem->get_innerHTML(&str);
			if (FAILED(hRet))
				return sRet;
			sRet = str;
		}
		return sRet;
	}

	ATL::CString GetCharSet()
	{
		ATL::CString sRet;
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl (IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
		{
			return sRet;
		}
		CComPtr<IDispatch> pDoc;
		hRet = spWebBrowser2->get_Document(&pDoc);
		if (SUCCEEDED(hRet) && pDoc != NULL)
		{
			IHTMLDocument2 * iDocu;
			hRet = pDoc->QueryInterface(IID_IHTMLDocument2, (void**)&iDocu);
			if (FAILED(hRet))
			{
				return sRet;
			}
			BSTR str;
			iDocu->get_charset(&str);
			sRet = str;
		}
		return sRet;
	}

	void SetCharSet(ATL::CString charset)
	{
		CComPtr<IWebBrowser2> spWebBrowser2;
		HRESULT hRet = QueryControl (IID_IWebBrowser2, (void**)&spWebBrowser2);
		if (FAILED(hRet))
		{
			return;
		}
		CComPtr<IDispatch> pDoc;
		hRet = spWebBrowser2->get_Document(&pDoc);
		if (SUCCEEDED(hRet) && pDoc != NULL)
		{
			IHTMLDocument2 * iDocu;
			hRet = pDoc->QueryInterface(IID_IHTMLDocument2, (void**)&iDocu);
			if (FAILED(hRet))
			{
				return;
			}
			BSTR str = charset.AllocSysString();
			iDocu->put_charset(str);
		}
	}
};

__declspec(selectany) _ATL_FUNC_INFO CXHtmlView::NewWindow3_Info = { CC_STDCALL, VT_EMPTY, 5, { VT_BYREF | VT_DISPATCH, VT_BYREF | VT_BOOL, VT_UINT, VT_BSTR, VT_BSTR } };
