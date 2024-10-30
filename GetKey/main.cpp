#include <iostream>
#include <Windows.h>
#include <windows.h>
#include <stdio.h>

#include <cstdio>
#include <tlhelp32.h>
#include <tchar.h>

//////////////////////////////////////
#include <time.h>

#include <Shellapi.h>
#include "resource.h"

#include <vector>
#include <string>
#include <cctype> // ����������� ��� isprint
#include <ctype.h> // ��� C
#include <fstream> // ��������� ���� ������������ ����
#include <set>
#include <fstream>
//#include <winuser.h>




using namespace std;
//////////////////////////////////////

#define MYWM_NOTIFYICON (WM_USER + 1)
// ��������� ���������, ������� ����� ���������� ��� �������������� ����� � ����� ������� � �������

HHOOK hook;
HHOOK mousehook;
//
bool ClampedGap = false;
bool Shift = false;
bool LControl = false;
bool RControl = false;
bool FirstOccurrence = true;
ULONG Delay = 0;
bool CaptureEnabled = true;
HKL hkl = GetKeyboardLayout(0);
HINSTANCE hInst;
HWND hWnd;
HWND hWndSuggestions = nullptr; // ���������� ���������� ��� ���� � �����������
POINT point;
wstring matchedSuggestions;
static HWND hWndEdit = NULL;

const int ArrayZize = 300;
int MatchingKeys[ArrayZize];
//

//
bool LBUTTONDOWN = false;
bool RBUTTONDOWN = false;
bool MBUTTONDOWN = false;
bool lRetFalse = false;
bool rRetFalse = false;
bool mRetFalse = false;
//
// ���������� ���������� ��� ������������ �����
wstring input;

/*  Declare Windows procedure  */
LRESULT CALLBACK WndProcA(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SuggestionsWndProc(HWND, UINT, WPARAM, LPARAM);

vector<wstring> suggestions;
set<wstring> uniqueSuggestions; // ��� �������� ���������� �����������


int selectedSuggestion = -1; // ������ �������� ���������� �����������

void UpdateSuggestions();
//void InsertSuggestionInInput(HWND hWndEdit, const string& text);
void InitializeSuggestionWindow(HINSTANCE hInstance);

	
/*
void OnKeyDown(WPARAM wParam, HWND hWndEdit) {
	switch (wParam) {
	case VK_DOWN:
		selectedSuggestion = (selectedSuggestion + 1) % suggestions.size(); // ������� ����
		UpdateSuggestions();
		break;
	case VK_UP:
		selectedSuggestion = (selectedSuggestion - 1 + suggestions.size()) % suggestions.size(); // ������� �����
		UpdateSuggestions();
		break;
	}

}
*/


void Log(const string& message) {
	cout << message << endl;
}



void SaveSuggestions(const vector<wstring>& suggestions, const wstring& filename) {
	wofstream file(filename);  // ���������� wofstream ��� Unicode
	file.imbue(locale(""));    // ������������� ������ ��� ���������� ������ � Unicode

	if (!file.is_open()) {
		wcerr << L"�� ������� ������� ���� ��� ������: " << filename << endl;
		return;
	}

	for (const auto& suggestion : suggestions) {
		file << suggestion << L"\n"; // ���������� ������ ������ � ����
	}

	file.close(); // ��������� ���� ����� ������
	wcout << L"���� ������� �������: " << filename << endl; // ��������� �� �������� ������
}

vector<wstring> LoadSuggestions(const wstring& filename) {
	vector<wstring> suggestions;
	wifstream file(filename);
	if (!file.is_open()) {
		wcerr << L"�� ������� ������� ���� ��� ������: " << filename << endl;
		return suggestions;
	}

	wstring line;
	while (getline(file, line)) {
		suggestions.push_back(line); // ��������� ������ ������ �� ����� � ������
	}

	file.close(); // ��������� ���� ����� ������
	return suggestions;
}

POINT getCaretPosition(HWND hwnd) {
	POINT point;
	if (GetCaretPos(&point)) {
		// ����������� ���������� � ��������
		ClientToScreen(hwnd, &point);
		return point;
	}
	else {
		point.x = -1;
		point.y = -1;
		return point;
	}
}

// ������� ��������� ������� ������� � ������ ��������� � �������� ����������
POINT GetAdjustedCaretPosition(HWND hwndEdit) {
	POINT caretPos;
 	if (GetCaretPos(&caretPos)) {
		// ������������� ���������� ������� � ��������
		ClientToScreen(hwndEdit, &caretPos);
	}
	else {
		caretPos.x = -1;
		caretPos.y = -1;
	}
	return caretPos;
}

void SendString(const wstring& text) {
	INPUT inputs[2]; // ��� ������� ������� ����� ��� INPUT (������� � ���������� �������)
	ZeroMemory(inputs, sizeof(inputs));

	for (wchar_t ch : text) {
		// ��������� ������� ������� (KEYDOWN)
		if (input.find(ch) == 0 && !input.empty()) {
			continue;
		}
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = 0; // �� ������������ ��� �������� Unicode
		inputs[0].ki.wScan = ch;
		inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
		//Sleep(100);
		// ��������� ���������� ������� (KEYUP)
		inputs[1] = inputs[0];
		inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

		// ���������� ������� � ����������
		SendInput(2, inputs, sizeof(INPUT));
		Sleep(100);
	}
}

wstring stringToWstring(const string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
	return wstr;
}

// ������� ��� ����������� ���������� �����
void InsertSelectedSuggestion(const wstring& word) {
	//HWND hWndEdit = GetForegroundWindow(); // ����, ��� ���� ����� ������

		// ����������� ������ � ���� �����
		//SendMessage(hWndEdit, EM_REPLACESEL, TRUE, (LPARAM)word.c_str());
	
	SetForegroundWindow(hWndEdit);
	Sleep(100); // ������� ���� ��� ��������� ����
	
	//wstring wtext = stringToWstring(word);
	SendString(word);
	input.clear();

	SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);

	SetForegroundWindow(hWndEdit);
	Sleep(100); // ������� ���� ��� ��������� ����

}



