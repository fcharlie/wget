///
#include "stdafx.h"

#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <ctype.h>

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

bool LineToBin(const wchar_t *cstr,std::wstring &line) {
	assert(cstr);
	auto len = wcslen(cstr);
	line.reserve(len + 1);
	/// Do Parse....
	auto iter = cstr;
	unsigned char c;
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
				c = hextobin(ch);
				ch = *iter;
				if (isxdigit(ch)) {
					iter++;
					c = c * 16 + hextobin(ch);
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

bool Printline(const std::wstring &line)
{
	return true;
}