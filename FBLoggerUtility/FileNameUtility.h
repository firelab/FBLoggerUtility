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

static inline void ReplaceIllegalCharacters(CString& fileName)
{
    LPCTSTR pchUnwanted = _T("<>:\"/\\|?*");
    TCHAR chReplace = _T('_');

    int pos;
    while ((pos = fileName.FindOneOf(pchUnwanted)) >= 0)
    {
        fileName.SetAt(pos, chReplace);
    }
}
