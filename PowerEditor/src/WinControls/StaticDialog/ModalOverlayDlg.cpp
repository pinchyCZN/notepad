#include "precompiledHeaders.h"
#include "ModalOverlayDlg.h"

static std::vector<generic_string> g_pendingDropPaths;
static generic_string g_originalMessage;
static HWND g_hMessageEdit = NULL;

static const size_t MAX_DROPPED_FILES = 10;

namespace {

const TCHAR overlayClassName[] = TEXT("NppModalOverlayWnd");
const TCHAR panelClassName[] = TEXT("NppModalPanelWnd");

const int BTN_ID_YES = 1;
const int BTN_ID_NO = 2;
const int ID_MESSAGE_EDIT = 3;

const int TEXT_MARGIN = 16;
const int TEXT_TOP = 16;

const int PANEL_WIDTH = 380;
const int PANEL_HEIGHT = 240;
const int BTN_WIDTH = 75;
const int BTN_HEIGHT = 23;
const int BTN_BOTTOM_MARGIN = 24;
const int BTN_GAP = 20;

// #701e3d at 50% opacity
const COLORREF OVERLAY_COLOR = RGB(0x70, 0x1E, 0x3D);
const BYTE OVERLAY_ALPHA = 128;

struct ModalDlgState
{
	HWND hOverlay;
	HWND hPanel;
	bool done;
	int result;
};

ModalDlgState g_state = { NULL, NULL, false, 0 };

void updateDroppedFilesMessage()
{
	if (!g_hMessageEdit || !::IsWindow(g_hMessageEdit))
		return;

	generic_string msg = g_originalMessage;
	if (!g_pendingDropPaths.empty())
	{
		msg += TEXT("\r\n\r\nFiles dropped:\r\n");
		for (size_t i = 0; i < g_pendingDropPaths.size(); ++i)
		{
			msg += g_pendingDropPaths[i];
			msg += TEXT("\r\n");
		}
	}

	::SetWindowText(g_hMessageEdit, msg.c_str());
}

void storeDroppedFiles(HDROP hdrop)
{
	if (!hdrop)
		return;

	const UINT fileCount = ::DragQueryFile(hdrop, 0xFFFFFFFF, NULL, 0);
	for (UINT i = 0; i < fileCount && g_pendingDropPaths.size() < MAX_DROPPED_FILES; ++i)
	{
		TCHAR path[MAX_PATH] = {};
		if (::DragQueryFile(hdrop, i, path, MAX_PATH) > 0)
			g_pendingDropPaths.push_back(path);
	}

	::DragFinish(hdrop);
	updateDroppedFilesMessage();
}

HBRUSH g_hOverlayBrush = NULL;
bool g_classesRegistered = false;

void raisePanelAboveOverlay()
{
	if (g_state.hPanel && ::IsWindow(g_state.hPanel) &&
		g_state.hOverlay && ::IsWindow(g_state.hOverlay))
	{
		::SetWindowPos(g_state.hPanel, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
	}
}

void endModal(int result)
{
	g_state.result = result;
	g_state.done = true;
	if (g_state.hPanel && ::IsWindow(g_state.hPanel))
		::DestroyWindow(g_state.hPanel);
	if (g_state.hOverlay && ::IsWindow(g_state.hOverlay))
		::DestroyWindow(g_state.hOverlay);
}

void registerWindowClasses(HINSTANCE hInst)
{
	if (g_classesRegistered)
		return;

	g_hOverlayBrush = ::CreateSolidBrush(OVERLAY_COLOR);

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
		case WM_MOUSEACTIVATE:
			raisePanelAboveOverlay();
			return MA_NOACTIVATE;

		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			raisePanelAboveOverlay();
			break;

		case WM_ACTIVATE:
			raisePanelAboveOverlay();
			break;

		case WM_ERASEBKGND:
			if (g_hOverlayBrush)
			{
				RECT rc = {};
				::GetClientRect(hwnd, &rc);
				::FillRect((HDC)wParam, &rc, g_hOverlayBrush);
				return 1;
			}
			break;

		case WM_DROPFILES:
			storeDroppedFiles((HDROP)wParam);
			raisePanelAboveOverlay();
			return 0;

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
			return 0;
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

LRESULT CALLBACK panelProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_ERASEBKGND:
		{
			RECT rc = {};
			::GetClientRect(hwnd, &rc);
			::FillRect((HDC)wParam, &rc, ::GetSysColorBrush(COLOR_BTNFACE));
			return 1;
		}

		case WM_DROPFILES:
			storeDroppedFiles((HDROP)wParam);
			return 0;

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

		case WM_CLOSE:
			endModal(0);
			return 0;

		case WM_DESTROY:
			g_state.hPanel = NULL;
			g_hMessageEdit = NULL;
			return 0;
	}

	return ::DefWindowProc(hwnd, message, wParam, lParam);
}

} // namespace

