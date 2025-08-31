#include "stdafx.h"

#pragma warning(push)
#include "BIScoringCounters.h"
#include "Utils.h"
#pragma warning(pop)

namespace Ads::BIScoring
{
std::uint64_t ElapsedTime(const std::chrono::high_resolution_clock::time_point startTime)
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - startTime).count();
}

void SetScoringLatency(const Response& response, std::shared_ptr<APUtilities::ApPerfCounter> serverCounter)
{
    using namespace ServerCounters;
    for (const auto& entry : response.Latency)
    {
        std::uint64_t latency = static_cast<std::uint64_t>(entry.second);
        switch (entry.first)
        {
        case LatencyCounterType::Pvai:
            serverCounter->Set(ToCounterId(BIScoringServerCounters::ScoringLatencyPerComponent), latency, "Pvai");
            break;
        case LatencyCounterType::Click:
            serverCounter->Set(ToCounterId(BIScoringServerCounters::ScoringLatencyPerComponent), latency, "Click");
            break;
        case LatencyCounterType::CreateInput:
            serverCounter->Set(ToCounterId(BIScoringServerCounters::ScoringLatencyPerComponent), latency, "CreateInput");
            break;
        case LatencyCounterType::TotalScoring:
            serverCounter->Set(ToCounterId(BIScoringServerCounters::ScoringLatencyPerComponent), latency, "TotalScoring");
            break;
        default:
            break;
        }
    }
}

} // namespace Ads::BIScoring