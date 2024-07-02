#include "Utils.h"

#include <string>

bool ParseUInt(const wchar_t *str, unsigned int *val, const int base)
{
    wchar_t *end;
    if (!str)
    {
        return false;
    }
    *val = std::wcstoul(str, &end, base);
    if (str == end || errno == ERANGE)
    {
        return false;
    }
    return true;
}

bool ParseUInt(const char *str, unsigned int *val, const int base)
{
    char *end;
    if (!str)
    {
        return false;
    }
    *val = std::strtoul(str, &end, base);
    if (str == end || errno == ERANGE)
    {
        return false;
    }
    return true;
}

bool ParseInt(const wchar_t *str, int *val, const int base)
{
    wchar_t *end;
    if (!str)
    {
        return false;
    }
    *val = std::wcstol(str, &end, base);
    if (str == end || errno == ERANGE)
    {
        return false;
    }
    return true;
}

bool ParseInt(const char *str, int *val, const int base)
{
    char *end;
    if (!str)
    {
        return false;
    }
    *val = std::strtol(str, &end, base);
    if (str == end || errno == ERANGE)
    {
        return false;
    }
    return true;
}
