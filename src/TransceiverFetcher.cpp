#include "TransceiverFetcher.h"

#include "FrequencyStore.h"
#include "FrequencyUtil.h"

#include <winhttp.h>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    const wchar_t* const kHost = L"data.vatsim.net";
    const wchar_t* const kPath = L"/v3/transceivers-data.json";

    // The feed itself only regenerates every ~15s
    const DWORD kPollIntervalMs = 20000;
    const DWORD kHttpTimeoutMs = 15000;

    // How soon to re-check while nothing is on screen. Short, because no
    // request is made in that state - it only re-reads a local set.
    const DWORD kIdleRecheckMs = 2000;

    bool IsChatterFrequency(double mhz)
    {
        return std::fabs(mhz - 121.500) < 0.0005 || std::fabs(mhz - 122.800) < 0.0005;
    }

    bool PickFrequency(const std::vector<double>& mhz, double& out)
    {
        if (mhz.empty())
            return false;

        const double* best = nullptr;
        for (const double& candidate : mhz)
        {
            if (IsChatterFrequency(candidate))
                continue;
            if (best == nullptr || candidate < *best)
                best = &candidate;
        }

        if (best == nullptr)
            best = &*std::min_element(mhz.begin(), mhz.end());

        out = *best;
        return true;
    }
}

TransceiverFetcher::TransceiverFetcher(FrequencyStore& store)
    : m_store(store)
    , m_stopEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr))
{
}

TransceiverFetcher::~TransceiverFetcher()
{
    if (m_stopEvent != nullptr)
        SetEvent(m_stopEvent);

    if (m_thread.joinable())
        m_thread.join();

    if (m_stopEvent != nullptr)
        CloseHandle(m_stopEvent);
}

void TransceiverFetcher::Start()
{
    if (m_stopEvent == nullptr || m_thread.joinable())
        return;

    m_thread = std::thread(&TransceiverFetcher::ThreadMain, this);
}

void TransceiverFetcher::ThreadMain()
{
    for (;;)
    {
        DWORD wait = kPollIntervalMs;

        if (m_store.GetDisplayedCallsigns().empty())
        {
            wait = kIdleRecheckMs;
        }
        else
        {
            std::string body;
            if (Download(body))
                ParseAndPublish(body);
        }

        if (WaitForSingleObject(m_stopEvent, wait) == WAIT_OBJECT_0)
            return;
    }
}

bool TransceiverFetcher::Download(std::string& outBody) const
{
    HINTERNET session = WinHttpOpen(L"OFPlugin/0.1",
        WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (session == nullptr)
        return false;

    WinHttpSetTimeouts(session, kHttpTimeoutMs, kHttpTimeoutMs, kHttpTimeoutMs, kHttpTimeoutMs);

    bool ok = false;
    HINTERNET connect = WinHttpConnect(session, kHost, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (connect != nullptr)
    {
        HINTERNET request = WinHttpOpenRequest(connect, L"GET", kPath,
            nullptr,
            WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            WINHTTP_FLAG_SECURE);
        if (request != nullptr)
        {
            if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                    WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
                && WinHttpReceiveResponse(request, nullptr))
            {
                DWORD status = 0;
                DWORD statusSize = sizeof(status);
                if (WinHttpQueryHeaders(request,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &status, &statusSize, WINHTTP_NO_HEADER_INDEX)
                    && status == 200)
                {
                    std::string body;
                    char buffer[8192];
                    DWORD read = 0;
                    ok = true;
                    while (WinHttpReadData(request, buffer, sizeof(buffer), &read))
                    {
                        if (read == 0)
                            break;
                        body.append(buffer, read);
                    }
                    outBody = std::move(body);
                }
            }
            WinHttpCloseHandle(request);
        }
        WinHttpCloseHandle(connect);
    }

    WinHttpCloseHandle(session);
    return ok;
}

void TransceiverFetcher::ParseAndPublish(const std::string& body) const
{
    const std::unordered_set<std::string> displayed = m_store.GetDisplayedCallsigns();
    if (displayed.empty())
        return;

    const nlohmann::json root = nlohmann::json::parse(body, nullptr, false);
    if (root.is_discarded() || !root.is_array())
        return;

    std::unordered_map<std::string, double> frequencies;
    std::vector<double> mhz;

    for (const auto& entry : root)
    {
        if (!entry.is_object())
            continue;

        const auto callsignField = entry.find("callsign");
        if (callsignField == entry.end() || !callsignField->is_string())
            continue;

        const std::string callsign = callsignField->get<std::string>();
        if (displayed.find(callsign) == displayed.end())
            continue;

        const auto transceiversField = entry.find("transceivers");
        if (transceiversField == entry.end() || !transceiversField->is_array())
            continue;

        mhz.clear();
        for (const auto& transceiver : *transceiversField)
        {
            if (!transceiver.is_object())
                continue;

            const auto frequencyField = transceiver.find("frequency");
            if (frequencyField == transceiver.end() || !frequencyField->is_number())
                continue;

            const double value = HzToMhz(frequencyField->get<long long>());
            if (std::find(mhz.begin(), mhz.end(), value) == mhz.end())
                mhz.push_back(value);
        }

        double picked = 0.0;
        if (PickFrequency(mhz, picked))
            frequencies.emplace(callsign, picked);
    }

    m_store.ReplaceFrequencies(std::move(frequencies));
}
