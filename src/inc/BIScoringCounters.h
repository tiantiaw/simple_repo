#pragma once
#pragma warning(push)
#include <APUtilities/ApPerfCounterDefs.h>
#include <apsdk/counters.h>
#include <memory>
#pragma warning(pop)

namespace Ads::BIScoring::ServerCounters
{
constexpr char c_perfCategory[]{"BIScoring"};

#define BIScoringPERFCOUNTER_SEQ \
    /* BIScoring Server Counters */ \
    ((RequestLatency)(apsdk::CounterFlags::CounterFlag_Number_Percentiles)) \
    ((ScoringLatency)(apsdk::CounterFlags::CounterFlag_Number_Percentiles)) \
    ((ScoringLatencyPerComponent)(apsdk::CounterFlags::CounterFlag_Number_Percentiles)) \
    ((EventsPerSec)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((EventsCompletedPerSec)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((EventsFailedPerSec)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((ModelLoading)(apsdk::CounterFlags::CounterFlag_Rate)) \

    APPERFCOUNTER_DEFINE_ENUM_CLASS_TYPE(BIScoringServerCounters, BIScoringPERFCOUNTER_SEQ);

template <typename Enum>
inline auto ToCounterId(Enum val) noexcept
{
    return static_cast<APUtilities::CounterId>(val);
}

} // namespace Ads::BIScoring::ServerCounters
