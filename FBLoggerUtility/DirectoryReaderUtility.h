#pragma once

#include <string>
#include <vector>
#include <Windows.h>
#include "StringUtility.h"

using std::string;
using std::wstring;
using std::vector;

static inline void ReadDirectoryIntoStringVector(const string& name, vector<string>& fileNameList)
{
    int codepage = CP_UTF8;
    wstring pattern(Get_utf16(name, codepage));
    pattern += L"\\*.dat";
    WIN32_FIND_DATA data;
    HANDLE hFind;

    if ((hFind = FindFirstFile(pattern.c_str(), &data)) != INVALID_HANDLE_VALUE) 
    {
        do 
        {
            fileNameList.push_back(Utf8_encode(data.cFileName));
        } while (FindNextFile(hFind, &data) != 0);
        FindClose(hFind);
    }
}

static inline bool MyFileExists(const string& filePath)
{
    CString pathCString(filePath.c_str());
    return !(INVALID_FILE_ATTRIBUTES == GetFileAttributes(pathCString) && GetLastError() == ERROR_FILE_NOT_FOUND);
}

static inline bool MyFileExists(const CString& filePath)
{
    return !(INVALID_FILE_ATTRIBUTES == GetFileAttributes(filePath) && GetLastError() == ERROR_FILE_NOT_FOUND);
}
