#ifndef FormatUtil_h
#define FormatUtil_h

#include "Geometry.h"

void FormatNumber(char* txt, double n);
void FormatNumberExp(char* txt, double n);
void FormatTime(char* txt, double seconds);
void FormatTimeOfDay(char* txt, double seconds);
void FormatDayTime(char* txt, double seconds, bool short_format = false);
void FormatDay(char* txt, double seconds);
void FormatPoint(char* txt, const Point& p);
std::string FormatTimeString(int utc = 0);

std::string SafeString(std::string_view s);
std::string SafeQuotes(std::string_view s);

// scan msg and replace all occurrences of tgt with val
// return new result, leave msg unmodified
std::string FormatTextReplace(std::string_view msg, std::string_view tgt, std::string_view val);

// scan msg and replace all C-style \x escape sequences
// with their single-character values, leave orig unmodified
std::string FormatTextEscape(std::string_view _msg);

#endif FormatUtil_h
