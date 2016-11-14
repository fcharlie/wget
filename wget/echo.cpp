///
#include "stdafx.h"

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctype.h>
#include <codecvt>
#include <io.h>

struct StringBuffer {
	const wchar_t *begin;
	size_t length;
	WORD hcolor;
};

static int hextobin(wchar_t c) {
	switch (c) {
	default:
		return c - '0';
	case 'a':
	case 'A':
		return 10;
	case 'b':
	case 'B':
		return 11;
	case 'c':
	case 'C':
		return 12;
	case 'd':
	case 'D':
		return 13;
	case 'e':
	case 'E':
		return 14;
	case 'f':
	case 'F':
		return 15;
	}
}

bool LineToBin(const wchar_t *cstr, std::wstring &line) {
	assert(cstr);
	auto len = wcslen(cstr);
	line.reserve(len + 1);
	/// Do Parse....
	auto iter = cstr;
	wchar_t c;
	while ((c = *iter++)) {
		if (c == '\\' && *iter) {
			switch (c = *iter++) {
			case 'a':
				c = '\a';
				break;
			case 'b':
				c = '\b';
				break;
			case 'c':
				return true;
			case 'e':
				c = 0x1B;
				break;
			case 'f':
				c = '\f';
				break;
			case 'n':
				c = '\n';
				break;
			case 'r':
				c = '\r';
				break;
			case 't':
				c = '\t';
				break;
			case 'v':
				c = '\v';
				break;
			case 'x': {
				wchar_t ch = *iter;
				if (!isxdigit(ch)) {
					line.push_back(ch);
					continue;
				}
				iter++;
				c =static_cast<wchar_t>(hextobin(ch));
				ch = *iter;
				if (isxdigit(ch)) {
					iter++;
					c = c * 16 + static_cast<wchar_t>(hextobin(ch));
				}
			} break;
			case '0':
				c = 0;
				if (!('0' <= *iter && *iter <= '7'))
					break;
				c = *iter++;
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
				c -= '0';
				if ('0' <= *iter && *iter <= '7')
					c = c * 8 + (*iter++ - '0');
				if ('0' <= *iter && *iter <= '7')
					c = c * 8 + (*iter++ - '0');
			case '\\':
				break;
			default:
				line.push_back('\\');
				break;
			}
		}
		line.push_back(c);
	}
	return true;
}

bool IsUnderConhost(FILE *fp) {
	HANDLE hStderr = reinterpret_cast<HANDLE>(_get_osfhandle(_fileno(fp)));
	return GetFileType(hStderr) == FILE_TYPE_CHAR;
}

bool IsWindowsTTY() {
	if (GetEnvironmentVariableW(L"TERM", NULL, 0) == 0) {
		if (GetLastError() == ERROR_ENVVAR_NOT_FOUND)
			return false;
	}
	return true;
}

bool PrintlineU8(FILE *fp, const std::wstring &line) {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
	std::string u8line = conv.to_bytes(line);
	size_t sz = fwrite(u8line.c_str(), 1, u8line.size(), fp);
	return sz == u8line.size();
}

bool Printline(FILE *fp, const std::wstring &line)
{
	if (IsWindowsTTY()) {
		return PrintlineU8(fp, line);
	}
	if (!IsUnderConhost(fp)&&fp != stderr &&fp != stdout) {
		fwprintf(fp, L"%s", line.c_str());
		return true;
	}
	HANDLE hConsole = nullptr;
	if (fp == stdout) {
		hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else if (fp == stderr) {
		hConsole = GetStdHandle(STD_ERROR_HANDLE);
	}
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	GetConsoleScreenBufferInfo(hConsole, &csbi);
	WORD dColor = csbi.wAttributes;
	///

	return true;
}