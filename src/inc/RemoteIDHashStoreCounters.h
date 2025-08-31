#pragma once
#pragma warning(push)
#include <APUtilities/ApPerfCounterDefs.h>
#include <apsdk/counters.h>
#include <memory>
#pragma warning(pop)

namespace Ads::BIScoring::RemoteIDHashStoreCounters
{
constexpr char c_perfCategory[]{"RemoteIDHashStore"};

#define RemoteIDHashStorePERFCOUNTER_SEQ \
    /* Remote IDHashStore Counters per table */ \
    ((StoreLatency)(apsdk::CounterFlags::CounterFlag_Number_Percentiles)) \
    ((ReadDataRateFailed)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((RequestKeysRate)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((RequestKeysRateSuccess)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((RequestKeysRateNotFound)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((RequestKeysRateTimeout)(apsdk::CounterFlags::CounterFlag_Rate)) \
    ((RequestKeysRateUnexpectedError)(apsdk::CounterFlags::CounterFlag_Rate)) \

    APPERFCOUNTER_DEFINE_ENUM_CLASS_TYPE(Counters, RemoteIDHashStorePERFCOUNTER_SEQ);

template <typename Enum>
inline auto ToCounterId(Enum val) noexcept
{
    return static_cast<APUtilities::CounterId>(val);
}
} // namespace Ads::BIScoring::RemoteIDHashStoreCounters
