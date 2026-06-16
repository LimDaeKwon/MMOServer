#pragma once
#pragma once

#include <string>
#include <unordered_map>

class DKParser
{
public:
    bool Load(const std::string& filePath);

    bool HasSection(const std::string& section) const;
    bool HasValue(const std::string& section, const std::string& key) const;

    bool GetString(const std::string& section, const std::string& key, std::string* outValue) const;
    bool GetInt(const std::string& section, const std::string& key, int* outValue) const;
    bool GetUnsignedInt(const std::string& section, const std::string& key, unsigned int* outValue) const;
    bool GetDouble(const std::string& section, const std::string& key, double* outValue) const;
    bool GetBool(const std::string& section, const std::string& key, bool* outValue) const;
    bool GetUnsignedChar(const std::string& section,const std::string& key, unsigned char* outValue) const;

    const std::string& GetLastError() const;

    void Clear();

private:
    using ValueMap = std::unordered_map<std::string, std::string>;
    using SectionMap = std::unordered_map<std::string, ValueMap>;

    static std::string Trim(const std::string& value);
    static bool IsCommentLine(const std::string& line);
    static bool ParseSection(const std::string& line, std::string* outSection);
    static bool ParseKeyValue(const std::string& line, std::string* outKey, std::string* outValue);
    static std::string ToLower(const std::string& value);

    bool FindValue(const std::string& section, const std::string& key, const std::string** outValue) const;
    void SetError(int lineNumber, const std::string& message);

private:
    SectionMap sections_;
    std::string lastError_;
};
