#include "stdafx.h"
#include "TrayIcon.h"

#include <windows.h>
#include <shellapi.h>

#include "Frontend.h"
#include "FrontendDlg.h"
#include "Resource.h"

#define TRAYICONID 987435

HWND s_hWnd;
TrayIcon* TrayIcon::instance = NULL;

LRESULT CALLBACK TrayIconProc( HWND, UINT, WPARAM, LPARAM lParam )
{
	switch (lParam)
	{
	case WM_LBUTTONDOWN:
	case WM_RBUTTONDOWN:
		TrayIcon::instance->displayPopup();
		break;
	}
	
	return 1;
}

void TrayIcon::displayPopup()
{
	HMENU hMenu = CreatePopupMenu();
	hMenu = LoadMenu(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_TRAYMENU));
	
	POINT pt;
	
	GetCursorPos(&pt);
	
	SetForegroundWindow( s_hWnd );
	
	int cmd = TrackPopupMenu(GetSubMenu(hMenu, 0),
		TPM_RIGHTBUTTON | TPM_RETURNCMD, 
		pt.x, 
		pt.y, 
		0, 
		s_hWnd, 
		NULL);
	
	PostMessage(s_hWnd, WM_NULL, 0, 0); 
	
	switch(cmd)
	{
	case IDM_EXIT:
		main_dlg.OnExitButton();
		break;
		
	case IDM_WINDOW:
		main_dlg.OpenIcon();
		break;

	case IDM_SETTINGS:
		main_dlg.m_TabCtrl.SetCurSel(0);
		main_dlg.m_TabCtrl.DisplayTabDialog();
		main_dlg.OpenIcon();
		break;
		
	case IDM_OUTPUT:
		main_dlg.m_TabCtrl.SetCurSel(1);
		main_dlg.m_TabCtrl.DisplayTabDialog();
		main_dlg.OpenIcon();
		break;
		
	case IDM_STOP:
		main_dlg.OnStopButton();
		break;
		
	case IDM_START:
		main_dlg.OnStartButton();
		break;
	}
	
	DestroyMenu(hMenu);
}

void TrayIcon::setStatus( status con_stat, const char* message )
{
	switch( con_stat )
	{
	case CONNECTED:
		setTrayAppearance( false, IDI_ICON2, message );
		break;
		
	case OFF:
		setTrayAppearance( false, IDI_ICON3, message );
		break;
		
	case ON:
		setTrayAppearance( false, IDI_ICON1, message );
		break;
	}
}

void TrayIcon::setTrayAppearance( bool start, unsigned int res_id, const char* message )
{
	NOTIFYICONDATA notifyIconData;
	
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.hWnd = s_hWnd;
	
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = 123456;
	notifyIconData.uID = TRAYICONID; 
	notifyIconData.hIcon = (HICON)LoadIcon( hInst, MAKEINTRESOURCE( res_id ) );  
	strcpy( notifyIconData.szTip, message ); 
	
	if( start )
		Shell_NotifyIcon( NIM_ADD, &notifyIconData );
	else
		Shell_NotifyIcon( NIM_MODIFY, &notifyIconData );
}

TrayIcon::TrayIcon( CFrontendDlg& dlg, HINSTANCE hInst ) : main_dlg( dlg ), hInst( hInst )
{
	instance = this;
	WNDCLASS wndClass;
	
	wndClass.style = 0; 
	wndClass.lpfnWndProc = TrayIconProc;
	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hInstance = hInst;
	wndClass.hIcon = NULL;
	wndClass.hCursor = NULL;
	wndClass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndClass.lpszMenuName = NULL;
	wndClass.lpszClassName = "OLSRdTrayIcon";
	
	if (0 != RegisterClass(&wndClass))
	{
		s_hWnd = CreateWindow( 
			"OLSRdTrayIcon", 
			"Invisible Message Window", 
			WS_OVERLAPPEDWINDOW,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			NULL,
			NULL,
			hInst,
			NULL);
		
		if (s_hWnd)
		{
			ShowWindow(s_hWnd, SW_HIDE);
			
			UpdateWindow(s_hWnd);
		}
	}
	
	setTrayAppearance( true, IDI_ICON3, "Starting..." );
}

TrayIcon::~TrayIcon()
{
	NOTIFYICONDATA notifyIconData;
	
	notifyIconData.cbSize = sizeof(notifyIconData);
	notifyIconData.hWnd = s_hWnd;
	
	notifyIconData.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	notifyIconData.uCallbackMessage = 123456;
	notifyIconData.uID = TRAYICONID; 
	notifyIconData.hIcon = NULL;
	
	Shell_NotifyIcon( NIM_DELETE, &notifyIconData );
}