void UpdateSuggestionsWindow(HWND hwndSuggestions, const wstring& text) {
	HDC hdc = GetDC(hwndSuggestions);
	RECT rect = { 0, 0, 0, 0 };

	// �������� ������ ������
	DrawTextW(hdc, text.c_str(), -1, &rect, DT_CALCRECT);

	// ����� ����� � ���� ���������
	SetWindowTextW(hwndSuggestions, text.c_str());

	// �������� ������ ���� � ������������ � �������� ������
	SetWindowPos(hwndSuggestions, HWND_TOP, 0, 0, rect.right - rect.left + 10, rect.bottom - rect.top + 10, SWP_NOZORDER | SWP_NOMOVE);
	// ���������� ���� 
	InvalidateRect(hWndSuggestions, NULL, TRUE);
	UpdateWindow(hWndSuggestions);

	ReleaseDC(hwndSuggestions, hdc);

	
}





HWND GetEditWindow() {
	// �������� �������� ����
	HWND hwnd = GetForegroundWindow();
	if (hwnd != NULL) {
		// ���� �������� ���� ������ "Edit" � �������� ����
		HWND hwndEdit = FindWindowEx(hwnd, NULL, "Edit", NULL);
		if (hwndEdit == NULL) {
			cerr << "Edit control not found in active window" << endl;
		}
		return hwndEdit;
	}
	else {
		cerr << "No active window found" << endl;
	}
	return NULL;

}

POINT GetCaretPosition() {
	POINT point = { -1, -1 };
	GUITHREADINFO guiInfo;
	ZeroMemory(&guiInfo, sizeof(GUITHREADINFO));
	guiInfo.cbSize = sizeof(GUITHREADINFO);

	if (GetGUIThreadInfo(0, &guiInfo)) {
		if (guiInfo.hwndCaret != NULL) {
			if (GetCaretPos(&point)) {
				ClientToScreen(guiInfo.hwndCaret, &point);
				return point;
			}
		}
	}
	return point;
}



POINT GetCaretPosFromFocusedWindow() {
	POINT caretPos = { -1, -1 };

	// �������� ���������� ��������� ����
	HWND hwnd = GetForegroundWindow();
	if (hwnd == NULL) {
		cerr << "No active window found" << endl;
		return caretPos;
	}

	GUITHREADINFO guiThreadInfo = { 0 };
	guiThreadInfo.cbSize = sizeof(GUITHREADINFO);

	// �������� ���������� � ������ GUI, ������� �������
	if (GetGUIThreadInfo(0, &guiThreadInfo)) {
		if (guiThreadInfo.hwndCaret != NULL) {
			// �������� ������� �������
			if (GetCaretPos(&caretPos)) {
				// ����������� ���������� � ��������
				ClientToScreen(guiThreadInfo.hwndCaret, &caretPos);
				return caretPos;
			}
		}
	}
	return caretPos;
}



// ������� ��� ���������� ���� � �����������
void UpdateSuggestions() {
	
	// ������: ���������� ����������� �� ���������� ������
	uniqueSuggestions.clear();
	for (const auto& suggestion : suggestions) {
		if (suggestion.find(input) == 0 && !input.empty()) {
			uniqueSuggestions.insert(suggestion); // ��������� ������ ���������� �����������
		}
	}
	matchedSuggestions.clear();
	// �������� ���������� ����������� � ������
	for (const auto& suggestion : uniqueSuggestions) {
		matchedSuggestions += suggestion + L"\r\n"; // ��������� ������
	}
	
	
	UpdateSuggestionsWindow(hWndSuggestions, matchedSuggestions);
	
	if (matchedSuggestions.empty()) {
		SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
		input.clear();
	}else {
		SetWindowPos(hWndSuggestions, HWND_TOP, point.x, point.y + 20, 150, 100, SWP_SHOWWINDOW);
	}
	
}

