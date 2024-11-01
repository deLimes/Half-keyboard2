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
#include <cctype> // Подключение для isprint
#include <ctype.h> // для C
#include <fstream> // Добавляем этот заголовочный файл
#include <set>
#include <fstream>
#include <locale.h>
#include <tlhelp32.h>
#include <codecvt>
#include <locale>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <cstdlib> // Для использования функции atexit
//#include <winuser.h>




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
bool firstOccurrence = true;
ULONG Delay = 0;
bool CaptureEnabled = true;
HKL hkl = GetKeyboardLayout(0);
HINSTANCE hInst;
HWND hWnd;
HWND hWndSuggestions = nullptr; // Глобальная переменная для окна с подсказками
POINT point;
wstring matchedSuggestions;
static HWND hWndEdit = NULL;
wstring logFilename = L"hk_log.txt";
wstring filenameSuggestions = L"suggestions.txt";
wstring debugMsg = L"";

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
// Глобальная переменная для отслеживания ввода
wstring input;
int wcount = 0;


/*  Declare Windows procedure  */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SuggestionsWndProc(HWND, UINT, WPARAM, LPARAM);

vector<wstring> suggestions;
set<wstring> uniqueSuggestions; // Для хранения уникальных предложений


int selectedSuggestion = -1; // Индекс текущего выбранного предложения

void UpdateSuggestions();
void InitializeSuggestionWindow(HINSTANCE hInstance);
BOOL TrayMessageW(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, LPWSTR pszTip);






void Log(const wstring& message, const wstring& filename) {
	wifstream file(filename); // Открываем файл для чтения
	vector<wstring> lines;    // Вектор для хранения строк из файла

	// Считываем существующие строки в вектор
	wstring line;
	while (getline(file, line)) {
		lines.push_back(line);
	}
	file.close(); // Закрываем файл после чтения

	// Открываем файл для записи (это очистит его содержимое)
	wofstream outFile(filename);
	outFile.imbue(locale(""));

	if (!outFile.is_open()) {
		return;
	}

	// Получаем текущее время
	auto now = chrono::system_clock::now();
	auto in_time_t = chrono::system_clock::to_time_t(now);

	// Используем localtime_s для безопасности
	tm timeInfo;
	localtime_s(&timeInfo, &in_time_t);

	// Форматируем новое сообщение с временем
	outFile << put_time(&timeInfo, L"[%Y-%m-%d %H:%M:%S] ") << message << endl;

	// Записываем ранее считанные строки обратно в файл
	for (const auto& existingLine : lines) {
		outFile << existingLine << endl;
	}

	outFile.close(); // Закрываем файл
}



bool containsDigit(const wstring& line) {
	// Проверка, содержит ли строка цифру
	return any_of(line.begin(), line.end(), ::iswdigit);
}

wstring trimToFirstSpace(const wstring& line) {
	// Поиск первого пробела
	size_t pos = line.find(L' ');
	// Если пробел найден, возвращаем подстроку до него
	if (pos != wstring::npos) {
		return line.substr(0, pos);
	}
	// Если пробела нет, возвращаем всю строку
	return line;
}

wstring trimToFirstTab(const wstring& line) {
	// Поиск первого пробела
	size_t pos = line.find(L'	');
	// Если пробел найден, возвращаем подстроку до него
	//if (pos != wstring::npos) {
	return line.substr(0, pos);
	//}
	// Если пробела нет, возвращаем всю строку
	//return line;
}

wstring toLowerCase(const wstring& str) {
	// Преобразование всех символов строки в нижний регистр
	wstring result = str;
	transform(result.begin(), result.end(), result.begin(),
		[](wchar_t c) { return tolower(c, locale()); });
	return result;
}

wstring capitalizeFirstLetter(const wstring& str) {
	// Преобразование первой буквы в заглавную
	wstring result = str;
	if (!result.empty()) {
		result[0] = toupper(result[0], locale());
	}
	return result;
}


