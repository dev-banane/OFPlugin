#include "TrackAudioClient.h"

#include "FrequencyStore.h"
#include "FrequencyUtil.h"

#include <winhttp.h>

#include <vector>

#include <nlohmann/json.hpp>

namespace
{
    const wchar_t* const kHost = L"127.0.0.1";
    const INTERNET_PORT kPort = 49080;
    const wchar_t* const kPath = L"/ws";
    const DWORD kReconnectDelayMs = 10000;
    const DWORD kConnectTimeoutMs = 15000;
    const DWORD kReceiveTimeoutMs = 5000;
}

TrackAudioClient::TrackAudioClient(FrequencyStore& store)
    : m_store(store)
    , m_stopEvent(CreateEvent(nullptr, TRUE, FALSE, nullptr))
{
}

TrackAudioClient::~TrackAudioClient()
{
    if (m_stopEvent != nullptr)
        SetEvent(m_stopEvent);

    if (m_thread.joinable())
        m_thread.join();

    if (m_stopEvent != nullptr)
        CloseHandle(m_stopEvent);
}

void TrackAudioClient::Start()
{
    if (m_stopEvent == nullptr || m_thread.joinable())
        return;

    m_thread = std::thread(&TrackAudioClient::ThreadMain, this);
}

void TrackAudioClient::ThreadMain()
{
    for (;;)
    {
        RunOnce();

        if (WaitForSingleObject(m_stopEvent, kReconnectDelayMs) == WAIT_OBJECT_0)
            return;
    }
}

void TrackAudioClient::RunOnce()
{
    HINTERNET session = WinHttpOpen(L"OFPlugin/0.1",
        WINHTTP_ACCESS_TYPE_NO_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);
    if (session == nullptr)
        return;

    WinHttpSetTimeouts(session, kConnectTimeoutMs, kConnectTimeoutMs, kConnectTimeoutMs, kReceiveTimeoutMs);

    HINTERNET connect = WinHttpConnect(session, kHost, kPort, 0);
    if (connect == nullptr)
    {
        WinHttpCloseHandle(session);
        return;
    }

    // No WINHTTP_FLAG_SECURE => wss:// is not supported.
    HINTERNET request = WinHttpOpenRequest(connect, L"GET", kPath,
        nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, 0);
    if (request == nullptr)
    {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return;
    }

    // Tells WinHTTP to negotiate a WebSocket upgrade for this request.
    if (!WinHttpSetOption(request, WINHTTP_OPTION_UPGRADE_TO_WEB_SOCKET, nullptr, 0))
    {
        WinHttpCloseHandle(request);
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return;
    }

    HINTERNET webSocket = nullptr;
    if (WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)
        && WinHttpReceiveResponse(request, nullptr))
    {
        webSocket = WinHttpWebSocketCompleteUpgrade(request, 0);
    }

    WinHttpCloseHandle(request);

    if (webSocket == nullptr)
    {
        WinHttpCloseHandle(connect);
        WinHttpCloseHandle(session);
        return;
    }

    std::string message;
    std::vector<char> buffer(8192);

    for (;;)
    {
        if (WaitForSingleObject(m_stopEvent, 0) == WAIT_OBJECT_0)
            break;

        DWORD bytesRead = 0;
        WINHTTP_WEB_SOCKET_BUFFER_TYPE bufferType;
        DWORD result = WinHttpWebSocketReceive(webSocket,
            buffer.data(), static_cast<DWORD>(buffer.size()),
            &bytesRead, &bufferType);

        if (result == ERROR_WINHTTP_TIMEOUT)
            continue;

        if (result != NO_ERROR)
            break;

        if (bufferType == WINHTTP_WEB_SOCKET_CLOSE_BUFFER_TYPE)
            break;

        message.append(buffer.data(), bytesRead);

        if (bufferType == WINHTTP_WEB_SOCKET_UTF8_MESSAGE_BUFFER_TYPE
            || bufferType == WINHTTP_WEB_SOCKET_BINARY_MESSAGE_BUFFER_TYPE)
        {
            HandleMessage(message);
            message.clear();
        }
    }

    WinHttpWebSocketClose(webSocket, WINHTTP_WEB_SOCKET_SUCCESS_CLOSE_STATUS, nullptr, 0);
    WinHttpCloseHandle(webSocket);
    WinHttpCloseHandle(connect);
    WinHttpCloseHandle(session);
}

void TrackAudioClient::HandleMessage(const std::string& message) const
{
    const nlohmann::json root = nlohmann::json::parse(message, nullptr, false);
    if (root.is_discarded() || !root.is_object())
        return;

    const auto typeField = root.find("type");
    if (typeField == root.end() || !typeField->is_string() || *typeField != "kRxBegin")
        return;

    const auto valueField = root.find("value");
    if (valueField == root.end() || !valueField->is_object())
        return;

    const auto callsignField = valueField->find("callsign");
    if (callsignField == valueField->end() || !callsignField->is_string())
        return;

    const auto frequencyField = valueField->find("pFrequencyHz");
    if (frequencyField == valueField->end() || !frequencyField->is_number())
        return;

    const std::string callsign = callsignField->get<std::string>();
    const double mhz = HzToMhz(frequencyField->get<long long>());

    m_store.ConfirmFrequency(callsign, mhz);
}