void InitializeSuggestionWindow(HINSTANCE hInstance) {
	
	// �������� ���� ���������
	hWndSuggestions = CreateWindowEx(
		WS_EX_TOPMOST,
		"SuggestionsWndClass",
		"",
		WS_POPUP | WS_BORDER | SS_LEFT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		150, 100,
		hWnd,
		NULL,
		hInstance,
		NULL
	);

	// ������������� ����� ��� �������� ������
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hWndSuggestions, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void TrayMessage_(HWND hWnd, DWORD dwMessage, UINT uID, HICON hIcon, const char* message) {
	NOTIFYICONDATA nid;
	nid.cbSize = sizeof(NOTIFYICONDATA);
	nid.hWnd = hWnd;
	nid.uID = uID;
	nid.uFlags = NIF_INFO;
	nid.dwInfoFlags = NIIF_INFO;

	// ���������� ���������� ������
	strcpy_s(nid.szInfo, sizeof(nid.szInfo), message); // �������� ���������
	strcpy_s(nid.szInfoTitle, sizeof(nid.szInfoTitle), "������������� �����"); // �������� ���������

	Shell_NotifyIcon(dwMessage, &nid);
}

BOOL TrayMessage(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, PSTR pszTip)
// systray icon 
{
	BOOL res;

	NOTIFYICONDATA tnd;

	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hDlg;
	tnd.uID = uID;

	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage = MYWM_NOTIFYICON; // ���������, ������� �������� ��� ������ ��� ������ �� ������... 
	tnd.hIcon = hIcon;

	if (pszTip)
	{
		// ��������� ������������ ��������
		LPSTR result = lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
		// ��������� ���������� (���� ����������)
		if (result == tnd.szTip) {
			// ����������� ������ �������
		}
		else {
			// ��������� ������, ���� ������ ���� ��������
		}
	}
	else
	{
		tnd.szTip[0] = '\0';
	}


	res = Shell_NotifyIcon(dwMessage, &tnd);


	return res;
}


bool IsRussianKeyboardLayout() {
	// �������� ���� � ������
	HWND hWnd = GetForegroundWindow();
	if (hWnd == NULL) return false;

	// �������� ������������� ������ ��� ���� � ������
	DWORD threadId = GetWindowThreadProcessId(hWnd, NULL);

	// �������� ��������� ���������� ��� ����� ������
	HKL hkl = GetKeyboardLayout(threadId);

	// ���������, ��� ��������� ������������� ������� (0x0419)
	return (LOWORD(hkl) == 0x0419);
}

/*
wstring ConvertToWString(const string& str) {
	wstring wstr(str.size(), L'\0');
	mbstowcs(&wstr[0], str.c_str(), str.size());
	return wstr;
}
*/


LRESULT CALLBACK keyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
	DWORD newVkCode;
	INPUT inputs[1];
	UINT ret;

	char wParamStr[16];
	char vkStr[16] = "";

	if (wParam == WM_KEYDOWN)
		strcpy_s(wParamStr, "KEYDOWN");
	else if (wParam == WM_KEYUP)
		strcpy_s(wParamStr, "KEYUP");
	else if (wParam == WM_SYSKEYDOWN)
		strcpy_s(wParamStr, "SYSKEYDOWN");
	else if (wParam == WM_SYSKEYUP)
		strcpy_s(wParamStr, "SYSKEYUP");
	else
		strcpy_s(wParamStr, "UNKNOWN");

	if (p->vkCode == 10)
		strcpy_s(vkStr, "<LF>");
	else if (p->vkCode == 13)
		strcpy_s(vkStr, "<CR>");
	else
		vkStr[0] = p->vkCode;


	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wScan = 0;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;

	if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
		inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
	}

	/*if (p->vkCode == VK_F1 && (p->flags & LLKHF_INJECTED) == 0) {
		inputs[0].ki.wVk = VK_NUMPAD1;
		ret = SendInput(1, inputs, sizeof(INPUT));
		return 1;
		}*/

	printf("%d - %s - %lu (%s) - %d -\t %lu\n",
		nCode, wParamStr, p->vkCode, vkStr, p->scanCode, p->time);

	//////////////////////////////////////////////////////////////
	////keybd_event('C', 0, 0, 0);
	////keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
	//////////////////////////////////////////////////////////////

	//*
	if (p->vkCode == 109 && wParam == WM_KEYDOWN)
	{
		CaptureEnabled = !CaptureEnabled;
		if (CaptureEnabled)
			//	MessageBox(0, "Çàõâàò âêëþ÷åí", "Àêòèâàöèÿ Half keyboard", MB_ICONINFORMATION);		
			TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), "Half keyboard\n(Çàõâàò âêëþ÷åí)");
		else
			//	MessageBox(0, "Çàõâàò âûêëþ÷åí", "Äåêòèâàöèÿ Half keyboard", MB_ICONSTOP);
			TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), "Half keyboard\n(Çàõâàò âûêëþ÷åí)");

		return 1;
	}

	//RWin
	if (p->vkCode == 92 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
	{
		PostMessage(GetForegroundWindow(), WM_INPUTLANGCHANGEREQUEST, 2, 0);
		return 1;
	}
	if (!CaptureEnabled)
	{
		return 0;
	}
	else
	{

		//Space
		if (!RControl)
		{
			if (p->vkCode == VK_SPACE && wParam == WM_KEYDOWN)
			{
				if (Delay == 0)
				{
					ClampedGap = true;
					Delay = p->time;
				}
				return 1;
			}
			else if ((p->vkCode == VK_SPACE) && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
			{
				ClampedGap = false;
				Delay = p->time - Delay;

				cout << Delay << endl;
				if (Delay < 300)
				{
					inputs[0].ki.wVk = 0;
					inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
					inputs[0].ki.wScan = 0x0020; //" " SPACE
					ret = SendInput(1, inputs, sizeof(INPUT));
				}
				Delay = 0;
				return 1;
			}
		}

		//Shift
		if ((p->vkCode == 161 || p->vkCode == 160) && wParam == WM_KEYDOWN)
			Shift = !Shift;

		//Left Control
		if (p->vkCode == 162 && wParam == WM_KEYDOWN)
			LControl = !LControl;

		//Right Control
		if (p->vkCode == 163 && wParam == WM_KEYDOWN)
			RControl = true;
		else if ((p->vkCode == 163) && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
			RControl = false;


		//
		/*if (p->vkCode == 162 && p->scanCode == 541)
			return 1;*/
			///////////////////////////
			/*else if ((p->vkCode == 162) && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
				Control = false;*/
				///////////////////////////

				//Caps Lock right hand
		if (p->vkCode == 222 && (p->flags & LLKHF_INJECTED) == 0 && ClampedGap) {

			ClampedGap = false;
			inputs[0].ki.wVk = VK_CAPITAL;
			inputs[0].ki.dwFlags = 0;
			ret = SendInput(1, inputs, sizeof(INPUT));
			inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
			ret = SendInput(1, inputs, sizeof(INPUT));
			return 1;
		}

		//Enter - Caps Lock left hand
		if ((p->vkCode == 20 && (p->flags & LLKHF_INJECTED) == 0) && (wParam == WM_KEYDOWN) && !ClampedGap) {
			if (Shift)
			{
				inputs[0].ki.wVk = VK_CAPITAL;
				inputs[0].ki.dwFlags = 0;
				ret = SendInput(1, inputs, sizeof(INPUT));
				inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
				ret = SendInput(1, inputs, sizeof(INPUT));
				Shift = false;
				return 1;
			}
			else if (wParam == WM_KEYDOWN)
			{
				inputs[0].ki.wVk = VK_RETURN;
				inputs[0].ki.dwFlags = 0;
				ret = SendInput(1, inputs, sizeof(INPUT));
				inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
				ret = SendInput(1, inputs, sizeof(INPUT));
				return 1;
			}
		}

		//Exit
		if (p->vkCode == 27 && (p->flags & LLKHF_INJECTED) == 0 && (wParam == WM_KEYDOWN) && ClampedGap) {

			UnhookWindowsHookEx;
			UnhookWindowsHookEx;
			MessageBox(0, "Çàêîí÷èëè", "Âñåãî õîðîøåãî!!!", 65536);//MsgBoxSetForeground
			TrayMessage(hWnd, NIM_DELETE, 0, 0, 0);
			exit(0);

			return 1;
		}

		//<
		if ((p->vkCode == 219 && (p->flags & LLKHF_INJECTED) == 0) && (wParam == WM_KEYDOWN) && ClampedGap) {
			inputs[0].ki.wVk = 0;
			inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
			inputs[0].ki.wScan = 0x003C; //<
			ret = SendInput(1, inputs, sizeof(INPUT));

			return 1;
		}

		//>
		if ((p->vkCode == 221 && (p->flags & LLKHF_INJECTED) == 0) && (wParam == WM_KEYDOWN) && ClampedGap) {
			inputs[0].ki.wVk = 0;
			inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
			inputs[0].ki.wScan = 0x003E; //>
			ret = SendInput(1, inputs, sizeof(INPUT));

			return 1;
		}

		//Other characters
		if (MatchingKeys[p->vkCode] > 0)
		{
			if ((p->flags & LLKHF_INJECTED) == 0 && ClampedGap) {
				if (Shift)
				{
					Shift = false;
					inputs[0].ki.wVk = 161;
					inputs[0].ki.dwFlags = 0;
					ret = SendInput(1, inputs, sizeof(INPUT));
				}
				//ï
				inputs[0].ki.wVk = MatchingKeys[p->vkCode];
				ret = SendInput(1, inputs, sizeof(INPUT));

				Delay = 300;

				////////////////////////////////////////////////////////
				//keybd_event('C', 0, 0, 0);
				//keybd_event('C', 0, KEYEVENTF_KEYUP, 0);
				////////////////////////////////////////////////////////
				//
				if (Shift)
				{
					inputs[0].ki.wVk = 161;
					inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
					ret = SendInput(1, inputs, sizeof(INPUT));
					Shift = false;
				}
				return 1;
			}
			else if ((p->flags & LLKHF_INJECTED) == 0) {
				if (Shift)
				{
					Shift = false;
					inputs[0].ki.wVk = 161;
					inputs[0].ki.dwFlags = 0;
					ret = SendInput(1, inputs, sizeof(INPUT));
				}
				//õ,ú-tab,¸
				if (LControl)
				{
					inputs[0].ki.wVk = 162;
					inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
					ret = SendInput(1, inputs, sizeof(INPUT));

					inputs[0].ki.dwFlags = 0;
					if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
						inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
					}

					if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)
					{
						LControl = false;
					}
					if (p->vkCode == 9)
					{
						inputs[0].ki.wVk = 219;
						ret = SendInput(1, inputs, sizeof(INPUT));
					}
					if (p->vkCode == 192)
					{
						inputs[0].ki.wVk = 221;
						ret = SendInput(1, inputs, sizeof(INPUT));
					}
					if (p->vkCode == 49)
					{
						inputs[0].ki.wVk = 187;
						ret = SendInput(1, inputs, sizeof(INPUT));
					}
					if (p->vkCode == 50)
					{
						inputs[0].ki.wVk = 220;
						ret = SendInput(1, inputs, sizeof(INPUT));
					}
					if (!(p->vkCode == 9 || p->vkCode == 192 || p->vkCode == 49 || p->vkCode == 50))
					{
						inputs[0].ki.wVk = p->vkCode;
						ret = SendInput(1, inputs, sizeof(INPUT));
					}
				}
				else
				{
					inputs[0].ki.wVk = p->vkCode;
					ret = SendInput(1, inputs, sizeof(INPUT));
				}

				//
				if (Shift)
				{
					inputs[0].ki.wVk = 161;
					inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
					ret = SendInput(1, inputs, sizeof(INPUT));
					Shift = false;
				}
				return 1;
			}
			
			////////////////////////////////////////////////////////////////////
			//%%C
			if (GetCaretPos(&point)) {
				HWND hWndEditA = GetForegroundWindow();

				if (hWndEdit != hWndEditA && hWndEditA != hWndSuggestions) {
					hWndEdit = hWndEditA; // ��������� �������� ����, ���� ��� �� ���� � �����������
				}
				SetForegroundWindow(hWndEdit);
				Sleep(100); // ������� ���� ��� ��������� ����
				point = GetCaretPosition();
				ClientToScreen(hWndEdit, &point);
			}

			//UpdateSuggestions(); // ��������� ���������
			//%%C

			///////////////////KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

			// ��������� ������ ������� ������� ������
			if (nCode == HC_ACTION && wParam == WM_KEYDOWN) {
				BYTE keyboardState[256];
				GetKeyboardState(keyboardState);

				// ��������� ������� � ������ ������� ��������� ����������
				WCHAR buffer[2];
				int result = ToUnicode(p->vkCode, p->scanCode, keyboardState, buffer, 2, 0);

				if (result == 1) { // ���� ������� ���� ������
					if (buffer[0]) { // ���������, ��� ������ ��������
						input += buffer[0];
					}
				}

				//if ((p->flags & LLKHF_INJECTED)  != 0) {
				UpdateSuggestions(); // ��������� ���������
				//}
				// �������������� ���������� ���������� ��� ������������ �������
				//printf("Current input: %s\n", input.c_str());

				//////////////////////////////////////////////////////////////////////////
			}
		}
	}

	return 0;
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//HWND AppWnd;
	///
	//KBDLLHOOKSTRUCT *p = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);
	if (!CaptureEnabled)
	{
		return 0;
	}
	INPUT inputs[1];
	UINT ret;

	inputs[0].type = INPUT_KEYBOARD;
	inputs[0].ki.wScan = 0;
	inputs[0].ki.dwFlags = 0;
	inputs[0].ki.time = 0;
	inputs[0].ki.dwExtraInfo = 0;

	if (wParam == WM_LBUTTONDOWN)
	{
		LBUTTONDOWN = true;
		if (RBUTTONDOWN)
		{
			//LCTRL KEYDOWN	
			inputs[0].ki.wVk = 162;
			inputs[0].ki.dwFlags = 0;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//C KEYDOWN
			inputs[0].ki.wVk = 86;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//LCTRL KEYUP	
			inputs[0].ki.wVk = 162;
			inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//C KEYUP
			inputs[0].ki.wVk = 86;
			ret = SendInput(1, inputs, sizeof(INPUT));

			lRetFalse = true;
			return 1;
		}
	}

	if (wParam == WM_LBUTTONUP)
	{
		LBUTTONDOWN = false;
		/////////////////
		if (lRetFalse)
		{
			lRetFalse = false;
			return 1;
		}
		////////////////
	}

	if (wParam == WM_MBUTTONDOWN)
	{
		MBUTTONDOWN = true;
		if (RBUTTONDOWN)
		{
			//LCTRL KEYDOWN	
			inputs[0].ki.wVk = 162;
			inputs[0].ki.dwFlags = 0;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//C KEYDOWN
			inputs[0].ki.wVk = 86;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//LCTRL KEYUP	
			inputs[0].ki.wVk = 162;
			inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//C KEYUP
			inputs[0].ki.wVk = 86;
			ret = SendInput(1, inputs, sizeof(INPUT));
			//mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); //���������

			mRetFalse = true;
			return 1;
		}
	}

	if (wParam == WM_MBUTTONUP)
	{
		if (RBUTTONDOWN)
		{
			//return 1;
		}
		else
		{
			MBUTTONDOWN = false;
		}
		////////////////////
		if (mRetFalse)
		{
			mRetFalse = false;
			return 1;
		}
		////////////////////
	}

	if (wParam == WM_RBUTTONDOWN)
	{
		RBUTTONDOWN = true;
		if (MBUTTONDOWN)
		{
			//LCTRL KEYDOWN	
			inputs[0].ki.wVk = 162;//163;
			inputs[0].ki.dwFlags = 0;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//C KEYDOWN
			inputs[0].ki.wVk = 67;//45;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//LCTRL KEYUP	
			inputs[0].ki.wVk = 162;//163;
			inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
			ret = SendInput(1, inputs, sizeof(INPUT));

			//C KEYUP
			inputs[0].ki.wVk = 67;//45;
			ret = SendInput(1, inputs, sizeof(INPUT));

			rRetFalse = true;
			return 1;
		}
	}
	if (wParam == WM_RBUTTONUP)
	{
		RBUTTONDOWN = false;
		if (MBUTTONDOWN)
		{
			MBUTTONDOWN = false;
			//return 1;
		}
		///////////////
		if (rRetFalse)
		{
			rRetFalse = false;
			return 1;
		}
		///////////////
	}

	printf("%d\n", wParam);
	printf("MBUTTONDOWN %d\n", MBUTTONDOWN);
	printf("RBUTTONDOWN %d\n", RBUTTONDOWN);
	//printf("nCode %d\n", nCode);

	return 0;
}