void UpdateDict() {
	
	/////////////////////////////////////////////////////////////////////////////
	wstring inputFilename = L"opcorpora.txt";
	wstring outputFilename = L"dict_output.txt";

	// Открытие файла для чтения с кодировкой UTF-8
	wifstream inputFile(inputFilename, ios::binary);
	inputFile.imbue(locale(inputFile.getloc(), new codecvt_utf8<wchar_t>)); // Установка UTF-8 кодировки

	if (!inputFile) {
		throw runtime_error("Не удалось открыть файл для чтения");
	}

	// Открытие файла для записи с кодировкой UTF-8
	wofstream outputFile(outputFilename, ios::binary);
	outputFile.imbue(locale(outputFile.getloc(), new codecvt_utf8<wchar_t>)); // Установка UTF-8 кодировки

	if (!outputFile.is_open()) {
		throw runtime_error("Не удалось открыть файл для записи");
	}

	//////-=1=-/////////////////////////////////////////////////////////////////////////
	/*
	vector<wstring> lines;
	wstring line;

	// Чтение файла построчно и удаление пустых строк
	while (getline(inputFile, line)) {
		wcount++;
		try
		{
			if (!line.empty() && !containsDigit(line)) { // Проверка на пустую строку и наличие цифр
				
				
				wstring lowerLine = toLowerCase(trimToFirstTab(trimToFirstSpace(line)));
				outputFile << lowerLine << endl; // Запись строки в файл
				outputFile << capitalizeFirstLetter(lowerLine) << endl; // Запись строки в файл

				
			}
			else {
				continue;
			}
		}
		catch (const exception&)
		{
			wstring message = L"count: " + to_wstring(wcount);
			//PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(message.c_str()), 0); // Отправляем сообщение окну
			Log(message, logFilename);
			//continue;
		}

	}
	*/
	//////-=1=-/////////////////////////////////////////////////////////////////////////

	//////-=2=-/////////////////////////////////////////////////////////////////////////
	set<wstring> uniqueLines; // Множество для хранения уникальных строк
	wstring line;

	while (getline(inputFile, line)) {
		uniqueLines.insert(line); // Добавляем строку в множество (дубликаты не сохраняются)
	}

	vector<wstring> vectorUniqueLines = vector<wstring>(uniqueLines.begin(), uniqueLines.end()); // Преобразуем множество в вектор

	for (const auto& line : vectorUniqueLines) {
		outputFile << line << endl; // Записываем уникальные строки в файл
	}

	outputFile.close();
	inputFile.close(); // Закрытие файла после чтения
	//////-=2=-/////////////////////////////////////////////////////////////////////////

}

void SaveSuggestions(const vector<wstring>& suggestions, const wstring& filename) {
	wofstream file(filename);
	file.imbue(locale(""));

	if (!file.is_open()) {
		return;
	}

	for (const auto& suggestion : suggestions) {
		file << suggestion << L"\n";
	}

	file.close();

}



void LoadSuggestionsUTF8(const wstring& filename) {
	
	CaptureEnabled = false;
	TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), L"Half keyboard\n(Загрузка словаря...)");

	wifstream file(filename, ios::binary);
	file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>)); // UTF-8 кодировка

	if (!file) {
		throw runtime_error("Не удалось открыть файл");
	}

	wstring line;
	wstring message = L"";
	Log(L"begin while", logFilename);
	while (getline(file, line)) {
		
		if (!line.empty()) {
			//line.erase(remove(line.begin(), line.end(), L'\r'), line.end()); // Удаляем символы \r
			suggestions.push_back(line);
			//message += (line + L"\n");

			/*
			// Проверка на длину и наличие пробелов
			if (line.length() > 1 || any_of(line.begin(), line.end(), [](wchar_t c) { return iswspace(c) == 0; })) {
				suggestions.push_back(line); // Добавляем строку только если она не состоит из одного символа без пробела
			}
			*/

		}

	}
	Log(message, logFilename);
	file.close();
	CaptureEnabled = true;
	TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");
}

vector<wstring> LoadSuggestionsA(const wstring& filename) {
	vector<wstring> suggestions;
	wifstream file(filename);
	if (!file.is_open()) {
		wcerr << L"Не удалось открыть файл для чтения: " << filename << endl;
		return suggestions;
	}

	wstring line;
	while (getline(file, line)) {
		suggestions.push_back(line); // Загружаем каждую строку из файла в вектор
	}

	file.close(); // Закрываем файл после чтения
	return suggestions;
}

POINT getCaretPosition(HWND hwnd) {
	POINT point;
	if (GetCaretPos(&point)) {
		// Преобразуем координаты в экранные
		ClientToScreen(hwnd, &point);
		return point;
	}
	else {
		point.x = -1;
		point.y = -1;
		return point;
	}
}

