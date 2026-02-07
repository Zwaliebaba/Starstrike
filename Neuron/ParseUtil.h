#ifndef ParseUtil_h
#define ParseUtil_h

#include "Color.h"
#include "Geometry.h"
#include "Parser.h"


bool GetDefBool(bool& dst, TermDef* def, std::string_view file);
bool GetDefText(std::string& dst, TermDef* def, std::string_view file);
bool GetDefNumber(int& dst, TermDef* def, std::string_view file);
bool GetDefNumber(DWORD& dst, TermDef* def, std::string_view file);
bool GetDefNumber(float& dst, TermDef* def, std::string_view file);
bool GetDefNumber(double& dst, TermDef* def, std::string_view file);
bool GetDefVec(Vec3& dst, TermDef* def, std::string_view file);
bool GetDefColor(Color& dst, TermDef* def, std::string_view file);
bool GetDefColor(ColorValue& dst, TermDef* def, std::string_view file);
bool GetDefRect(Rect& dst, TermDef* def, std::string_view file);
bool GetDefInsets(Insets& dst, TermDef* def, std::string_view file);
bool GetDefTime(int& dst, TermDef* def, std::string_view file);

bool GetDefArray(int* dst, int size, TermDef* def, std::string_view file);
bool GetDefArray(float* dst, int size, TermDef* def, std::string_view file);
bool GetDefArray(double* dst, int size, TermDef* def, std::string_view file);
bool GetDefArray(std::vector<DWORD>& array, TermDef* def, std::string_view file);
bool GetDefArray(std::vector<float>& array, TermDef* def, std::string_view file);

#endif ParseUtil_h