bool isProcessRun(char* processName)
{
	bool Relitigation = false;

	HANDLE hSnap = NULL;
	PROCESSENTRY32 pe32;
	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != NULL)
	{
		if (Process32First(hSnap, &pe32))
		{
			if (strcmp(pe32.szExeFile, processName) == 0)
				return TRUE;
			while (Process32Next(hSnap, &pe32))
				if (strcmp(pe32.szExeFile, processName) == 0)
				{
					if (Relitigation)
						return TRUE;
					Relitigation = true;
				}
		}
	}
	if (hSnap)
	{
		CloseHandle(hSnap);
	}

	return FALSE;
}

BOOL ShowPopupMenu(HWND hWnd, HINSTANCE hInstance, WORD nResourceID)
{
	HMENU hMenu = ::LoadMenu(hInstance,
		MAKEINTRESOURCE(nResourceID));
	if (!hMenu)  return FALSE;
	HMENU hPopup = ::GetSubMenu(hMenu, 0);
	if (!hPopup) return FALSE;

	SetForegroundWindow(hWnd);

	POINT pt;
	GetCursorPos(&pt);
	BOOL bOK = TrackPopupMenu(hPopup, 0, pt.x, pt.y, 0, hWnd, NULL);//TPM_RETURNCMD

	DestroyMenu(hMenu);
	return bOK;
}

