; GetCaretPos.ahk - Получение координат каретки и запись их в файл
#Persistent
SetTitleMatchMode, 2
SetWinDelay, -1

CoordMode, Caret, Screen
WinGet, id, ID, ahk_class Notepad

Loop {
    if WinExist("ahk_id " id) {
        CoordMode, Caret, Screen
        MouseGetPos, x, y
        FileAppend, X: %x% - Y: %y%`n, C:\Users\38301\source\repos\Half-keyboard2\Debug\caret_position.txt

        ExitApp
    }
    Sleep, 100
