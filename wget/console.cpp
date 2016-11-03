#include "stdafx.h"
#include "console.h"

int BaseWriteConhostEx(HANDLE hConsole, WORD color, const wchar_t *buf, size_t len) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	WORD oldColor = csbi.wAttributes;
	SetConsoleTextAttribute(hConsole, color);
	DWORD dwWrite;
	WriteConsoleW(hConsole, buf, len, &dwWrite, nullptr);
	SetConsoleTextAttribute(hConsole, oldColor);
	return dwWrite;
}
int BaseMessagePrintEx(WORD color, const wchar_t *format, ...) {
	HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
	wchar_t buf[16348];
	va_list ap;
	va_start(ap, format);
	auto l = _vswprintf_c_l(buf, 16348, format, nullptr, ap);
	va_end(ap);
	return BaseWriteConhostEx(hConsole, color, buf, l);
}

int BaseWriteConhost(HANDLE hConsole, WORD fcolor,const wchar_t *buf, size_t len) {
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	WORD oldColor = csbi.wAttributes;
	WORD newColor = (oldColor & 0xF0) |fcolor;
	SetConsoleTextAttribute(hConsole, newColor);
	DWORD dwWrite;
	WriteConsoleW(hConsole, buf, len, &dwWrite, nullptr);
	SetConsoleTextAttribute(hConsole, oldColor);
	return dwWrite;
}

int BaseErrorMessagePrint(const wchar_t *format, ...) {
	HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
	wchar_t buf[16348];
	va_list ap;
	va_start(ap, format);
	auto l = _vswprintf_c_l(buf, 16348, format, nullptr,ap);
	va_end(ap);
	return BaseWriteConhost(hConsole,console::fc::LightRed,buf, l);
}

int BaseMessagePrint(WORD color,const wchar_t *format, ...) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	wchar_t buf[16348];
	va_list ap;
	va_start(ap, format);
	auto l = _vswprintf_c_l(buf, 16348, format, nullptr, ap);
	va_end(ap);
	return BaseWriteConhost(hConsole,color,buf, l);
}