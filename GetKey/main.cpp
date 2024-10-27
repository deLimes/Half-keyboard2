#include <Windows.h>
#include <stdio.h>

#include <cstdio>
#include <tlhelp32.h>
#include <tchar.h>

//////////////////////////////////////
#include <time.h>
#include <iostream>
#include <Shellapi.h>
#include "resource.h"
using namespace std;
//////////////////////////////////////

#define MYWM_NOTIFYICON (WM_USER + 1)
// Описываем сообщение, которое будет посылаться при взаимодействии юзера с нашей иконкой в систрее

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

/*  Declare Windows procedure  */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

BOOL TrayMessage(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, PSTR pszTip)
// systray icon 
{
	BOOL res;

	NOTIFYICONDATA tnd;

	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hDlg;
	tnd.uID = uID;

	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage = MYWM_NOTIFYICON; // Сообщение, которое пошлется при всяких там кликах на иконке... 
	tnd.hIcon = hIcon;

	if (pszTip)
	{
		lstrcpyn(tnd.szTip, pszTip, sizeof(tnd.szTip));
	}
	else
	{
		tnd.szTip[0] = '\0';
	}

	res = Shell_NotifyIcon(dwMessage, &tnd);


	return res;
}

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
			//	MessageBox(0, "Захват включен", "Активация Half keyboard", MB_ICONINFORMATION);		
			TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), "Half keyboard\n(Захват включен)");
		else
			//	MessageBox(0, "Захват выключен", "Дективация Half keyboard", MB_ICONSTOP);
			TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), "Half keyboard\n(Захват выключен)");

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
			MessageBox(0, "Закончили", "Всего хорошего!!!", 65536);//MsgBoxSetForeground
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
				//п
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
				//х,ъ-tab,ё
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
			//mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP, 0, 0, 0, 0); //отпустили

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

// Обработчик сообщений для окна "Справка".
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

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
		// Разобрать выбор в меню:
		switch (wmId)
		{
		case ID_MENU_Switching:
			CaptureEnabled = !CaptureEnabled;
			if (CaptureEnabled)
				TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), "Half keyboard\n(Захват включен)");
			else
				TrayMessage(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), "Half keyboard\n(Захват выключен)");

			break;
		case ID_MENU_Reference:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_REFERENCE), hWnd, Reference);
			break;
		case ID_MENU_About:
			DialogBox(hInst, MAKEINTRESOURCE(IDD_ABOUT), hWnd, Reference);
			break;
		case ID_MENU_EXIT:
			DestroyWindow(hWnd);
			UnhookWindowsHookEx;
			UnhookWindowsHookEx;
			MessageBox(0, "Закончили", "Всего хорошего!!!", 65536);//MsgBoxSetForeground
			TrayMessage(hWnd, NIM_DELETE, 0, 0, 0);
			exit(0);
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	default:
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	return 0;
}

//int main(int argc, TCHAR* argv[])
int CALLBACK WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	MSG messages;

	if (isProcessRun("HalfKeyboard.exe"))
	{
		MessageBox(0, "HalfKeyboard.exe уже ЗАПУЩЕН", "Повторный запуск!", 0);
		return 1;
	}

	//
	//MyRegisterClass(hInstance);

	WNDCLASS wc;

	wc.cbClsExtra = 0;                              //Дополнительные параметры класса
	wc.cbWndExtra = 0;                              //Дополнительные параметры окна
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);    //Цвет окна
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);       //Курсор
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);         //Иконка
	wc.hInstance = hInstance;//Дискриптор приложения
	wc.lpfnWndProc = WndProc;                       //Имя ф-ии обработки сообщений
	wc.lpszClassName = "szWindowClass";               //Имя класса окна
	wc.lpszMenuName = NULL;                         //Ссылка на главное меню
	wc.style = CS_VREDRAW | CS_HREDRAW;             //Стиль окна

	if (!RegisterClass(&wc))
	{
		MessageBox(NULL, "Не удалось зарегистрировать класс окна!", "Ошибка регистрации", MB_ICONERROR | MB_OK);
	}

	hInst = hInstance;
	hWnd = CreateWindow("szWindowClass", NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return 1;

	//ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_HIDE);
	TrayMessage(hWnd, NIM_ADD, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), "Half keyboard\n(Захват включен)");
	//TrayMessage(hWnd, NIM_ADD, 0, LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON2)), "Half keyboard\n(Захват выключен)");	

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
	/////////////////////////////////////////////////////////////////////////////////
	MessageBox(0, "Начинаем", "Добро пожаловать!", 0);

	hook = SetWindowsHookEx(WH_KEYBOARD_LL, keyboardHook, NULL, 0);
	if (hook == NULL) {
		printf("Error %d\n", GetLastError());
		system("pause");
		return 1;
	}
	/////

	//	HWND hWnd = GetForegroundWindow();
	/*На время использования Mause Button Controll v.2.20.5
	* отключил, из-за конфликта
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

}
