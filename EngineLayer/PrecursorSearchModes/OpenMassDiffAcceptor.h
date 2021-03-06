﻿#pragma once

#include "MassDiffAcceptor.h"
#include <string>
#include <vector>
#include <limits>

#include "AllowedIntervalWithNotch.h"

using namespace MzLibUtil;

namespace EngineLayer
{
    class OpenSearchMode : public MassDiffAcceptor
    {
    public:
        OpenSearchMode();
        
        int Accepts(double scanPrecursorMass, double peptideMass) override;
        
        std::vector<AllowedIntervalWithNotch*> GetAllowedPrecursorMassIntervalsFromTheoreticalMass(double peptideMonoisotopicMass) override;
        
        std::vector<AllowedIntervalWithNotch*> GetAllowedPrecursorMassIntervalsFromObservedMass(double peptideMonoisotopicMass) override;
        
        std::string ToProseString() override;
        
        std::string ToString();
    };
}
