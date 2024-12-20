#include <iostream>
#include <Windows.h>
#include <windows.h>
#include <stdio.h>

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
#include <algorithm>
#include <stdexcept>
#include <cstdlib> // Для использования функции atexit
#include <regex>
#include <thread>
#include <atomic>
#include <mutex>
#include <unordered_set>
#include <queue>
//#include <winuser.h>
/////////////
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <filesystem> 
#include <cstdio> // Для remove и rename
#include <locale>

#include <cwctype>  // Добавляем для iswlower и iswupper
//#include <filesystem> // Для работы с временным файлом
////////////



using namespace std;
//////////////////////////////////////

#define MYWM_NOTIFYICON (WM_USER + 1)
// Описываем сообщение, которое будет посылаться при взаимодействии юзера с нашей иконкой в систрее

HHOOK hook;
HHOOK mousehook;
//
bool IsRun = true;
bool ClampedGap = false;
bool Shift = false;
bool LControl = false;
bool RControl = false;
bool firstOccurrence = true;
ULONG Delay = 0;
atomic<bool> CaptureEnabled(true);
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
int keyPressCounter = 0; // Счётчик нажатий клавиш


const int ArrayZize = 300;
int MatchingKeys[ArrayZize];
atomic<bool> isUpdatingSuggestions(false);  // Флаг занятости
atomic<bool> isLoadingSuggestions(false);  // Флаг занятости
atomic<bool> isWordAddedToSuggestions(false);
atomic<bool> isInputProcessed(true);  // Флаг, который указывает, что ввод обработан
atomic<bool> isSaving(false); // Флаг, указывающий, что поток сохранения активен



//

atomic<bool> isLoggingEnabled(true); // Флаг для управления потоком логирования
queue<wstring> logQueue;             // Очередь для сообщений логирования
mutex logMutex;                      // Мьютекс для синхронизации доступа к очереди
condition_variable logCondition;      // Условная переменная для уведомления потока
thread logThread;                    // Поток для асинхронного логирования
thread saveThread;

//
bool LBUTTONDOWN = false;
bool RBUTTONDOWN = false;
bool MBUTTONDOWN = false;
bool lRetFalse = false;
bool rRetFalse = false;
bool mRetFalse = false;
//
// Глобальная переменная для отслеживания ввода
//atomic<wstring> input(L"");
wstring input = L"";
mutex inputMutex;

int wcount = 0;


/*  Declare Windows procedure  */
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SuggestionsWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK keyboardHook(int, WPARAM, LPARAM);

vector<wstring> suggestions;
set<wstring> uniqueSuggestions; // Для хранения уникальных предложений


int selectedSuggestion = -1; // Индекс текущего выбранного предложения

thread updateSuggestionsThread; // Поток для выполнения UpdateSuggestions
thread loadSuggestionsThread; // Поток для выполнения LoadSuggestionsUTF8

mutex updateMutex;


condition_variable cv;
mutex mtx;
bool isSymbolAdded = false;

void UpdateSuggestions();
void InitializeSuggestionWindow(HINSTANCE hInstance);
BOOL TrayMessageW(HWND hDlg, DWORD dwMessage, UINT uID, HICON hIcon, LPWSTR pszTip);



void Log(const wstring& msg) {
	// Выделяем память для копии сообщения
	wchar_t* messageCopy = new (nothrow) wchar_t[msg.size() + 1];
	if (messageCopy) { // Проверяем, успешно ли выделена память
		wcscpy_s(messageCopy, msg.size() + 1, msg.c_str());
		PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);
	}
	else {
		// Если выделение памяти не удалось, можно обработать это, например, вывести сообщение об ошибке
		OutputDebugString(L"Не удалось выделить память для логирования.\n");
	}
}

string wstring_to_string(const wstring& wstr) {
	return string(wstr.begin(), wstr.end());
}


void AsyncLogToFile(const wstring& filename) {
	// Временный файл для записи логов
	wstring tempFilename = filename + L".tmp";

	while (isLoggingEnabled || !logQueue.empty()) {
		unique_lock<mutex> lock(logMutex);
		logCondition.wait(lock, [] { return !logQueue.empty() || !isLoggingEnabled; });

		// Если есть сообщения для записи
		while (!logQueue.empty()) {
			// Получаем текущее время
			auto now = chrono::system_clock::now();
			auto in_time_t = chrono::system_clock::to_time_t(now);

			tm timeInfo;
			localtime_s(&timeInfo, &in_time_t);

			// Форматируем текущее время в строку
			wostringstream woss;
			woss << put_time(&timeInfo, L"[%Y-%m-%d %H:%M:%S] "); // Форматируем время
			wstring message = woss.str() + logQueue.front(); // Объединяем время и сообщение
			logQueue.pop();

			lock.unlock(); // Освобождаем блокировку перед записью

			// Записываем новое сообщение в временный файл
			{
				wofstream tempFile(tempFilename, ios::app);
				tempFile.imbue(locale(""));
				if (tempFile.is_open()) {
					tempFile << message << endl; // Записываем новое сообщение
				}
			}

			lock.lock(); // Блокируем снова для обработки следующего сообщения
		}
	}

	// После завершения логирования добавляем старые записи из оригинального файла в конец временного файла
	{
		wifstream originalFile(filename);
		originalFile.imbue(locale(""));
		if (originalFile.is_open()) {
			wofstream tempFile(tempFilename, ios::app); // Открываем временный файл для добавления
			tempFile.imbue(locale(""));
			wstring line;
			tempFile << L"/////////////////////////" << endl;
			while (getline(originalFile, line)) {
				tempFile << line << endl; // Записываем старые сообщения
			}

		}
	}

	// Перемещаем временный файл обратно на место оригинального
	remove(wstring_to_string(filename).c_str()); // Удаляем оригинальный файл
	rename(wstring_to_string(tempFilename).c_str(), wstring_to_string(filename).c_str()); // Переименовываем временный файл
}

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


