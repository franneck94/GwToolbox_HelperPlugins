#pragma once

#include "stl_includes.h"

#define VAR_NAME(v) (#v)

#define TIMER_INIT() (clock())
#define TIMER_DIFF(t) (clock() - t)

bool ParseUInt(const wchar_t *str, unsigned int *val, const int base = 0);

bool ParseUInt(const char *str, unsigned int *val, const int base = 0);

bool ParseInt(const wchar_t *str, int *val, const int base = 0);

bool ParseInt(const char *str, int *val, const int base = 0);
