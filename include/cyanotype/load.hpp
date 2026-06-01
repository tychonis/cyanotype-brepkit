#pragma once

#include <fstream>
#include <map>

namespace cyanotype
{
    std::map<std::string, std::string> load_digest_map(const std::string &path)
    {
        std::ifstream file(path);
        if (!file)
        {
            throw std::runtime_error("failed to open digest map: " + path);
        }

        std::map<std::string, std::string> result;

        std::string line;
        while (std::getline(file, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }

            auto comma = line.find(',');
            if (comma == std::string::npos)
            {
                continue;
            }

            result[line.substr(0, comma)] = line.substr(comma + 1);
        }

        return result;
    }

} // namespace cyanotype
