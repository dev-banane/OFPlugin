#include "OFPlugIn.h"

#include <cstdio>
#include <string>
#include <unordered_set>

namespace
{
    const int TAG_ITEM_PILOT_FREQUENCY = 1;
    const int kCallsignRefreshSeconds = 5;
}

COFPlugIn::COFPlugIn()
    : CPlugIn(EuroScopePlugIn::COMPATIBILITY_CODE,
        "OFPlugin",
        "0.1.0",
        "Jakob Pütz",
        "AGPL v3")
    , m_fetcher(m_store)
    , m_trackAudioClient(m_store)
{
    RegisterTagItemType("Pilot frequency", TAG_ITEM_PILOT_FREQUENCY);
    m_fetcher.Start();
    m_trackAudioClient.Start();
}

COFPlugIn::~COFPlugIn()
{
}

void COFPlugIn::OnTimer(int Counter)
{
    if (Counter % kCallsignRefreshSeconds != 0)
        return;

    std::unordered_set<std::string> displayed;
    for (EuroScopePlugIn::CRadarTarget target = RadarTargetSelectFirst();
        target.IsValid();
        target = RadarTargetSelectNext(target))
    {
        const char* callsign = target.GetCallsign();
        if (callsign != nullptr && callsign[0] != '\0')
            displayed.insert(callsign);
    }

    m_store.SetDisplayedCallsigns(std::move(displayed));
}

void COFPlugIn::OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
    EuroScopePlugIn::CRadarTarget RadarTarget,
    int ItemCode,
    int TagData,
    char sItemString[16],
    int* pColorCode,
    COLORREF* pRGB,
    double* pFontSize)
{
    if (ItemCode != TAG_ITEM_PILOT_FREQUENCY)
        return;

    const char* callsign = nullptr;
    if (RadarTarget.IsValid())
        callsign = RadarTarget.GetCallsign();
    else if (FlightPlan.IsValid())
        callsign = FlightPlan.GetCallsign();

    double mhz = 0.0;
    if (!m_store.Lookup(callsign, mhz))
        return;

    snprintf(sItemString, 16, "%.3f", mhz);
}

COFPlugIn* g_pPlugInInstance = nullptr;

void __declspec(dllexport) EuroScopePlugInInit(EuroScopePlugIn::CPlugIn** ppPlugInInstance)
{
    *ppPlugInInstance = g_pPlugInInstance = new COFPlugIn();
}

void __declspec(dllexport) EuroScopePlugInExit(void)
{
    delete g_pPlugInInstance;
    g_pPlugInInstance = nullptr;
}
