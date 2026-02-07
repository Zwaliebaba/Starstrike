#include "pch.h"
#include "Res.h"

static uint64_t RESOURCE_KEY = 1;

Resource::Resource()
  : id((HANDLE)RESOURCE_KEY++) {}

Resource::~Resource() {}
