#include "DKParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <limits>
#include <sstream>
#include <iostream>
#include <stdexcept>

bool DKParser::Load(const std::string& filePath)
{
    Clear();

    std::ifstream file(filePath);

    if (!file.is_open())
    {
        lastError_ = "설정 파일을 열 수 없습니다: " + filePath;
		std::cout <<"can not open "  << std::endl;
        return false;
    }

    std::string currentSection;
    std::string line;
    int lineNumber = 0;

    while (std::getline(file, line))
    {
        ++lineNumber;

        if (!line.empty() && line.back() == '\r')
        {
            line.pop_back();
        }

        std::string trimmedLine = Trim(line);

        if (trimmedLine.empty() || IsCommentLine(trimmedLine))
        {
            continue;
        }

        std::string section;

        if (ParseSection(trimmedLine, &section))
        {
            currentSection = section;
            sections_[currentSection];
            continue;
        }

        std::string key;
        std::string value;

        if (!ParseKeyValue(trimmedLine, &key, &value))
        {
            SetError(lineNumber, "올바르지 않은 설정 형식입니다.");
            Clear();

            std::cout << "Wrong" << std::endl;
            return false;
        }

        if (currentSection.empty())
        {
            SetError(lineNumber, "설정값보다 먼저 섹션을 선언해야 합니다.");
            Clear();
            std::cout << "SectionFirst" << std::endl;
            return false;
        }

        sections_[currentSection][key] = value;
    }

    return true;
}

bool DKParser::HasSection(const std::string& section) const
{
    return sections_.find(section) != sections_.end();
}

bool DKParser::HasValue(const std::string& section, const std::string& key) const
{
    const std::string* value = nullptr;
    return FindValue(section, key, &value);
}

bool DKParser::GetString(const std::string& section, const std::string& key, std::string* outValue) const
{
    if (outValue == nullptr)
    {
        return false;
    }

    const std::string* value = nullptr;

    if (!FindValue(section, key, &value))
    {
        return false;
    }

    *outValue = *value;
    return true;
}

bool DKParser::GetInt(const std::string& section, const std::string& key, int* outValue) const
{
    if (outValue == nullptr)
    {
        return false;
    }

    const std::string* value = nullptr;

    if (!FindValue(section, key, &value))
    {
        return false;
    }

    try
    {
        std::size_t parsedLength = 0;
        long parsedValue = std::stol(*value, &parsedLength, 0);

        if (parsedLength != value->size())
        {
            return false;
        }

        if (parsedValue < std::numeric_limits<int>::min() || parsedValue > std::numeric_limits<int>::max())
        {
            return false;
        }

        *outValue = static_cast<int>(parsedValue);
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

bool DKParser::GetUnsignedInt(const std::string& section, const std::string& key, unsigned int* outValue) const
{
    if (outValue == nullptr)
    {
        return false;
    }

    const std::string* value = nullptr;

    if (!FindValue(section, key, &value))
    {
        return false;
    }

    if (!value->empty() && value->front() == '-')
    {
        return false;
    }

    try
    {
        std::size_t parsedLength = 0;
        unsigned long parsedValue = std::stoul(*value, &parsedLength, 0);

        if (parsedLength != value->size())
        {
            return false;
        }

        if (parsedValue > std::numeric_limits<unsigned int>::max())
        {
            return false;
        }

        *outValue = static_cast<unsigned int>(parsedValue);
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

bool DKParser::GetDouble(const std::string& section, const std::string& key, double* outValue) const
{
    if (outValue == nullptr)
    {
        return false;
    }

    const std::string* value = nullptr;

    if (!FindValue(section, key, &value))
    {
        return false;
    }

    try
    {
        std::size_t parsedLength = 0;
        double parsedValue = std::stod(*value, &parsedLength);

        if (parsedLength != value->size())
        {
            return false;
        }

        *outValue = parsedValue;
    }
    catch (const std::exception&)
    {
        return false;
    }

    return true;
}

bool DKParser::GetBool(const std::string& section, const std::string& key, bool* outValue) const
{
    if (outValue == nullptr)
    {
        return false;
    }

    const std::string* value = nullptr;

    if (!FindValue(section, key, &value))
    {
        return false;
    }

    std::string lowerValue = ToLower(*value);

    if (lowerValue == "true" || lowerValue == "1" || lowerValue == "yes" || lowerValue == "on")
    {
        *outValue = true;
        return true;
    }

    if (lowerValue == "false" || lowerValue == "0" || lowerValue == "no" || lowerValue == "off")
    {
        *outValue = false;
        return true;
    }

    return false;
}

bool DKParser::GetUnsignedChar(const std::string& section, const std::string& key, unsigned char* outValue) const
{

    if (outValue == nullptr)
    {
        return false;
    }

    unsigned int parsedValue = 0;

    if (!GetUnsignedInt(section, key, &parsedValue))
    {
        return false;
    }

    if (parsedValue > std::numeric_limits<unsigned char>::max())
    {
        return false;
    }

    *outValue = static_cast<unsigned char>(parsedValue);
    return true;
}

const std::string& DKParser::GetLastError() const
{
    return lastError_;
}

void DKParser::Clear()
{
    sections_.clear();
    lastError_.clear();
}

std::string DKParser::Trim(const std::string& value)
{
    std::size_t begin = 0;

    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin])) != 0)
    {
        ++begin;
    }

    std::size_t end = value.size();

    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
    {
        --end;
    }

    return value.substr(begin, end - begin);
}

