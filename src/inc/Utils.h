#pragma once

#pragma warning(push)
#include <Ads.BIScoring.Request_types.h>
#include <Ads.BIScoring.Response_types.h>
#include <Server/AsyncResult.h>

#include "APUtilities/ApPerfCounter.h"
#include "RemoteIDHashStore.h"

#include <bond/core/bond.h>
#pragma warning(pop)

namespace Ads
{
namespace APUtilities
{
class ILogger;
} // namespace APUtilities

namespace BIScoring
{
std::uint64_t ElapsedTime(const std::chrono::high_resolution_clock::time_point startTime);

void SetScoringLatency(const Response& response, std::shared_ptr<APUtilities::ApPerfCounter> serverCounter);

} // namespace BIScoring
} // namespace Ads