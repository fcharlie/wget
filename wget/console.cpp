#include "stdafx.h"


int BaseErrorWriteConhost(const wchar_t *buf, size_t len) {
	// TO set Foreground color
	HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	WORD oldColor = csbi.wAttributes;
	WORD newColor = (oldColor & 0xF0) | FOREGROUND_INTENSITY | FOREGROUND_RED;
	SetConsoleTextAttribute(hConsole, newColor);
	DWORD dwWrite;
	WriteConsoleW(hConsole, buf, len, &dwWrite, nullptr);
	SetConsoleTextAttribute(hConsole, oldColor);
	return dwWrite;
}

int BaseErrorMessagePrint(const wchar_t *format, ...) {
	wchar_t buf[16348];
	va_list ap;
	va_start(ap, format);
	auto l = _vswprintf_c_l(buf, 16348, format, nullptr,ap);
	va_end(ap);
	return BaseErrorWriteConhost(buf, l);
}