// Асинхронное логирование: добавляет сообщение в очередь
void LogAsync(const wstring& message) {
	{
		lock_guard<mutex> lock(logMutex);
		logQueue.push(message);
	}
	logCondition.notify_one();

	SetForegroundWindow(hWndEdit);
	Sleep(100); // Немного ждем для активации окна
}

// Инициализация асинхронного логирования
void StartLogging(const wstring& filename) {
	isLoggingEnabled = true;
	logThread = thread(AsyncLogToFile, filename);
}

// Остановка асинхронного логирования и завершение потока
void StopLogging() {
	isLoggingEnabled = false;
	logCondition.notify_all();
	if (logThread.joinable()) {
		logThread.join();
	}
}

void removeLastChar() {
	//lock_guard<mutex> lock(inputMutex);
	if (!input.empty()) {
		input.pop_back();
	}
}

void appendToInput(const wstring& str) {
	//lock_guard<mutex> lock(inputMutex);
	input += str;
}

void appendToInput(wchar_t ch) {
	//lock_guard<mutex> lock(inputMutex);
	input += ch;
}

void setInput(const wstring& newValue) {
	//lock_guard<mutex> lock(inputMutex);
	input = newValue;
}

wstring getInput() {
	//lock_guard<mutex> lock(inputMutex);
	return input;
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

// Извлекает все последовательности русских букв из строки и возвращает их как одну строку
wstring extractRussianSubstrings(const wstring& line) {
	wregex russianRegex(L"[А-Яа-яЁё]+"); // Регулярное выражение для русских букв
	wstringstream result;

	auto words_begin = wsregex_iterator(line.begin(), line.end(), russianRegex);
	auto words_end = wsregex_iterator();

	for (auto it = words_begin; it != words_end; ++it) {
		result << it->str(); // Добавляем найденные подстроки в итоговый результат
	}

	return result.str(); // Конкатенированные русские подстроки
}

// Извлекает первую последовательность русских букв из строки
wstring extractFirstRussianSubstring(const wstring& line) {
	wregex russianRegex(L"[А-Яа-яЁё]+"); // Регулярное выражение для русских букв
	wsmatch match;

	// Поиск первой подстроки, соответствующей регулярному выражению
	if (regex_search(line, match, russianRegex)) {
		if (match.str().length() >= 5) { // Проверка длины подстроки
			return match.str(); // Возвращаем первую подходящую подстроку
		}
	}

	return L""; // Если подстрока не найдена или не соответствует условиям
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
	/*
	set<wstring> uniqueLines; // Множество для хранения уникальных строк
	wstring line;

	while (getline(inputFile, line)) {
		uniqueLines.insert(line); // Добавляем строку в множество (дубликаты не сохраняются)
	}

	vector<wstring> vectorUniqueLines = vector<wstring>(uniqueLines.begin(), uniqueLines.end()); // Преобразуем множество в вектор

	for (const auto& line : vectorUniqueLines) {
		outputFile << line << endl; // Записываем уникальные строки в файл
	}


	*/
	//////-=2=-/////////////////////////////////////////////////////////////////////////

	//////-=3=-/////////////////////////////////////////////////////////////////////////
	/*
	wstring line;
	while (getline(inputFile, line)) {
		wstring russianSubstring = extractFirstRussianSubstring(line);
		if (!russianSubstring.empty()) {
			outputFile << russianSubstring << endl; // Записываем только первую подстроку, если она найдена
		}
	}
	*/
	//////-=3=-/////////////////////////////////////////////////////////////////////////

	//////-=4=-/////////////////////////////////////////////////////////////////////////
	/*
	wstring line;
	wstring wstringline = L"";
	while (getline(inputFile, line)) {
		wstringline += line;
		if (wstringline.find(line) >= 0) {
			outputFile << line << endl; // Записываем только первую подстроку, если она найдена
		}
	}
	*/
	//////-=4=-/////////////////////////////////////////////////////////////////////////
	outputFile.close();
	inputFile.close(); // Закрытие файла после чтения
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
	if (text.empty()) return;

	INPUT inputs[2];
	ZeroMemory(inputs, sizeof(inputs));

	//wstring logMessage = L"**************************************";
	//Log(logMessage);

	for (wchar_t ch : text) {

		// Ждем, пока предыдущий ввод не будет обработан
		while (!isInputProcessed) {
			this_thread::sleep_for(chrono::milliseconds(10));
		}
		// Сбрасываем флаг перед отправкой следующего символа
		isInputProcessed = false;

		// Настройка нажатия клавиши
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = 0;
		inputs[0].ki.wScan = ch;
		inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;

		// Настройка отпускания клавиши
		inputs[1] = inputs[0];
		inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

		SendInput(2, inputs, sizeof(INPUT));

		//Sleep(200);

		// Здесь мы можем предположить, что система обработала ввод, и сразу устанавливаем флаг
		isInputProcessed = true;

		// Дополнительная задержка между вводами
		this_thread::sleep_for(chrono::milliseconds(50));

	}

	//logMessage = L"--------------------------------------------------";
	//Log(logMessage);

}

/*
void SendString(const wstring& text) {
	if (text.empty()) return; // Проверка на пустую строку


	INPUT inputs[2]; // Для каждого символа нужны два INPUT (нажатие и отпускание клавиши)
	ZeroMemory(inputs, sizeof(inputs));

	for (wchar_t ch : text) {
		// Настройка нажатия клавиши (KEYDOWN)
		inputs[0].type = INPUT_KEYBOARD;
		inputs[0].ki.wVk = 0; // Не используется для символов Unicode
		inputs[0].ki.wScan = ch;
		inputs[0].ki.dwFlags = KEYEVENTF_UNICODE;
		//Sleep(100);

		// Настройка отпускания клавиши (KEYUP)
		inputs[1] = inputs[0];
		inputs[1].ki.dwFlags = KEYEVENTF_UNICODE | KEYEVENTF_KEYUP;

		// Отправляем нажатие и отпускание
		SendInput(2, inputs, sizeof(INPUT));
		//Sleep(100);

	}
}
*/
/*
void SendString(const wstring& text) {
	if (text.empty()) return; // Проверка на пустую строку

	INPUT inputs[2]; // Для каждого символа нужны два INPUT (нажатие и отпускание клавиши)
	ZeroMemory(inputs, sizeof(inputs));

	for (wchar_t ch : text) {
		// Настройка нажатия клавиши (KEYDOWN)
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
*/

wstring stringToWstring(const string& str) {
	int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
	wstring wstr(size_needed, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
	return wstr;
}

wstring ReplaceNewline(const wstring& word, const wstring& toReplace) {

	wstring result = word; // Копируем исходную строку в новую переменную
	size_t pos = 0;

	// Находим все вхождения L"\n" и заменяем их на пустую строку
	while ((pos = result.find(toReplace, pos)) != wstring::npos) {
		result.erase(pos, toReplace.length()); // Удаляем L"\n"
	}

	return result; // Возвращаем новую строку без символов L"\n"
}

void SaveSuggestions(const wstring& filename) {
	if (!isLoadingSuggestions) {
		isSaving = false;
		return;
	}

	// Создаем временное имя файла
	wstring tempFilename = filename + L".tmp";

	// Открываем временный файл для записи
	wofstream tempFile(tempFilename);
	tempFile.imbue(locale(tempFile.getloc(), new codecvt_utf8<wchar_t>)); // Устанавливаем кодировку UTF-8 без BOM

	if (!tempFile.is_open()) {
		return;
	}

	//CaptureEnabled = false;
	//TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), L"Half keyboard\n(Сохранение словаря...)");

	unordered_set<wstring> uniqueSuggestions;
	for (const wstring& suggestion : suggestions) {
		wstring modifiedText = ReplaceNewline(suggestion, L"\n");
		modifiedText = ReplaceNewline(modifiedText, L"\r");
		if (uniqueSuggestions.find(modifiedText) == uniqueSuggestions.end()) {  // Если строка еще не встречалась
			tempFile << modifiedText << L"\n";  // Записываем уникальную строку
			uniqueSuggestions.insert(modifiedText);  // Добавляем в набор уникальных строк
		}
	}

	tempFile.close();

	// Удаление старого файла и замена временного файла на основной
	if (remove(wstring_to_string(filename).c_str()) != 0) {
		// Обработка ошибки при удалении оригинального файла		


		/*wstring logMessage = L"Ошибка при удалении файла: " + filename + L"\n";// строка для логирования
		wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
		wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
		PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);
		delete[] messageCopy;
		*/
	}
	if (rename(wstring_to_string(tempFilename).c_str(), wstring_to_string(filename).c_str()) != 0) {
		// Обработка ошибки при переименовании временного файла
		//wcerr << L"Ошибка при переименовании файла: " << tempFilename << L" в " << filename << endl;


		/*wstring logMessage = L"Ошибка при переименовании файла: " + tempFilename + L" в " + filename + L"\n";// строка для логирования
		wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
		wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
		PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);
		delete[] messageCopy;
		*/
	}


	//CaptureEnabled = true;
	//TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");

	Log(L"Suggestions saved.\n");
	isSaving = false;
}

/*
void SaveSuggestions(const wstring& filename) {


	if (!isLoadingSuggestions) {
		return;
	}

	wofstream file(filename);
	file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>)); // Устанавливаем кодировку UTF-8 без BOM

	if (!file.is_open()) {
		return;
	}

	CaptureEnabled = false;
	TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), L"Half keyboard\n(Сохранение словаря...)");


	unordered_set<wstring> uniqueSuggestions;
	for (const wstring& suggestion : suggestions) {
		wstring modifiedText = ReplaceNewline(suggestion, L"\n");
		modifiedText = ReplaceNewline(modifiedText, L"\r");
		if (uniqueSuggestions.find(modifiedText) == uniqueSuggestions.end()) {  // Если строка еще не встречалась
			file << modifiedText << L"\n";  // Записываем уникальную строку
			uniqueSuggestions.insert(modifiedText);  // Добавляем в набор уникальных строк
		}
	}

	file.close();

	CaptureEnabled = true;
	TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");

}
*/

void LoadSuggestionsUTF8(const wstring& filename) {

	//CaptureEnabled = false;
	//TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON2)), L"Half keyboard\n(Загрузка словаря...)");

	wifstream file(filename, ios::binary);
	file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>)); // UTF-8 кодировка

	if (!file) {
		isLoadingSuggestions = true;
		//SaveSuggestions(filenameSuggestions);
		//throw runtime_error("Не удалось открыть файл");
		return;
	}

	wstring line;
	wstring message = L"";
	//Log(L"begin while", logFilename);
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
	//Log(message, logFilename);
	file.close();
	isLoadingSuggestions = true;

	CaptureEnabled = true;
	TrayMessageW(hWnd, NIM_MODIFY, 0, LoadIcon(hInst, MAKEINTRESOURCE(IDI_ICON1)), L"Half keyboard\n(Захват включен)");
}


