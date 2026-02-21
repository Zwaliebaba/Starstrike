#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

void DarwiniaReleaseAssert(bool _condition, char const *_fmt, ...);

#ifdef _DEBUG
  #ifdef TARGET_MSVC
	#define DarwiniaDebugAssert(x) \
		if(!(x)) { \
			/*GenerateBlackBox();*/ \
			::ShowCursor(true); \
			_ASSERT(x); \
		}
  #else
    #define DarwiniaDebugAssert(x) { assert(x); }
  #endif
#else
	#define DarwiniaDebugAssert(x)
#endif

void PrintMemoryLeaks();
void GenerateBlackBox( char *_msg );

#endif

