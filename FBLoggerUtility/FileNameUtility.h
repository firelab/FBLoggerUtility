#pragma once

#include <string>
using std::string;

static inline string RemoveFileExtension(const string& filename)
{
    size_t lastdot = filename.find_last_of(".");
    if (lastdot == string::npos)
    {
        return filename;
    }
    else
    {
        return filename.substr(0, lastdot);
    }
}
