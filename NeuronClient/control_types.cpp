#include "pch.h"

#include "input_types.h"
#include "control_bindings.h"

static ControlAction s_actions[] = {

	#define DEF_CONTROL_TYPE(x,y) "x", y,
	#include "control_types.inc"
	#undef DEF_CONTROL_TYPE

	"",                                  INPUT_TYPE_ANY,

	NULL,                                NULL

};