// ���������� ��������� ��� ���� "�������".
INT_PTR CALLBACK Reference(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	switch (message)
	{
	case WM_INITDIALOG:
		return (INT_PTR)TRUE;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		}
		break;
	}
	return (INT_PTR)FALSE;
}



LRESULT CALLBACK WndProcA(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int wmId;

	if (uMsg == MYWM_NOTIFYICON && lParam == WM_RBUTTONDOWN)
	{
		ShowPopupMenu(hWnd, hInst, IDR_MENU1);
	}

	switch (uMsg)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		// ��������� ����� � ����:
		switch (wmId)
		{
		case ID_MENU_Switching:
			CaptureEnabled = !CaptureEnabled;
			if (CaptureEnabled)
				TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), "Half keyboard\n(������ �������)");
			else
				TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), "Half keyboard\n(������ ��������)");

			break;
		case ID_MENU_Reference:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_REFERENCE), hWnd, Reference);
			break;
		case ID_MENU_About:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, Reference);
			break;
		case ID_MENU_EXIT:
			DestroyWindow(hWnd);
			DestroyWindow(hWndSuggestions);
			UnhookWindowsHookEx;
			UnhookWindowsHookEx;
			MessageBox(0, "���������", "����� ��������!!!", 65536);//MsgBoxSetForeground
			TrayMessage(hWnd, NIM_DELETE, 0, 0, 0);
			exit(0);
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	case WM_CREATE:

		break;

	case WM_LBUTTONDOWN: {
		/*// ��������� ������ �� ������ � ���� ���������
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWndSuggestions, &pt);

		RECT rect;
		GetClientRect(hWndSuggestions, &rect);

		HDC hdc = GetDC(hWndSuggestions);
		SIZE size;
		string matchedSuggestions;
		for (const auto& suggestion : suggestions) {
			matchedSuggestions += suggestion + L"\n";
			GetTextExtentPoint32(hdc, matchedSuggestions.c_str(), matchedSuggestions.length(), &size);

			if (pt.y < size.cy) {
				//MessageBox(hWnd, suggestion.c_str(), "Selected Suggestion", MB_OK);
				break;
			}
		}
		ReleaseDC(hWndSuggestions, hdc);
		*/
		break;
	}
	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