void modalOverlayDlg_clearPendingDrops()
{
	g_pendingDropPaths.clear();
}

static void clearOriginalMessageState()
{
	g_originalMessage.clear();
	g_hMessageEdit = NULL;
}

size_t modalOverlayDlg_getPendingDropCount()
{
	return g_pendingDropPaths.size();
}

const TCHAR* modalOverlayDlg_getPendingDropPath(size_t index)
{
	if (index >= g_pendingDropPaths.size())
		return NULL;
	return g_pendingDropPaths[index].c_str();
}

int modalDlg(HWND hParent, int type, const TCHAR* pszContext)
{
	if (!hParent || !::IsWindow(hParent))
		return 0;

	modalOverlayDlg_clearPendingDrops();
	clearOriginalMessageState();

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

	// Layered owned popup: semi-transparent tint only (no child windows).
	// WS_EX_NOACTIVATE and no WS_EX_TOPMOST so clicks cannot raise it above the panel.
	HWND hOverlay = ::CreateWindowEx(
		WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_ACCEPTFILES,
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
	::SetLayeredWindowAttributes(hOverlay, 0, OVERLAY_ALPHA, LWA_ALPHA);

	const int panelX = (clientW - PANEL_WIDTH) / 2;
	const int panelY = (clientH - PANEL_HEIGHT) / 2;

	// Separate opaque owned popup (not a child of the layered overlay).
	HWND hPanel = ::CreateWindowEx(
		WS_EX_CLIENTEDGE | WS_EX_TOPMOST | WS_EX_ACCEPTFILES,
		panelClassName,
		TEXT(""),
		WS_POPUP | WS_VISIBLE | WS_TABSTOP,
		ptOrigin.x + panelX,
		ptOrigin.y + panelY,
		PANEL_WIDTH,
		PANEL_HEIGHT,
		hParent,
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
	raisePanelAboveOverlay();
	::SetForegroundWindow(hPanel);

	const int btnY = PANEL_HEIGHT - BTN_HEIGHT - BTN_BOTTOM_MARGIN;
	const int yesX = (PANEL_WIDTH / 2) - BTN_WIDTH - (BTN_GAP / 2);
	const int noX = (PANEL_WIDTH / 2) + (BTN_GAP / 2);

	if (type == MODAL_OVERLAY_DLG_FILE_DELETED && pszContext && pszContext[0])
	{
		TCHAR phrase[512] = {};
		::wsprintf(phrase, TEXT("The file \"%s\" doesn't exist anymore.\r\nKeep this file in editor?"), pszContext);
		g_originalMessage = phrase;

		g_hMessageEdit = ::CreateWindowEx(
			WS_EX_CLIENTEDGE,
			TEXT("EDIT"),
			g_originalMessage.c_str(),
			WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_TABSTOP | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL | ES_LEFT,
			TEXT_MARGIN,
			TEXT_TOP,
			PANEL_WIDTH - (TEXT_MARGIN * 2),
			btnY - TEXT_TOP - 8,
			hPanel,
			(HMENU)ID_MESSAGE_EDIT,
			hInst,
			NULL);
	}

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

	if (g_state.hPanel && ::IsWindow(g_state.hPanel))
	{
		::DestroyWindow(g_state.hPanel);
		g_state.hPanel = NULL;
	}

	if (g_state.hOverlay && ::IsWindow(g_state.hOverlay))
	{
		::DestroyWindow(g_state.hOverlay);
		g_state.hOverlay = NULL;
	}

	::EnableWindow(hParent, TRUE);
	::SetForegroundWindow(hParent);
	::SetFocus(hParent);

	return g_state.result;
}
