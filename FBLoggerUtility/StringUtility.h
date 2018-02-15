#pragma once

#include <string>
#include <afxstr.h>
#include <Windows.h>
#include <algorithm>

static inline std::wstring WidenStdString(const std::string& stdString)
{
    // deal with trivial case of empty string
    if (stdString.empty())
    {
        return std::wstring();
    }

    // determine required length of new string
    size_t requiredLength = ::MultiByteToWideChar(CP_UTF8, 0, stdString.c_str(), (int)stdString.length(), 0, 0);

    // construct new string of required length
    std::wstring wideString(requiredLength, L'\0');

    // convert old string to new string
    ::MultiByteToWideChar(CP_UTF8, 0, stdString.c_str(), (int)stdString.length(), &wideString[0], (int)wideString.length());

    // return new string ( compiler should optimize this away )
    return wideString;
}

static inline std::wstring Get_utf16(const std::string &str, int codepage)
{
    if (str.empty()) return std::wstring();
    int sz = MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), 0, 0);
    std::wstring res(sz, 0);
    MultiByteToWideChar(codepage, 0, &str[0], (int)str.size(), &res[0], sz);
    return res;
}

static inline std::string NarrowCStringToStdString(const CString& CString)
{
    // Convert a TCHAR string to a LPCSTR
    CT2CA pszConvertedAnsiString(CString);
    // construct a std::string using the LPCSTR input
    std::string stdString(pszConvertedAnsiString);

    return stdString;
}

static inline std::string Utf8_encode(const std::wstring &wstr)
{
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static bool IsOnlyDigits(const std::string &str)
{
    if (str == "")
    {
        return false;
    }
    else
    {
        return std::all_of(str.begin(), str.end(), ::isdigit);
    }
}
