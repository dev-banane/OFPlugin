#pragma once

#include <chrono>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>

class FrequencyStore
{
public:
    void SetDisplayedCallsigns(std::unordered_set<std::string> callsigns);
    std::unordered_set<std::string> GetDisplayedCallsigns() const;

    void ReplaceFrequencies(std::unordered_map<std::string, double> frequencies);

    // Callsign heard on our working station (certain COM1 identification).
    void ConfirmFrequency(const std::string& callsign, double mhz);

    // Returns false if we have no frequency for this callsign.
    bool Lookup(const char* callsign, double& outMhz) const;

private:
    static constexpr std::chrono::seconds kPinExpiry{60};

    void PruneExpiredPinsLocked(std::chrono::steady_clock::time_point now);

    mutable std::mutex m_mutex;
    std::unordered_set<std::string> m_displayed;
    std::unordered_map<std::string, double> m_frequencies;
    std::unordered_map<std::string, double> m_confirmedFrequencies;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> m_lastSeen;
};
