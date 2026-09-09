#include "FrequencyStore.h"

void FrequencyStore::SetDisplayedCallsigns(std::unordered_set<std::string> callsigns)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const auto now = std::chrono::steady_clock::now();
    for (const auto& callsign : callsigns)
        m_lastSeen[callsign] = now;

    m_displayed = std::move(callsigns);

    PruneExpiredPinsLocked(now);
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

void FrequencyStore::ConfirmFrequency(const std::string& callsign, double mhz)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_confirmedFrequencies[callsign] = mhz;

    m_lastSeen.emplace(callsign, std::chrono::steady_clock::now());
}

bool FrequencyStore::Lookup(const char* callsign, double& outMhz) const
{
    if (callsign == nullptr || callsign[0] == '\0')
        return false;

    std::lock_guard<std::mutex> lock(m_mutex);

    auto pinned = m_confirmedFrequencies.find(callsign);
    if (pinned != m_confirmedFrequencies.end())
    {
        outMhz = pinned->second;
        return true;
    }

    auto guessed = m_frequencies.find(callsign);
    if (guessed != m_frequencies.end())
    {
        outMhz = guessed->second;
        return true;
    }

    return false;
}

void FrequencyStore::PruneExpiredPinsLocked(std::chrono::steady_clock::time_point now)
{
    for (auto it = m_lastSeen.begin(); it != m_lastSeen.end(); )
    {
        if (now - it->second > kPinExpiry)
        {
            m_confirmedFrequencies.erase(it->first);
            it = m_lastSeen.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
