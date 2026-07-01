// This file is part of Notepad++ project
// Copyright (C)2003 Don HO <don.h@free.fr>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include "precompiledHeaders.h"
#include "ModalOverlayDlg.h"

namespace {

const TCHAR overlayClassName[] = TEXT("NppModalOverlayWnd");
const TCHAR panelClassName[] = TEXT("NppModalPanelWnd");

const int BTN_ID_YES = 1;
const int BTN_ID_NO = 2;

const int PANEL_WIDTH = 380;
const int PANEL_HEIGHT = 240;
const int BTN_WIDTH = 75;
const int BTN_HEIGHT = 23;
const int BTN_BOTTOM_MARGIN = 24;
const int BTN_GAP = 20;

struct ModalDlgState
{
	HWND hOverlay;
	HWND hPanel;
	bool done;
	int result;
};

ModalDlgState g_state = { NULL, NULL, false, 0 };

HBRUSH g_hOverlayBrush = NULL;
bool g_classesRegistered = false;

void endModal(int result)
{
	g_state.result = result;
	g_state.done = true;
	if (g_state.hOverlay)
		::DestroyWindow(g_state.hOverlay);
}

void registerWindowClasses(HINSTANCE hInst)
{
	if (g_classesRegistered)
		return;

	g_hOverlayBrush = ::CreateSolidBrush(RGB(255, 210, 210));

	WNDCLASS overlayClass = {};
	overlayClass.style = CS_HREDRAW | CS_VREDRAW;
	overlayClass.lpfnWndProc = ::DefWindowProc;
	overlayClass.hInstance = hInst;
	overlayClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	overlayClass.hbrBackground = g_hOverlayBrush;
	overlayClass.lpszClassName = overlayClassName;
	::RegisterClass(&overlayClass);

	WNDCLASS panelClass = {};
	panelClass.style = CS_HREDRAW | CS_VREDRAW;
	panelClass.lpfnWndProc = ::DefWindowProc;
	panelClass.hInstance = hInst;
	panelClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
	panelClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	panelClass.lpszClassName = panelClassName;
	::RegisterClass(&panelClass);

	g_classesRegistered = true;
}

LRESULT CALLBACK overlayProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				endModal(0);
				return 0;
			}
			break;

		case WM_CLOSE:
			endModal(0);
			return 0;

		case WM_DESTROY:
			g_state.hOverlay = NULL;
			g_state.hPanel = NULL;
			return 0;
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK panelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_KEYDOWN:
			if (wParam == VK_ESCAPE)
			{
				endModal(0);
				return 0;
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case BTN_ID_YES:
					endModal(1);
					return 0;

				case BTN_ID_NO:
					endModal(0);
					return 0;
			}
			break;
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

} // namespace

int modalDlg(HWND hParent, int type)
{
	(void)type;

	if (!hParent || !::IsWindow(hParent))
		return 0;

	HINSTANCE hInst = (HINSTANCE)::GetWindowLongPtr(hParent, GWLP_HINSTANCE);
	if (!hInst)
		hInst = ::GetModuleHandle(NULL);

	registerWindowClasses(hInst);

	g_state.hOverlay = NULL;
	g_state.hPanel = NULL;
	g_state.done = false;
	g_state.result = 0;

	RECT rcClient = {};
	::GetClientRect(hParent, &rcClient);
	const int clientW = rcClient.right - rcClient.left;
	const int clientH = rcClient.bottom - rcClient.top;

	POINT ptOrigin = { rcClient.left, rcClient.top };
	::ClientToScreen(hParent, &ptOrigin);

	// Owned popup (not a child) so it stays interactive while the parent is disabled.
	HWND hOverlay = ::CreateWindowEx(
		WS_EX_TOPMOST,
		overlayClassName,
		TEXT(""),
		WS_POPUP | WS_VISIBLE,
		ptOrigin.x,
		ptOrigin.y,
		clientW,
		clientH,
		hParent,
		NULL,
		hInst,
		NULL);

	if (!hOverlay)
		return 0;

	g_state.hOverlay = hOverlay;
	::SetWindowLongPtr(hOverlay, GWLP_WNDPROC, (LONG_PTR)overlayProc);

	const int panelX = (clientW - PANEL_WIDTH) / 2;
	const int panelY = (clientH - PANEL_HEIGHT) / 2;

	HWND hPanel = ::CreateWindowEx(
		WS_EX_CLIENTEDGE,
		panelClassName,
		TEXT(""),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP,
		panelX,
		panelY,
		PANEL_WIDTH,
		PANEL_HEIGHT,
		hOverlay,
		NULL,
		hInst,
		NULL);

	if (!hPanel)
	{
		::DestroyWindow(hOverlay);
		g_state.hOverlay = NULL;
		return 0;
	}

	g_state.hPanel = hPanel;
	::SetWindowLongPtr(hPanel, GWLP_WNDPROC, (LONG_PTR)panelProc);

	const int btnY = PANEL_HEIGHT - BTN_HEIGHT - BTN_BOTTOM_MARGIN;
	const int yesX = (PANEL_WIDTH / 2) - BTN_WIDTH - (BTN_GAP / 2);
	const int noX = (PANEL_WIDTH / 2) + (BTN_GAP / 2);

	HWND hYes = ::CreateWindow(
		TEXT("BUTTON"),
		TEXT("Yes"),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		yesX,
		btnY,
		BTN_WIDTH,
		BTN_HEIGHT,
		hPanel,
		(HMENU)BTN_ID_YES,
		hInst,
		NULL);

	HWND hNo = ::CreateWindow(
		TEXT("BUTTON"),
		TEXT("No"),
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
		noX,
		btnY,
		BTN_WIDTH,
		BTN_HEIGHT,
		hPanel,
		(HMENU)BTN_ID_NO,
		hInst,
		NULL);

	if (hYes)
		::SetFocus(hYes);
	else if (hNo)
		::SetFocus(hNo);
	else
		::SetFocus(hPanel);

	::EnableWindow(hParent, FALSE);

	MSG msg = {};
	while (!g_state.done)
	{
		if (::GetMessage(&msg, NULL, 0, 0) <= 0)
		{
			::PostQuitMessage((int)msg.wParam);
			break;
		}

		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
		{
			endModal(0);
			continue;
		}

		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}

	if (g_state.hOverlay && ::IsWindow(g_state.hOverlay))
	{
		::DestroyWindow(g_state.hOverlay);
		g_state.hOverlay = NULL;
		g_state.hPanel = NULL;
	}

	::EnableWindow(hParent, TRUE);
	::SetForegroundWindow(hParent);
	::SetFocus(hParent);

	return g_state.result;
}
