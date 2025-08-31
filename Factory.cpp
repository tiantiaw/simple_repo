#include "stdafx.h"

#pragma warning(push)
#include "Factory.h"
#include "DataProviderManager.h"
#include "Configuration.h"
#include "Config.hxx"

#include <GeneCommon/Factory.h>
#include <GeneCommon/OlsLogger.h>
#include <MockLogRecord_bond_cb.h>
#include <MockLogRecord_reflection.h>

#include <utility>
#include <memory>
#pragma warning(pop)

namespace Ads::BIScoring
{

using namespace GeneCommon;

std::shared_ptr<IDataProviderManagerDerivedAccessor> DataProviderManagerAccessorFactory::Create(
    const Configuration& config,
    WinAPI::ThreadPool::CleanupGroup& threadPoolCleanupGroup,
    std::shared_ptr<const GeneCommon::OlsLogger> logger) const
{
    return std::make_shared<DataProviderManager::Accessor>(config, threadPoolCleanupGroup, std::move(logger));
}

std::shared_ptr<GeneCommon::MetricDataLogger<GeneType>> MetricDataLoggerFactory::Create(
    std::shared_ptr<GeneCommon::ApplicationApCounters> applicationCounters,
    const GeneCommon::FillLogRecordFunc<GeneType>& fillLogRecordFunc,
    std::shared_ptr<const GeneCommon::EventLogger> appLogger) const
{
    return GeneCommon::MetricDataLogger<GeneType>::Create(
        std::move(applicationCounters),
        BondLoggerConfig{
            false, // FeatureOn
            "", // ProviderGUID
            "", // LogSessionName
            "", // LogNamePrefix
            "", // LogDirectory
            std::chrono::minutes(0),
            0 }, // SessionBufferSize
        fillLogRecordFunc,
        "BIScoringMetricLog",
        *appLogger);
}
} // namespace Ads::BIScoring
