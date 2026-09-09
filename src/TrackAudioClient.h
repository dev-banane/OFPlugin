#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <thread>

class FrequencyStore;

class TrackAudioClient
{
public:
    explicit TrackAudioClient(FrequencyStore& store);
    ~TrackAudioClient();

    TrackAudioClient(const TrackAudioClient&) = delete;
    TrackAudioClient& operator=(const TrackAudioClient&) = delete;

    void Start();

private:
    void ThreadMain();

    void RunOnce();

    void HandleMessage(const std::string& message) const;

    FrequencyStore& m_store;
    HANDLE m_stopEvent;
    std::thread m_thread;
};