bool DKParser::IsCommentLine(const std::string& line)
{
    return !line.empty() && (line.front() == '#' || line.front() == ';');
}

bool DKParser::ParseSection(const std::string& line, std::string* outSection)
{
    if (outSection == nullptr)
    {
        return false;
    }

    if (line.size() < 3 || line.front() != '[' || line.back() != ']')
    {
        return false;
    }

    std::string section = Trim(line.substr(1, line.size() - 2));

    if (section.empty())
    {
        return false;
    }

    *outSection = section;
    return true;
}

bool DKParser::ParseKeyValue(const std::string& line, std::string* outKey, std::string* outValue)
{
    if (outKey == nullptr || outValue == nullptr)
    {
        return false;
    }

    std::size_t separatorPosition = line.find('=');

    if (separatorPosition == std::string::npos)
    {
        return false;
    }

    std::string key = Trim(line.substr(0, separatorPosition));
    std::string value = Trim(line.substr(separatorPosition + 1));

    if (key.empty())
    {
        return false;
    }

    if (value.size() >= 2)
    {
        bool isDoubleQuoted = value.front() == '"' && value.back() == '"';
        bool isSingleQuoted = value.front() == '\'' && value.back() == '\'';

        if (isDoubleQuoted || isSingleQuoted)
        {
            value = value.substr(1, value.size() - 2);
        }
    }

    *outKey = key;
    *outValue = value;

    return true;
}

std::string DKParser::ToLower(const std::string& value)
{
    std::string result = value;

    std::transform(result.begin(), result.end(), result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });

    return result;
}

bool DKParser::FindValue(const std::string& section, const std::string& key, const std::string** outValue) const
{
    if (outValue == nullptr)
    {
        return false;
    }

    SectionMap::const_iterator sectionIterator = sections_.find(section);

    if (sectionIterator == sections_.end())
    {
        return false;
    }

    ValueMap::const_iterator valueIterator = sectionIterator->second.find(key);

    if (valueIterator == sectionIterator->second.end())
    {
        return false;
    }

    *outValue = &valueIterator->second;
    return true;
}

void DKParser::SetError(int lineNumber, const std::string& message)
{
    std::ostringstream stream;
    stream << "설정 파일 " << lineNumber << "번째 줄: " << message;

    lastError_ = stream.str();
}