// ������� ��� ���������� ������ �� �����������
vector<wstring> split(const wstring& str, const wstring& delimiter) {
	vector<wstring> tokens;
	size_t start = 0, end;
	while ((end = str.find(delimiter, start)) != wstring::npos) {
		tokens.push_back(str.substr(start, end - start));
		start = end + delimiter.length();
	}
	tokens.push_back(str.substr(start));
	return tokens;
}

LRESULT CALLBACK SuggestionsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_LBUTTONDOWN: {
		// ��������� ������ �� ������ � ���� ���������
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWndSuggestions, &pt);

		RECT rect;
		GetClientRect(hWndSuggestions, &rect);

		HDC hdc = GetDC(hWndSuggestions);
		SIZE size;
		int yOffset = 0;  // �������� �� ��������� ��� ������� ��������
		//string matchedSuggestions;

		for (const auto& suggestion : split(matchedSuggestions, L"\r\n")) {
			// �������� ������ ������� ������
			GetTextExtentPoint32W(hdc, suggestion.c_str(), suggestion.length(), &size);

			// ���������, ��������� �� ���� � �������� �������� ��������
			if (pt.y >= yOffset && pt.y < yOffset + size.cy) {
				//MessageBox(hWnd, suggestion.c_str(), "Selected Suggestion", MB_OK);
				InsertSelectedSuggestion(suggestion.c_str());
				break;
			}

			// ����������� �������� ��� ���������� ��������
			yOffset += size.cy;
		}
		//matchedSuggestions.clear();
		ReleaseDC(hWndSuggestions, hdc);
		break;
	}
	case WM_ERASEBKGND: {
		HDC hdc = (HDC)wParam;
		RECT rect;
		GetClientRect(hWnd, &rect);
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1)); // ��������� ������ ����
		return 1;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);
		RECT rect;
		GetClientRect(hWnd, &rect);
		char text[1024];
		GetWindowText(hWnd, text, sizeof(text));
		DrawText(hdc, text, -1, &rect, DT_LEFT);
		EndPaint(hWnd, &ps);
		break;
	}
	case WM_KEYDOWN:
		/*
		if (wParam == VK_RETURN) { // ������� Enter �������� ���������
			InsertSelectedSuggestion(suggestions[selectedSuggestion]); // ������� ���������� �����
			return 0;
		}
		OnKeyDown(wParam, hWndEdit);
		*/
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

