#include "FrequencyStore.h"

void FrequencyStore::SetDisplayedCallsigns(std::unordered_set<std::string> callsigns)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_displayed = std::move(callsigns);
}

std::unordered_set<std::string> FrequencyStore::GetDisplayedCallsigns() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_displayed;
}

void FrequencyStore::ReplaceFrequencies(std::unordered_map<std::string, double> frequencies)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_frequencies = std::move(frequencies);
}

bool FrequencyStore::Lookup(const char* callsign, double& outMhz) const
{
    if (callsign == nullptr || callsign[0] == '\0')
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_frequencies.find(callsign);
    if (it == m_frequencies.end())
        return false;

    outMhz = it->second;
    return true;
}
