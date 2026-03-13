#pragma once

#include <memory>

#include "inputspec.h"
#include "auto_vector.h"


typedef auto_vector<const InputSpec> InputSpecList;

typedef InputSpecList::const_iterator InputSpecIt;

typedef std::unique_ptr<const InputSpec> InputSpecPtr;

typedef std::unique_ptr<const InputSpecList> InputSpecListPtr;