/*
// Функция получения позиции каретки с учетом пересчета в экранные координаты
POINT GetAdjustedCaretPosition(HWND hwndEdit) {
	POINT caretPos;
	if (GetCaretPos(&caretPos)) {
		// Пересчитываем координаты каретки в экранные
		ClientToScreen(hwndEdit, &caretPos);
	}
	else {
		caretPos.x = -1;
		caretPos.y = -1;
	}
	return caretPos;
}
*/

void SendString(const wstring& text) {
	INPUT inputs[2]; // Для каждого символа нужны два INPUT (нажатие и отпускание клавиши)
	ZeroMemory(inputs, sizeof(inputs));

	for (wchar_t ch : text) {
		// Настройка нажатия клавиши (KEYDOWN)
		if (input.find(ch) == 0 && !input.empty()) {
			continue;
		}
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = 0; // Не используется для символов Unicode
		inputs[0].ki.wScan = ch;
		inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
		Sleep(100);
		// Настройка отпускания клавиши (KEYUP)
		inputs[1] = inputs[0];
		inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

		// Отправляем нажатие и отпускание
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

// Функция для подстановки выбранного слова
void InsertSelectedSuggestion(const wstring& word) {
	//HWND hWndEdit = GetForegroundWindow(); // Окно, где идет набор текста

		// Подстановка текста в поле ввода
		//SendMessage(hWndEdit, EM_REPLACESEL, TRUE, (LPARAM)word.c_str());

	SetForegroundWindow(hWndEdit);
	Sleep(100); // Немного ждем для активации окна

	// Создаем копию word для изменения
	wstring modifiedWord = word;

	// Удаляем input из modifiedWord, если modifiedWord начинается с input
	if (modifiedWord.find(input) == 0) { // Проверяем, начинается ли modifiedWord с input
		modifiedWord.erase(0, input.length()); // Удаляем input из начала modifiedWord
	}

	// Подстановка текста в поле ввода
	SendString(modifiedWord);

	SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);

	SetForegroundWindow(hWndEdit);
	Sleep(100); // Немного ждем для активации окна

}



void UpdateSuggestionsWindow(HWND hwndSuggestions, const wstring& text) {
	HDC hdc = GetDC(hwndSuggestions);
	RECT rect = { 0, 0, 0, 0 };

	// Получаем размер текста
	DrawTextW(hdc, text.c_str(), -1, &rect, DT_CALCRECT);

	if (!hwndSuggestions) {
		MessageBoxW(NULL, L"Ошибка создания окна", L"Ошибка", MB_OK | MB_ICONERROR);
	}
	// Задаём текст в окне подсказок
	SetWindowTextW(hwndSuggestions, text.c_str());

	// Изменяем размер окна в соответствии с размером текста
	SetWindowPos(hwndSuggestions, HWND_TOP, 0, 0, rect.right - rect.left + 10, rect.bottom - rect.top + 10, SWP_NOZORDER | SWP_NOMOVE);
	// Обновление окна 
	InvalidateRect(hWndSuggestions, NULL, TRUE);
	UpdateWindow(hWndSuggestions);

	ReleaseDC(hwndSuggestions, hdc);


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


/*
POINT GetCaretPosFromFocusedWindow() {
	POINT caretPos = { -1, -1 };

	// Получаем дескриптор активного окна
	HWND hwnd = GetForegroundWindow();
	if (hwnd == NULL) {
		cerr << "No active window found" << endl;
		return caretPos;
	}

	GUITHREADINFO guiThreadInfo = { 0 };
	guiThreadInfo.cbSize = sizeof(GUITHREADINFO);

	// Получаем информацию о потоке GUI, включая каретку
	if (GetGUIThreadInfo(0, &guiThreadInfo)) {
		if (guiThreadInfo.hwndCaret != NULL) {
			// Получаем позицию каретки
			if (GetCaretPos(&caretPos)) {
				// Преобразуем координаты в экранные
				ClientToScreen(guiThreadInfo.hwndCaret, &caretPos);
				return caretPos;
			}
		}
	}
	return caretPos;
}
*/



// Функция для обновления окна с подсказками
void UpdateSuggestions() {
	// Пример: фильтрация предложений по введенному тексту
	uniqueSuggestions.clear();
	int count = 0; // Счётчик добавленных элементов
	for (const auto& suggestion : suggestions) {

		if (suggestion.find(input) == 0 && !input.empty()) {
			uniqueSuggestions.insert(suggestion); // Добавляем только уникальные предложения
			++count;
			if (count == 5) { // Останавливаемся на шестом элементе
				break;
			}
		}
	}
	matchedSuggestions.clear();
	// Собираем уникальные предложения в строку
	for (const auto& suggestion : uniqueSuggestions) {
		matchedSuggestions += suggestion + L"\r\n"; // Формируем строку
	}


	if (matchedSuggestions.empty()) {
		if (IsWindowVisible(hWndSuggestions)) {
			SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
		}
		//SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
		///input.clear();
	}
	else {
		SetWindowPos(hWndSuggestions, HWND_TOP, point.x, point.y + 20, 150, 100, SWP_SHOWWINDOW);
	}

	UpdateSuggestionsWindow(hWndSuggestions, matchedSuggestions);

}

void del_InitializeSuggestionWindow_(HINSTANCE hInstance) {

	// Регистрация класса окна
	WNDCLASSW wc = { 0 };
	wc.lpfnWndProc = SuggestionsWndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = L"SuggestionsWndClass";
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

	if (!RegisterClassW(&wc)) {
		MessageBoxW(NULL, L"Не удалось зарегистрировать класс окна подсказок", L"Ошибка", MB_ICONERROR);
		return;
	}


	// Создание окна
	hWndSuggestions = CreateWindowEx(
		WS_EX_TOPMOST,
		L"SuggestionsWndClass",
		L"Окно Подсказки",
		WS_POPUP | WS_BORDER | SS_LEFT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		150, 100,
		hWnd,
		NULL,
		hInstance,
		NULL
	);

	if (!hWndSuggestions) {
		MessageBoxW(NULL, L"Не удалось создать окно подсказок", L"Ошибка", MB_ICONERROR);
		return;
	}

	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hWndSuggestions, WM_SETFONT, (WPARAM)hFont, TRUE);

	ShowWindow(hWndSuggestions, SW_SHOW);
	UpdateWindow(hWndSuggestions);
	InvalidateRect(hWndSuggestions, NULL, TRUE); // Принудительно вызываем WM_PAINT
}
void InitializeSuggestionWindow(HINSTANCE hInstance) {

	// Создайте окно подсказок
	hWndSuggestions = CreateWindowEx(
		WS_EX_TOPMOST,
		L"SuggestionsWndClass",
		L"Окно Подсказки",
		WS_POPUP | WS_BORDER | SS_LEFT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		150, 100,
		hWnd,
		NULL,
		hInstance,
		NULL
	);

	// Устанавливаем шрифт для удобства чтения
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hWndSuggestions, WM_SETFONT, (WPARAM)hFont, TRUE);

	SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
}





