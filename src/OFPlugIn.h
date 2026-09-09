#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "EuroScopePlugIn.h"

#include "FrequencyStore.h"
#include "TransceiverFetcher.h"

class COFPlugIn : public EuroScopePlugIn::CPlugIn
{
public:
    COFPlugIn();
    virtual ~COFPlugIn();

    void OnGetTagItem(EuroScopePlugIn::CFlightPlan FlightPlan,
        EuroScopePlugIn::CRadarTarget RadarTarget,
        int ItemCode,
        int TagData,
        char sItemString[16],
        int* pColorCode,
        COLORREF* pRGB,
        double* pFontSize) override;

    void OnTimer(int Counter) override;

private:
    FrequencyStore m_store;
    TransceiverFetcher m_fetcher;
};
