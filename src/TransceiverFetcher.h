#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <thread>

class FrequencyStore;

class TransceiverFetcher
{
public:
    explicit TransceiverFetcher(FrequencyStore& store);
    ~TransceiverFetcher();

    TransceiverFetcher(const TransceiverFetcher&) = delete;
    TransceiverFetcher& operator=(const TransceiverFetcher&) = delete;

    void Start();

private:
    void ThreadMain();
    bool Download(std::string& outBody) const;
    void ParseAndPublish(const std::string& body) const;

    FrequencyStore& m_store;
    HANDLE m_stopEvent;
    std::thread m_thread;
};