bool IsRussianKeyboardLayout() {
	// Получаем окно в фокусе
	HWND hWnd = GetForegroundWindow();
	if (hWnd == NULL) return false;

	// Получаем идентификатор потока для окна в фокусе
	DWORD threadId = GetWindowThreadProcessId(hWnd, NULL);

	// Получаем раскладку клавиатуры для этого потока
	HKL hkl = GetKeyboardLayout(threadId);

	// Проверяем, что раскладка соответствует русской (0x0419)
	return (LOWORD(hkl) == 0x0419);
}


/*
wstring ConvertToWString(const string& str) {
	wstring wstr(str.size(), L'\0');
	mbstowcs(&wstr[0], str.c_str(), str.size());
	return wstr;
}
*/

// Функция для разделения строки по разделителю
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



BOOL TrayMessageW(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, LPWSTR pszTip) {
	BOOL res;

	NOTIFYICONDATAW tnd;
	tnd.cbSize = sizeof(NOTIFYICONDATA);
	tnd.hWnd = hDlg;
	tnd.uID = uID;

	tnd.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	tnd.uCallbackMessage = MYWM_NOTIFYICON;
	tnd.hIcon = hIcon;


	if (pszTip) {
		// Используем wcsncpy_s для безопасного копирования
		wcsncpy_s(tnd.szTip, pszTip, _TRUNCATE);
	}
	else {
		tnd.szTip[0] = L'\0';
	}

	res = Shell_NotifyIconW(dwMessage, &tnd);
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

	if (nCode != HC_ACTION)
	{
		return 0;
	}


	if (wParam == WM_KEYDOWN) {
		// Обработка нажатия клавиши
		OutputDebugString(L"Key down event detected.\n");
	}
	else if (wParam == WM_KEYUP) {
		// Обработка отпускания клавиши
		OutputDebugString(L"Key up event detected.\n");
	}

	debugMsg = L"Initial key press: nCode: " + to_wstring(nCode) +
		L", wParam: " + to_wstring((unsigned long)wParam) +
		L", vkCode: " + to_wstring(p->vkCode) + L"\n";
	OutputDebugString(debugMsg.c_str());






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
			TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");
		else
			//	MessageBox(0, "Çàõâàò âûêëþ÷åí", "Äåêòèâàöèÿ Half keyboard", MB_ICONSTOP);
			TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), L"Half keyboard\n(Захват выключен)");

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
					if (IsWindowVisible(hWndSuggestions)) {
						SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
					}
					input.clear();
					ret = SendInput(1, inputs, sizeof(INPUT));
				}
				Delay = 0;
				return 1;
			}
		}

		// Alt
		if ((wParam == WM_KEYUP || wParam == WM_SYSKEYUP) && (p->vkCode == VK_RMENU)) {  // VK_MENU - код клавиши Alt
			// Проверяем, открыто ли окно подсказок и есть ли в нем предложения
			if (IsWindowVisible(hWndSuggestions) && !matchedSuggestions.empty()) {
				// Вставляем первое предложение
				wstring firstSuggestion = split(matchedSuggestions, L"\r\n")[0];
				InsertSelectedSuggestion(firstSuggestion);
				return 1; // Останавливаем дальнейшую обработку Alt, чтобы он не повлиял на другое поведение
			}
		}

		//Enter
		if ((p->vkCode == VK_RETURN) && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
		{
			if (IsWindowVisible(hWndSuggestions)) {
				SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
			}
			input.clear();
			//return 0;
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
				SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
				input.clear();
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
			MessageBoxW(0, L"Закончили", L"Всего хорошего!!!", 65536);//MsgBoxSetForeground
			TrayMessageW(hWnd, NIM_DELETE, 0, 0, 0);
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





		/*
		if (nCode == HC_ACTION) {
			if (GetAsyncKeyState(p->vkCode) & 0x8000) {
				wstring debugMsg = L"Key is down: vkCode: " + to_wstring(p->vkCode) + L"\n";
				OutputDebugString(debugMsg.c_str());
			}
			// Ваш текущий вывод
			wstring debugMsg = L"Initial key press: nCode: " + to_wstring(nCode) +
				L", wParam: " + to_wstring((unsigned long)wParam) +
				L", vkCode: " + to_wstring(p->vkCode) + L"\n";
			OutputDebugString(debugMsg.c_str());
		}
		*/


		//Other characters

		if (MatchingKeys[p->vkCode] > 0) {
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
				else {
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

			if (GetCursorPos(&point)) {
				HWND hWndEditA = GetForegroundWindow();

				if (hWndEdit != hWndEditA && hWndEditA != hWndSuggestions) {
					hWndEdit = hWndEditA; // Обновляем активное окно, если это не окно с подсказками
				}
				SetForegroundWindow(hWndEdit);
				Sleep(100); // Немного ждем для активации окна
				// Преобразуем координаты в клиентские координаты текущего окна
				//ScreenToClient(hWndEdit, &point);

			   //ClientToScreen(hWndEdit, &point);
			}

			//UpdateSuggestions(); // ��������� ���������
			//%%C

			///////////////////KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);

			/*
			debugMsg = L"input value1: " + input + L"\n";
			OutputDebugString(debugMsg.c_str());
			Log(debugMsg.c_str(), logFilename);
			*/


			if (p->vkCode == VK_BACK && !input.empty() && wParam == WM_KEYDOWN) {

				input.pop_back(); // Удаляем последний символ при нажатии Backspace
				// Обновляем текстовое поле, чтобы отобразить изменения
				//UpdateTextField(input); // Ваша функция обновления текстового поля
				int m = 1;

			}
			else if (wParam == WM_KEYDOWN) {
				BYTE keyboardState[256];

				// Проверка, что GetKeyboardState успешно выполнилась
				if (GetKeyboardState(keyboardState)) {
					WCHAR buffer[2];
					int result = ToUnicode(p->vkCode, p->scanCode, keyboardState, buffer, 2, 0);

					if (result == 1 && buffer[0]) {
						// Добавляем символ напрямую в input
						input += buffer[0]; // buffer[0] уже в формате WCHAR (UTF-16)
					}
				}
				else {
					// Ошибка при получении состояния клавиатуры
					debugMsg = L"Не удалось получить состояние клавиатуры.";
					OutputDebugString(debugMsg.c_str());
					Log(debugMsg.c_str(), logFilename);
				}
			}
			/*
			else if (wParam == WM_KEYDOWN) {
				BYTE keyboardState[256];

				// Проверка, что GetKeyboardState успешно выполнилась
				if (GetKeyboardState(keyboardState)) {
					WCHAR buffer[2];
					int result = ToUnicode(p->vkCode, p->scanCode, keyboardState, buffer, 2, 0);

					if (result == 1) {
						if (buffer[0]) {
							input += buffer[0];
						}
					}
				}
				else {
					// Ошибка при получении состояния клавиатуры
					debugMsg = L"Не удалось получить состояние клавиатуры.";
					OutputDebugString(debugMsg.c_str());
					Log(debugMsg.c_str(), logFilename);

				}
			}
			*/


			/*
			if (p->vkCode == VK_BACK && !input.empty() && wParam == WM_KEYDOWN) {

				input.pop_back(); // Удаляем последний символ при нажатии Backspace
				// Обновляем текстовое поле, чтобы отобразить изменения
				//UpdateTextField(input); // Ваша функция обновления текстового поля
				int m = 1;

				debugMsg = L"inputVK_BACK value1: " + input + L"\n";
				OutputDebugString(debugMsg.c_str());
				Log(debugMsg.c_str(), logFilename);

			}
			else if (wParam == WM_KEYDOWN) {
				BYTE keyboardState[256];
				GetKeyboardState(keyboardState);


				WCHAR buffer[2];
				int result = ToUnicode(p->vkCode, p->scanCode, keyboardState, buffer, 2, 0);

				if (result == 1) {
					if (buffer[0]) {
						input += buffer[0];
					}
				}

				//////////////////////////////////////////////////////////////////////////
			}
			*/


			/*
			debugMsg = L"input value2: " + input + L"\n";
			OutputDebugString(debugMsg.c_str());
			Log(debugMsg.c_str(), logFilename);
			*/

			//Sleep(100);
			//UpdateSuggestions();
			if (IsRussianKeyboardLayout()) {
				//UpdateSuggestions();		      
				PostMessage(hWnd, WM_UPDATE_SUGGESTIONS, wParam, 0); // Отправляем сообщение окну
			}
			/*
			if (!firstOccurrence)
			{

			}
			firstOccurrence = false;
			*/

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


bool isProcessRun(const wchar_t* processName) {
	bool isRunning = false;

	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (hSnap != INVALID_HANDLE_VALUE) {
		PROCESSENTRY32 pe32;
		pe32.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hSnap, &pe32)) {
			if (wcscmp(pe32.szExeFile, processName) == 0) {
				return true;
			}
			while (Process32Next(hSnap, &pe32)) {
				if (wcscmp(pe32.szExeFile, processName) == 0) {
					if (isRunning) {
						return true;
					}
					isRunning = true;
				}
			}
		}
		CloseHandle(hSnap);
	}

	return false;
}
/*
bool isProcessRun_(char* processName)
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
*/
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

	/*
	debugMsg = L"WndProc";
	Log(debugMsg.c_str(), logFilename);
	*/

	if (uMsg == MYWM_NOTIFYICON && lParam == WM_RBUTTONDOWN)
	{
		ShowPopupMenu(hWnd, hInst, IDR_MENU1);
	}

	//if (uMsg == WM_UPDATE_SUGGESTIONS && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
	if (uMsg == WM_UPDATE_SUGGESTIONS)
	{

		//Log(L"UpdateSuggestions1", logFilename);
		UpdateSuggestions();
	}

	if (uMsg == WM_LOAD_SUGGESTIONS)
	{
		//Log(L"LoadSuggestionsUTF8(filename)", logFilename);
		LoadSuggestionsUTF8(filenameSuggestions);
	}


	if (uMsg == WM_WRITE_LOG)
	{
		// Преобразуем wParam обратно в указатель на wchar_t
		const wchar_t* message = reinterpret_cast<const wchar_t*>(wParam);
		Log(message, logFilename);
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
				TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");
			else
				TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), L"Half keyboard\n(Захват выключен)");

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
			MessageBoxW(0, L"Закончили", L"Всего хорошего!!!", 65536);//MsgBoxSetForeground
			TrayMessageW(hWnd, NIM_DELETE, 0, 0, 0);
			exit(0);
			break;
		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		break;
	case WM_CREATE:

		break;

	case WM_LBUTTONDOWN: {
		/*// Обработка кликов на тексте в окне подсказок
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

LRESULT CALLBACK SuggestionsWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	switch (message) {
	case WM_LBUTTONDOWN: {
		// Обработка кликов на тексте в окне подсказок
		POINT pt;
		GetCursorPos(&pt);
		ScreenToClient(hWndSuggestions, &pt);

		RECT rect;
		GetClientRect(hWndSuggestions, &rect);

		HDC hdc = GetDC(hWndSuggestions);
		SIZE size;
		int yOffset = 0;  // Смещение по вертикали для каждого элемента
		//string matchedSuggestions;

		for (const auto& suggestion : split(matchedSuggestions, L"\r\n")) {
			// Получаем размер текущей строки
			GetTextExtentPoint32W(hdc, suggestion.c_str(), suggestion.length(), &size);

			// Проверяем, находится ли клик в границах текущего элемента
			if (pt.y >= yOffset && pt.y < yOffset + size.cy) {
				//MessageBox(hWnd, suggestion.c_str(), "Selected Suggestion", MB_OK);
				InsertSelectedSuggestion(suggestion.c_str());
				break;
			}

			// Увеличиваем смещение для следующего элемента
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
		FillRect(hdc, &rect, (HBRUSH)(COLOR_WINDOW + 1)); // Установка белого фона
		return 1;
	}

	case WM_PAINT: {
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hWnd, &ps);

		RECT rect;
		GetClientRect(hWnd, &rect);

		SetTextColor(hdc, RGB(0, 0, 0)); // Устанавливаем цвет текста
		SetBkMode(hdc, TRANSPARENT); // Прозрачный фон для текста

		DrawTextW(hdc, matchedSuggestions.c_str(), -1, &rect, DT_LEFT | DT_TOP);
		EndPaint(hWnd, &ps);
		break;
	}

				 /*
case WM_PAINT: {
	PAINTSTRUCT ps;
	HDC hdc = BeginPaint(hWnd, &ps);
	RECT rect;
	GetClientRect(hWnd, &rect);
	wchar_t text[1024]; // Используем wchar_t для поддержки широких символов
	GetWindowTextW(hWnd, text, sizeof(text) / sizeof(wchar_t)); // Не забудьте делить на размер одного wchar_tDrawText(hdc, text, -1, &rect, DT_LEFT);

	// Установка цвета текста///////////////////////////////////////
	SetTextColor(hdc, RGB(0, 0, 0));  // Черный текст
	SetBkColor(hdc, RGB(255, 255, 255));  // Белый фон
	EndPaint(hWnd, &ps);
	/////////////////////////////////////////

	EndPaint(hWnd, &ps);
	break;
}
			 */

	case WM_KEYDOWN:
		/*
		if (wParam == VK_RETURN) { // Нажатие Enter выбирает подсказку
			InsertSelectedSuggestion(suggestions[selectedSuggestion]); // Вставка выбранного слова
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
		// Обработка кликов на тексте в окне подсказок
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
		if (wParam == VK_RETURN) { // Нажатие Enter выбирает подсказку
			//InsertSelectedSuggestion(L"example"); // Вставка выбранного слова
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
	//SetThreadLocale(LCID(L"ru-RU"));
	//setlocale(LC_ALL, "ru");
	// Установка локали потока на русский
	//SetThreadLocale(MAKELCID(MAKELANGID(LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA), SORT_DEFAULT));


	if (isProcessRun(L"HalfKeyboard.exe"))
	{
		MessageBoxW(0, L"HalfKeyboard.exe уже ЗАПУЩЕН", L"Повторный запуск!", 0);
		return 1;
	}
	//MessageBoxW(0, L"Начинаем", L"Добро пожаловать!", 0);
	//
	//MyRegisterClass(hInstance);

	WNDCLASS wc;


	wc.cbClsExtra = 0;                              //Дополнительные параметры класса
	wc.cbWndExtra = 0;                              //Дополнительные параметры окна
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);    //Цвет окна
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);       //Курсор
	//wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);         //Иконка
	wc.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WINLOGO));
	wc.hInstance = hInstance;//Дискриптор приложения
	wc.lpfnWndProc = WndProc;                       //Имя ф-ии обработки сообщений
	wc.lpszClassName = L"szWindowClass";               //Имя класса окна
	wc.lpszMenuName = NULL;                         //Ссылка на главное меню
	wc.style = CS_VREDRAW | CS_HREDRAW;             //Стиль окна

	if (!RegisterClass(&wc))
	{
		MessageBoxW(NULL, L"Не удалось зарегистрировать класс окна!", L"Ошибка регистрации", MB_ICONERROR | MB_OK);
		return 1;
	}


	// Зарегистрируйте класс окна подсказок
	WNDCLASS wcSuggestions = { };
	wcSuggestions.lpfnWndProc = SuggestionsWndProc;
	wcSuggestions.hInstance = hInstance;
	wcSuggestions.lpszClassName = L"SuggestionsWndClass";

	if (!RegisterClass(&wcSuggestions))
	{
		MessageBoxW(NULL, L"Не удалось зарегистрировать класс окна подсказок!", L"Ошибка регистрации", MB_ICONERROR | MB_OK);
		return 1;
	};




	hInst = hInstance;
	hWnd = CreateWindow(L"szWindowClass", NULL, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

	if (!hWnd)
		return 1;
	/*
	// Создаем окно для подсказок
	hWndSuggestions = CreateWindowEx(
		WS_EX_TOPMOST | WS_EX_TOOLWINDOW, // Стиль для подсказок поверх окна
		"EDIT", // Класс окна (можно использовать класс для текстового поля)
		"", // Текст по умолчанию
		WS_CHILD | WS_BORDER | ES_MULTILINE | ES_READONLY, // Стиль окна
		0, 0, 150, 100, // Начальное положение и размеры
		hWnd, // Родительское окно
		NULL, // Идентификатор меню
		hInstance, // Дескриптор экземпляра приложения
		NULL // Дополнительные параметры
	);
	*/
	InitializeSuggestionWindow(hInstance);

	if (!hWndSuggestions)
		return 1;


	//ShowWindow(hWnd, nCmdShow);
	ShowWindow(hWnd, SW_HIDE);
	TrayMessageW(hWnd, NIM_ADD, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");
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
	//wstring filename = L"C:\\Users\\38301\\source\\repos\\Half-keyboard2\\Debug\\suggestions.txt"; // Измените на ваш путь


	suggestions = {
		L"зайцев",
		L"читать",
		L"текстовый",
		L"файл",
		L"программы",
		L"привет",
		L"пиво",
		L"портрет",
		L"пончик",
		L"правда",
		L"программист",
		L"помидор",
		L"парковка",
	};

	/////////////////////////////////////////////////////////////////////////////////
	MessageBoxW(0, L"Начинаем", L"Добро пожаловать!", 0);

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

	
	// Сохранение вектора в файл
	//SaveSuggestions(suggestions, filename);

	// Загрузка вектора из файла
	//перед загрузкай надо преобразовать файл в UTF-8 в Notepad++
	//wstring filename = L"suggestions.txt";
	//suggestions = LoadSuggestionsUTF8(filename);
	PostMessage(hWnd, WM_LOAD_SUGGESTIONS, 0, 0); // Отправляем сообщение окну


	input = L"прив";
	UpdateSuggestions();
	Sleep(100);

	input = L"";
	UpdateSuggestions();
	Sleep(100);

	/////////////////////////////////////////////////////////
	//PostMessage(hWnd, WM_UPDATE_DICT, 0, 0); // Отправляем сообщение окну
	//UpdateDict();
	//return 0;

	///////////////////////////////////////////////////////////

	//printf("Waiting for messages ...\n");
	while (GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	return 0;
}