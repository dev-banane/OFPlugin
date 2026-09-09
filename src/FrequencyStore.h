#pragma once

#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class FrequencyStore
{
public:
    void SetDisplayedCallsigns(std::unordered_set<std::string> callsigns);
    std::unordered_set<std::string> GetDisplayedCallsigns() const;

    void ReplaceFrequencies(std::unordered_map<std::string, double> frequencies);

    // Returns false if we have no frequency for this callsign.
    bool Lookup(const char* callsign, double& outMhz) const;

private:
    mutable std::mutex m_mutex;
    std::unordered_set<std::string> m_displayed;
    std::unordered_map<std::string, double> m_frequencies;
};