/*
LRESULT CALLBACK SuggestionsWndProcA(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_LBUTTONDOWN: {
		// ��������� ������ �� ������ � ���� ���������
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWndSuggestions, &pt);

		RECT rect;
		GetClientRect(hWndSuggestions, &rect);

		HDC hdc = GetDC(hWndSuggestions);
		SIZE size;
		wstring matchedSuggestions;
		for (const auto& suggestion : suggestions) {
			matchedSuggestions += suggestion + L"\n";
			GetTextExtentPoint32W(hdc, matchedSuggestions.c_str(), matchedSuggestions.length(), &size);

			if (pt.y < size.cy) {
				//MessageBox(hWnd, suggestion.c_str(), "Selected Suggestion", MB_OK);
				break;
			}
		}
		ReleaseDC(hWndSuggestions, hdc);
		break;
	}
	case WM_KEYDOWN:
		if (wParam == VK_RETURN) { // ������� Enter �������� ���������
			//InsertSelectedSuggestion(L"example"); // ������� ���������� �����
			return 0;
		}
		OnKeyDown(wParam, hWndEdit);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}
*/

//int main(int argc, TCHAR* argv[])
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG messages;

	/////////////////////////////////////////////////////////////////////////////////
	wstring filename = L"suggestions.txt";
	//wstring filename = L"C:\\Users\\38301\\source\\repos\\Half-keyboard2\\Debug\\suggestions.txt"; // �������� �� ��� ����
	//wstring filename = L"D:\\suggestions.txt"; // �������� �� ��� ����

	suggestions = {
		L"������",
		L"������",
		L"���������",
		L"����",
		L"���������",
		L"������"
	};

	// ���������� ������� � ����
	//SaveSuggestions(suggestions, filename);

	// �������� ������� �� �����
	suggestions = LoadSuggestions(filename);
	/////////////////////////////////////////////////////////////////////////////////


	if (isProcessRun("HalfKeyboard.exe"))
	{
		MessageBox(0, "HalfKeyboard.exe ��� �������", "��������� ������!", 0);
		return 1;
	}

	//
	//MyRegisterClass(hInstance);

	WNDCLASS wc;

	wc.cbClsExtra = 0;                              //�������������� ��������� ������
	wc.cbWndExtra = 0;                              //�������������� ��������� ����
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);    //���� ����
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);       //������
	//wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);         //������
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINLOGO));
	wc.hInstance = hInstance;//���������� ����������
	wc.lpfnWndProc = WndProcA;                       //��� �-�� ��������� ���������
	wc.lpszClassName = "szWindowClass";               //��� ������ ����
	wc.lpszMenuName = NULL;                         //������ �� ������� ����
	wc.style = CS_VREDRAW | CS_HREDRAW;             //����� ����

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "�� ������� ���������������� ����� ����!", "������ �����������", MB_ICONERROR | MB_OK);
		return 1;
	}

	// ��������������� ����� ���� ���������
	WNDCLASS wcSuggestions = { };
	wcSuggestions.lpfnWndProc = SuggestionsWndProc;
	wcSuggestions.hInstance = hInstance;
	wcSuggestions.lpszClassName = "SuggestionsWndClass";

	if (!RegisterClass(&wcSuggestions))
	{
		MessageBox(NULL, "�� ������� ���������������� ����� ���� ���������!", "������ �����������", MB_ICONERROR | MB_OK);
		return 1;
	};



	hInst = hInstance;
	hWnd = CreateWindow("szWindowClass", NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return 1;
	/*
	// ������� ���� ��� ���������
	hWndSuggestions = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // ����� ��� ��������� ������ ����
		"EDIT", // ����� ���� (����� ������������ ����� ��� ���������� ����)
		"", // ����� �� ���������
		WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY, // ����� ����
		0, 0, 150, 100, // ��������� ��������� � �������
		hWnd, // ������������ ����
		NULL, // ������������� ����
		hInstance, // ���������� ���������� ����������
		NULL // �������������� ���������
	);
	*/
	InitializeSuggestionWindow(hInstance);

	if (!hWndSuggestions)
		return 1;

	//ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_HIDE);
	TrayMessage(hWnd, NIM_ADD, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), "Half keyboard\n(������ �������)");
	//TrayMessage(hWnd, NIM_ADD, 0, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2)), "Half keyboard\n(������ ��������)");	

	//////////////////////////////////////////////////////////////////////////////
	//creating an array of matching keys
	for (size_t i = 0; i < ArrayZize; i++)
	{
		MatchingKeys[i] = 0;

		//right hand
		if (i == 72)
			MatchingKeys[i] = 71;
		if (i == 74)
			MatchingKeys[i] = 70;
		if (i == 75)
			MatchingKeys[i] = 68;
		if (i == 76)
			MatchingKeys[i] = 83;
		if (i == 186)
			MatchingKeys[i] = 65;
		if (i == 89)
			MatchingKeys[i] = 84;
		if (i == 85)
			MatchingKeys[i] = 82;
		if (i == 73)
			MatchingKeys[i] = 69;
		if (i == 79)
			MatchingKeys[i] = 87;
		if (i == 80)
			MatchingKeys[i] = 81;
		if (i == 219)
			//MatchingKeys[i] = 9;
			MatchingKeys[i] = 219;
		if (i == 221)
			//MatchingKeys[i] = 9;
			MatchingKeys[i] = 221;
		if (i == 78)
			MatchingKeys[i] = 66;
		if (i == 77)
			MatchingKeys[i] = 86;
		if (i == 188)
			MatchingKeys[i] = 67;
		if (i == 190)
			MatchingKeys[i] = 88;
		if (i == 191)
			MatchingKeys[i] = 90;
		if (i == 54)
			MatchingKeys[i] = 53;
		if (i == 55)
			MatchingKeys[i] = 52;
		if (i == 56)
			MatchingKeys[i] = 51;
		if (i == 57)
			MatchingKeys[i] = 50;
		if (i == 48)
			MatchingKeys[i] = 49;
		if (i == 189)
			MatchingKeys[i] = 192;
		if (i == 222)
			MatchingKeys[i] = 222;
		if (i == 8)
			MatchingKeys[i] = 9;
		if (i == 187)
			MatchingKeys[i] = 187;
		if (i == 220)
			MatchingKeys[i] = 220;


		//left hand
		if (i == 71)
			MatchingKeys[i] = 72;
		if (i == 70)
			MatchingKeys[i] = 74;
		if (i == 68)
			MatchingKeys[i] = 75;
		if (i == 83)
			MatchingKeys[i] = 76;
		if (i == 65)
			MatchingKeys[i] = 186;
		if (i == 84)
			MatchingKeys[i] = 89;
		if (i == 82)
			MatchingKeys[i] = 85;
		if (i == 69)
			MatchingKeys[i] = 73;
		if (i == 87)
			MatchingKeys[i] = 79;
		if (i == 81)
			MatchingKeys[i] = 80;
		if (i == 9)
			MatchingKeys[i] = 8;
		/*if (i == 221)
			MatchingKeys[i] = 9;*/
		if (i == 66)
			MatchingKeys[i] = 78;
		if (i == 86)
			MatchingKeys[i] = 77;
		if (i == 67)
			MatchingKeys[i] = 188;
		if (i == 88)
			MatchingKeys[i] = 190;
		if (i == 90)
			MatchingKeys[i] = 191;
		if (i == 53)
			MatchingKeys[i] = 54;
		if (i == 52)
			MatchingKeys[i] = 55;
		if (i == 51)
			MatchingKeys[i] = 56;
		if (i == 50)
			MatchingKeys[i] = 57;
		if (i == 49)
			MatchingKeys[i] = 48;
		if (i == 192)
			MatchingKeys[i] = 189;
		if (i == 20)
			MatchingKeys[i] = 222;
		//

	}
	MessageBox(0, "��������", "����� ����������!", 0);

	hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHook, NULL, 0);
	if (hook == NULL) {
		printf("Error %d\n", GetLastError());
		system("pause");
		return 1;
	}
	/////

	//	HWND hWnd = GetForegroundWindow();
	/*�� ����� ������������� Mause Button Controll v.2.20.5
	* ��������, ��-�� ���������
	mousehook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, NULL, 0);
	if (mousehook == NULL) {
		printf("Error %d\n", GetLastError());
		system("pause");
		return 1;
	}
	*/

	


	//printf("Waiting for messages ...\n");
	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 0;

}
