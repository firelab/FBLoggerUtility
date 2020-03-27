#pragma once

#include <string>
#include <algorithm>

#include <QDateTime>

static inline bool IsOnlyDigits(const std::string& str)
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

string inline MakeStringWidthTwoFromInt(const int& data)
{
    string formattedString = "";
    if (data > 9)
    {
        formattedString += std::to_string(data);
    }
    else
    {
        formattedString += "0" + std::to_string(data);
    }
    return formattedString;
}

string inline MakeStringWidthThreeFromInt(const int& data)
{
    string formattedString = "";
    if (data > 99)
    {
        formattedString += std::to_string(data);
    }
    else if (data > 9)
    {
        formattedString += "0" + std::to_string(data);
    }
    else
    {
        formattedString += "00" + std::to_string(data);
    }
    return formattedString;
}

string inline GetMyLocalDateTimeString()
{
    //SYSTEMTIME systemTime;
    //GetLocalTime(&systemTime);
    QDateTime systemDate = QDateTime::currentDateTime();
    QString dateTimeString = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");

    return dateTimeString.toStdString();
}