// Функция для подстановки выбранного слова
void InsertSelectedSuggestion(const wstring& word) {


	wstring modifiedText = L"";
	// Добавление в начало
	if (word.length() > 1)
	{
		modifiedText = ReplaceNewline(word, L"\n");
		modifiedText = ReplaceNewline(modifiedText, L"\r");
		//suggestions.insert(suggestions.begin(), modifiedText +L"\n");
		//suggestions.insert(suggestions.begin(), modifiedText);
		//isWordAddedToSuggestions = true;
	}
	else
	{
		return;
	}


	SetForegroundWindow(hWndEdit);
	Sleep(100); // Немного ждем для активации окна

	// Создаем копию word для изменения
	wstring modifiedWord = modifiedText;

	// Удаляем input из modifiedWord, если modifiedWord начинается с input
	if (modifiedWord.find(getInput()) == 0) { // Проверяем, начинается ли modifiedWord с input
		modifiedWord.erase(0, getInput().length()); // Удаляем input из начала modifiedWord
	}

	// Подстановка текста в поле ввода
	SendString(modifiedWord);

	setInput(modifiedText);

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
	int count = 0; // Счётчик добавленных элементов
	matchedSuggestions.clear();
	unordered_set<wstring> seenSuggestions;


	unordered_set<wstring> uniqueSuggestions;
	for (const wstring& suggestion : suggestions) {
		wstring modifiedText = ReplaceNewline(suggestion, L"\n");
		modifiedText = ReplaceNewline(modifiedText, L"\r");
		if (suggestion.find(getInput()) == 0 && uniqueSuggestions.find(modifiedText) == uniqueSuggestions.end()) {  // Если строка еще не встречалась
			matchedSuggestions += (modifiedText + L"\n"); // Добавляем уникальное предложение в строку
			uniqueSuggestions.insert(modifiedText);  // Добавляем в набор уникальных строк

			++count;
			if (count == 5) { // Останавливаемся на пятом элементе
				break;
			}
		}
	}
	/*
	for (const wstring& suggestion : suggestions) {
		// Проверяем, начинается ли предложение с введенного текста, не пусто ли оно и уникально ли оно
		if (suggestion.find(getInput()) == 0 && !getInput().empty() && seenSuggestions.insert(suggestion).second) {
			wstring modifiedText = ReplaceNewline(suggestion, L"\r");
			//modifiedText = ReplaceNewline(modifiedText, L"\n");

			matchedSuggestions += (modifiedText + L"\n"); // Добавляем уникальное предложение в строку

			++count;
			if (count == 5) { // Останавливаемся на пятом элементе
				break;
			}

		}
	}
	*/

	/*
	uniqueSuggestions.clear();
	int count = 0; // Счётчик добавленных элементов
	//for (const auto& suggestion : suggestions) {
	for (const wstring& suggestion : suggestions) {


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
		matchedSuggestions += suggestion + L"\n"; // Формируем строку
	}
	*/

	if (matchedSuggestions.empty() || getInput().empty()) {
		if (IsWindowVisible(hWndSuggestions)) {
			SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
		}
		//SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
		///input.clear();
	}
	else {
		SetWindowPos(hWndSuggestions, HWND_TOP, point.x, point.y + 20, 150, 100, SWP_SHOWWINDOW);
		matchedSuggestions.pop_back();
		UpdateSuggestionsWindow(hWndSuggestions, matchedSuggestions);
	}

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


void UpdateSuggestionsAsync() {
	// Блокируем мьютекс для проверки и установки флага
	lock_guard<mutex> lock(updateMutex);
	if (isUpdatingSuggestions) {
		// Если процесс обновления уже выполняется, выходим
		return;
	}

	// Устанавливаем флаг, что процесс обновления начался
	isUpdatingSuggestions = true;

	// Запускаем обновление в новом потоке
	thread([]() {
		UpdateSuggestions();  // Выполнение функции

		// После завершения сбрасываем флаг
		lock_guard<mutex> lock(updateMutex);
		isUpdatingSuggestions = false;
		}).detach();  // Отсоединяем поток
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

wstring GetKeyPressed(UINT vkCode) {
	// Получаем состояние клавиш
	BYTE keyboardState[256];
	GetKeyboardState(keyboardState);

	// Определяем, какой модификатор нажат (если Shift нажат, устанавливаем флаг)
	if (GetAsyncKeyState(VK_SHIFT)) {
		keyboardState[VK_SHIFT] |= 0x80; // Устанавливаем бит, если Shift нажат
	}

	// Буфер для хранения символа
	WCHAR buffer[10];
	int result = ToUnicodeEx(vkCode, 0, keyboardState, buffer, sizeof(buffer) / sizeof(buffer[0]), 0, GetKeyboardLayout(0));

	if (result > 0) {
		buffer[result] = 0; // Завершаем строку
		return wstring(buffer);
	}

	return L""; // Если символ не получен
}


wstring RemoveDuplicateCaseLetters(wstring& wInput) {

	//input = L"aaaA bbcC dDeF gghH iiJk"; // Пример строки
	//wstring input1 = L"пПрттТтт";
	//wstring input = L"пПрттТтт"; // Пример строки
	//input = L"пПрттТтт";
	if (wInput.empty())
	{
		return wInput;
	}

	for (int i = 0; i < wInput.length() - 1; ++i) {
		wchar_t currentChar = wInput[i];
		wchar_t nextChar = wInput[i + 1];


		// Проверка: текущий символ — маленькая буква, следующий — такая же заглавная
		if (towlower(currentChar) == towlower((nextChar + 32))) {
			if (iswlower(currentChar) && iswupper(nextChar)) {

				wInput.erase(i, 1); // Удаляем маленькую букву
				--i; // Уменьшаем индекс, чтобы не пропустить следующий символ
			}
		}
	}
	return wInput;
	/*
	// Итерируемся с конца строки, чтобы избежать проблем с индексами
	for (int i = input.size() - 2; i >= 0; --i) {
		wchar_t currentChar = input[i];
		wchar_t nextChar = input[i + 1];

		if (iswlower(currentChar) && iswupper(nextChar)) {
			//wcout << L"Прошло проверку на регистр: " << currentChar << L", " << nextChar << endl;
			wstring logMessage = L"Прошло проверку на регистр " + input; // Ваша строка для логирования
			wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
			wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
			PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);


			if (iswlower(currentChar) && iswupper(nextChar) && (currentChar == nextChar + 32)) {
				//wcout << L"Прошло проверку на совпадение символов: " << currentChar << L", " << nextChar << endl;
				wstring logMessage = L"Прошло проверку на совпадение символов " + input; // Ваша строка для логирования
				wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
				wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
				PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);

				input.erase(i, 1);
			}
		}
	}
	*/

	/*
	for (size_t i = 0; i < input.length() - 1; ) {
		wchar_t currentChar = input[i];
		wchar_t nextChar = input[i + 1];

		// Проверка на соответствие по регистру:
		bool isCurrentLower = (currentChar >= L'а' && currentChar <= L'я') || (currentChar >= L'a' && currentChar <= L'z');
		bool isNextUpper = (nextChar >= L'А' && nextChar <= L'Я') || (nextChar >= L'A' && nextChar <= L'Z');

		// Проверка: текущий символ — маленькая буква, следующий — такая же заглавная
		if ((towlower(currentChar) == towlower(nextChar)) && isCurrentLower && isNextUpper) {
			input.erase(i, 1); // Удаляем маленькую букву
		}
		else {

		}
		++i; // Переходим к следующему символу
	}
	*/

	/*
	wstring logMessage = L"после4 " + input; // Ваша строка для логирования
	wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
	wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
	PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);
	*/

	/*
	for (size_t i = 0; i < input.length() - 1; ) {
		wchar_t currentChar = input[i];
		wchar_t nextChar = input[i + 1];

		// Проверка: текущий символ — маленькая буква, следующий — такая же заглавная
		if (towlower(currentChar) == towlower(nextChar) && iswlower(currentChar) && iswupper(nextChar)) {
			input.erase(i, 1); // Удаляем маленькую букву
		}
		else {

		}
		i++; // Переходим к следующему символу
	}
	*/
}

wstring GetVkInfoForChar(wchar_t ch) {
	SHORT result = VkKeyScanW(ch);

	// Проверяем, успешен ли результат (если вернул -1, значит символ не может быть набран)
	if (result == -1) {
		return L"Символ не может быть набран с текущей раскладкой клавиатуры.";
	}

	// Извлекаем виртуальный код и модификаторы
	BYTE vkCode = LOBYTE(result); // Виртуальный код клавиши
	BYTE shiftState = HIBYTE(result); // Состояние модификаторов (Shift, Ctrl, Alt)

	wstring logMessage = L"Для символа '";
	logMessage += ch;
	logMessage += L"': VkCode = ";
	logMessage += to_wstring(vkCode);

	// Проверяем состояние модификаторов
	if (shiftState & 1) logMessage += L", Shift";
	if (shiftState & 2) logMessage += L", Ctrl";
	if (shiftState & 4) logMessage += L", Alt";

	return logMessage;
}


std::wstring GetCharFromVkCode(UINT vkCode) {
	BYTE keyboardState[256];
	WCHAR buffer[2] = { 0 };

	// Получаем состояние клавиш
	if (GetKeyboardState(keyboardState)) {
		// Логируем состояние клавиши
		//wstring logMessage = L"keyboardState[VK_CODE]: " + std::to_wstring(keyboardState[vkCode]);
		//Log(logMessage);

		// Получаем окно в фокусе
		HWND hWnd = GetForegroundWindow();
		if (hWnd == NULL) return false;

		// Получаем идентификатор потока для окна в фокусе
		DWORD threadId = GetWindowThreadProcessId(hWnd, NULL);

		// Получаем раскладку клавиатуры для этого потока
		HKL keyboardLayout = GetKeyboardLayout(threadId);

		//HKL keyboardLayout = GetKeyboardLayout(0); // Получаем текущую раскладку клавиатуры

		// Логируем перед вызовом ToUnicodeEx
		//logMessage = L"Calling ToUnicodeEx with vkCode: " + std::to_wstring(vkCode);
		//Log(logMessage);

		int result = ToUnicodeEx(vkCode, MapVirtualKey(vkCode, 0), keyboardState, buffer, 2, 0, keyboardLayout);

		// Логирование результата
		//logMessage = L"ToUnicodeEx result: " + std::to_wstring(result) + L", wstring(1, buffer[0]): " + wstring(1, buffer[0]);
		//Log(logMessage);

		if (result == 1) {
			return wstring(1, buffer[0]); // Возвращаем символ
		}
	}

	return L""; // Возвращаем пустую строку, если не удалось получить символ
}

LRESULT CALLBACK keyboardHook(int nCode, WPARAM wParam, LPARAM lParam)
{
	KBDLLHOOKSTRUCT* p = reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam);
	DWORD newVkCode;
	INPUT inputs[2] = { 0 };
	UINT ret;
	//wstring keySymbol = L"";

	char wParamStr[16];
	char vkStr[16] = "";
	//bool isRepeated = (lParam & (1 << 30)) != 0;

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

	//Log(L"wParam " + to_wstring(wParam), logFilename);
	/*
		wstring logMessage = L"p->vkCode " + to_wstring(p->vkCode) + L"wParam " + to_wstring(wParam);// строка для логирования
		wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
		wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
		PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);
		//delete[] messageCopy;
	*/

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
		PostMessage(GetForegroundWindow(), WM_INPUTLANGCHANGEREQUEST, 0, 0);
		PostMessage(hWnd, WM_INPUTLANGCHANGEREQUEST, 0, 0);
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
					//Log(extractRussianSubstrings(input.c_str()) + L"*", logFilename);
					// Отправка строки в окно для логирования
					/*
					wstring logMessage = input; // Ваша строка для логирования
					wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
					wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
					PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);
					*/
					//LogAsync(input);
					// Добавление в начало

					if (getInput().length() > 1)
					{
						suggestions.insert(suggestions.begin(), getInput());
					}
					setInput(L"");

					SetForegroundWindow(hWndEdit);
					//Sleep(100); // Немного ждем для активации окна

					ret = SendInput(1, inputs, sizeof(INPUT));
				}
				Delay = 0;
				return 1;
			}
		}

		// Alt		
		if ((p->vkCode == VK_RMENU) && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
		{
			return 1; // Останавливаем дальнейшую обработку Alt, чтобы он не повлиял на другое поведение
		}
		if ((p->vkCode == VK_RMENU) && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP)) {  // VK_MENU - код клавиши Alt
			if (!matchedSuggestions.empty()) {
				// Вставляем первое предложение
				wstring firstSuggestion = split(matchedSuggestions, L"\n")[0];
				InsertSelectedSuggestion(firstSuggestion);
				//Sleep(100);				
			}
			return 1; // Останавливаем дальнейшую обработку Alt, чтобы он не повлиял на другое поведение
		}



		//Enter
		if ((p->vkCode == VK_RETURN) && (wParam == WM_KEYUP || wParam == WM_SYSKEYUP))
		{
			if (IsWindowVisible(hWndSuggestions)) {
				SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
			}
			SetForegroundWindow(hWndEdit);
			//Log(extractRussianSubstrings(input.c_str()), logFilename);
			//LogAsync(input);

			if (getInput().length() > 1)
			{
				suggestions.insert(suggestions.begin(), getInput());
			}
			setInput(L"");


			//return 0;
		}

		//Shift
		if ((p->vkCode == 161 || p->vkCode == 160) && wParam == WM_KEYDOWN) {
			Shift = !Shift;
		}

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
		if ((p->vkCode == VK_CAPITAL && (p->flags & LLKHF_INJECTED) == 0) && (wParam == WM_KEYDOWN) && !ClampedGap) {
			if (Shift)
			{

				inputs[0].ki.wVk = VK_CAPITAL;
				inputs[0].type = INPUT_KEYBOARD;			
				inputs[0].ki.dwFlags = 0;

				// Отпускание клавиши VK_CAPITAL
				inputs[1] = inputs[0]; // Копируем структуру для отпускания
				inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

				// Отправляем оба события за один вызов
				ret = SendInput(2, inputs, sizeof(INPUT));
				Shift = false;
				return 1;
			}
			else if (wParam == WM_KEYDOWN)
			{
				
				// Нажатие клавиши Enter
				inputs[0].type = INPUT_KEYBOARD;
				inputs[0].ki.wVk = VK_RETURN;
				inputs[0].ki.dwFlags = 0;

				// Отпускание клавиши Enter
				inputs[1] = inputs[0]; // Копируем структуру для отпускания
				inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;

				// Отправляем оба события за один вызов
				ret = SendInput(2, inputs, sizeof(INPUT));
				/*
				inputs[0].ki.wVk = VK_RETURN;
				inputs[0].ki.dwFlags = 0;
				
				ret = SendInput(1, inputs, sizeof(INPUT));
				inputs[0].ki.dwFlags = KEYEVENTF_KEYUP;
				ret = SendInput(1, inputs, sizeof(INPUT));
				*/

				
				if (IsWindowVisible(hWndSuggestions)) {
					SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
				}
				
				if (getInput().length() > 1)
				{
					suggestions.insert(suggestions.begin(), getInput());
				}
				setInput(L"");
				

				return 1;
			}
		}

		//Exit
		if (p->vkCode == 27 && (p->flags & LLKHF_INJECTED) == 0 && (wParam == WM_KEYDOWN) && ClampedGap) {
			//SaveSuggestions(filenameSuggestions);
			IsRun = false;

			/*
			UnhookWindowsHookEx;
			UnhookWindowsHookEx;
			MessageBoxW(0, L"Закончили", L"Всего хорошего!!!", 65536);//MsgBoxSetForeground
			TrayMessageW(hWnd, NIM_DELETE, 0, 0, 0);
			exit(0);
			*/
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
					if (wParam == WM_KEYDOWN)
					{
						wstring keySymbol = GetKeyPressed(MatchingKeys[p->vkCode]);
						//input += keySymbol;
						appendToInput(keySymbol);
						RemoveDuplicateCaseLetters(input);
					}

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
				/* Закомментировал потому-что надо было ставить на 
				* паузу чтоб нажать контрл+А левой рукой
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
				*///else
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

			if (wParam == WM_KEYDOWN) {
				GetCursorPos(&point);

				HWND hWndEditA = GetForegroundWindow();

				if (hWndEdit != hWndEditA && hWndEditA != hWndSuggestions) {
					hWndEdit = hWndEditA; // Обновляем активное окно, если это не окно с подсказками

					if (IsWindowVisible(hWndSuggestions)) {
						SetWindowPos(hWndSuggestions, HWND_TOP, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, SWP_HIDEWINDOW);
					}

					SetForegroundWindow(hWndEdit);
					//Sleep(100); // Немного ждем для активации окна
				}


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


			if (p->vkCode == VK_BACK && !getInput().empty() && wParam == WM_KEYDOWN) {
				//Loga((L"do " + input), logFilename);

				//Loga(L"posle " + input, logFilename);
				//////input.pop_back(); 
				removeLastChar(); // Удаляем последний символ при нажатии Backspace
				// Обновляем текстовое поле, чтобы отобразить изменения
				//UpdateTextField(input); // Ваша функция обновления текстового поля
				//int m = 1;
				setInput(RemoveDuplicateCaseLetters(getInput()));

			}
			else if (wParam == WM_KEYDOWN && !(p->vkCode == VK_BACK)) {

				if (!Shift) {
					appendToInput(GetCharFromVkCode(p->vkCode));									
				}


				if (Shift && !ClampedGap)
				{
					wstring keySymbol = GetKeyPressed(p->vkCode);
					//input += keySymbol;
					appendToInput(keySymbol);
					/////////////////RemoveDuplicateCaseLetters(input);
					setInput(RemoveDuplicateCaseLetters(getInput()));
				}
				//RemoveDuplicateCaseLetters(input);
				//if (!isRepeated)
				{
					PostMessage(hWnd, WM_UPDATE_SUGGESTIONS, wParam, 0); // Отправляем сообщение окну
				}
				

				//}
				//Loga((L"keySymbol " + keySymbol), logFilename);
				/*
				if (keySymbol != L"")
				{
					input += keySymbol;
					//keySymbol = L"";
				}
				*/


				//}
				/*
				else {
					// Ошибка при получении состояния клавиатуры
					debugMsg = L"Не удалось получить состояние клавиатуры.";
					OutputDebugString(debugMsg.c_str());
					//wstring logMessage = debugMsg; // Ваша строка для логирования
					//wchar_t* messageCopy = new wchar_t[logMessage.size() + 1];
					//wcscpy_s(messageCopy, logMessage.size() + 1, logMessage.c_str());
					//PostMessageW(hWnd, WM_WRITE_LOG, reinterpret_cast<WPARAM>(messageCopy), 0);

					//Log(debugMsg.c_str(), logFilename);0
					//Log(debugMsg.c_str())
				}
				*/

			}



			/*
			if (input.length() > 1 && input.back() == L' ')
			{
				input.pop_back(); // Удаляем последний символ
				suggestions.insert(suggestions.begin(), input);
				input.clear();
			}
			*/


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
			//if (IsRussianKeyboardLayout()) {
				//UpdateSuggestions();
				//Log(input.c_str(), logFilename);
			////////////////////PostMessage(hWnd, WM_UPDATE_SUGGESTIONS, wParam, 0); // Отправляем сообщение окну
			//}
			/*
			if (!firstOccurrence)
			{

			}
			firstOccurrence = false;
			*/

		}


		/*
		if (nCode >= 0 && wParam == WM_KEYUP) {
			// Ваша логика обработки клавиши
			// ...

			// Установка isSymbolAdded в true и уведомление SendString
			{
				lock_guard<mutex> lock(mtx);
				isSymbolAdded = true;
			}
			cv.notify_one();
		}
		*/

	}

	//return CallNextHookEx(NULL, nCode, wParam, lParam);
	return 0;


}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	//HWND AppWnd;
	///
	//KBDLLHOOKSTRUCT *p = reinterpret_cast<KBDLLHOOKSTRUCT *>(lParam);

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

	// Обработка WM_UPDATE_SUGGESTIONS в отдельном потоке
	if (uMsg == WM_UPDATE_SUGGESTIONS)
	{
		UpdateSuggestionsAsync(); // Запускаем функцию в новом потоке
		/*
		// Если поток уже работает, дожидаемся его завершения
		if (updateSuggestionsThread.joinable()) {
			updateSuggestionsThread.join();
		}

		// Запускаем UpdateSuggestions в новом потоке
		updateSuggestionsThread = thread([]() {
			UpdateSuggestions();
			});
		*/
	}

	// Обработка WM_LOAD_SUGGESTIONS в отдельном потоке
	if (uMsg == WM_LOAD_SUGGESTIONS)
	{
		if (loadSuggestionsThread.joinable()) {
			loadSuggestionsThread.join();
		}

		loadSuggestionsThread = thread([]() {
			LoadSuggestionsUTF8(filenameSuggestions);
			});
	}

	if (uMsg == WM_WRITE_LOG)
	{
		// Преобразуем wParam обратно в указатель на wchar_t
		const wchar_t* message = reinterpret_cast<const wchar_t*>(wParam);
		LogAsync(message); // Асинхронное логирование
		delete[] message; // Освобождаем память после логирования
	}


	switch (uMsg)
	case WM_QUERYENDSESSION:
	{
		if (!isSaving) {
			// Запускаем поток сохранения, если он еще не запущен
			isSaving = true;
			saveThread = thread([]() {
				SaveSuggestions(L"suggestions.txt");
				});

			// Блокируем завершение работы системы
			ShutdownBlockReasonCreate(hwnd, L"Saving suggestions...");
		}
		return TRUE; // Разрешаем завершение работы (отложится до WM_ENDSESSION)

	case WM_ENDSESSION:
		if (wParam && isSaving) {
			// Ждем завершения потока
			if (saveThread.joinable()) {
				saveThread.join();
			}

			// Убираем блокировку завершения
			ShutdownBlockReasonDestroy(hwnd);
		}
		break;

	
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
			//SaveSuggestions(filenameSuggestions);
			IsRun = false;
			/*
			DestroyWindow(hWnd);
			DestroyWindow(hWndSuggestions);
			UnhookWindowsHookEx;
			UnhookWindowsHookEx;
			MessageBoxW(0, L"Закончили", L"Всего хорошего!!!", 65536);//MsgBoxSetForeground
			TrayMessageW(hWnd, NIM_DELETE, 0, 0, 0);
			exit(0);
			*/
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

		for (const auto& suggestion : split(matchedSuggestions, L"\n")) {
			// Получаем размер текущей строки
			GetTextExtentPoint32W(hdc, suggestion.c_str(), suggestion.length(), &size);

			// Проверяем, находится ли клик в границах текущего элемента
			if (pt.y >= yOffset && pt.y < yOffset + size.cy) {
				//MessageBox(hWnd, suggestion.c_str(), "Selected Suggestion", MB_OK);

				if (IsWindowVisible(hWndSuggestions)) {
					InsertSelectedSuggestion(suggestion.c_str());
				}
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
		/////////////PostQuitMessage(0);
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

	// Запуск логирования
	StartLogging(logFilename);


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
		//L"Привет",
		//L"привет",
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


	/*
	input = L"прив";
	UpdateSuggestions();
	Sleep(100);

	input = L"";
	UpdateSuggestions();
	Sleep(100);
	*/

	/////////////////////////////////////////////////////////
	//PostMessage(hWnd, WM_UPDATE_DICT, 0, 0); // Отправляем сообщение окну
	//UpdateDict();
	//return 0;

	///////////////////////////////////////////////////////////

	//printf("Waiting for messages ...\n");
	while (IsRun && GetMessage(&messages, NULL, 0, 0)) {
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}

	// Не забывайте удалять хук перед завершением
	UnhookWindowsHookEx(hook);

	thread saveThread([]() {
		SaveSuggestions(filenameSuggestions);
		});
	// Остановка логирования перед завершением программы
	StopLogging();

	DestroyWindow(hWnd);
	DestroyWindow(hWndSuggestions);
	UnhookWindowsHookEx;
	UnhookWindowsHookEx;
	MessageBoxW(0, L"Закончили", L"Всего хорошего!!!", 65536);//MsgBoxSetForeground
	TrayMessageW(hWnd, NIM_DELETE, 0, 0, 0);

	// Ожидание завершения потока
	if (saveThread.joinable()) {
		saveThread.join();
	}

	exit(0);

	return 0;
}