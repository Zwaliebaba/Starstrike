#pragma once

#include <string.h>

void StrToLower(char *_string);

inline char *NewStr(const char *src)
{
	size_t len = strlen(src) + 1;
	char *dst = new char[len];
	memcpy(dst, src, len);
	return dst;
}

