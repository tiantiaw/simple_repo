#pragma once

#pragma warning(push)
#include "BIScoringTraits.h"

#include <GeneCommon/MetricDataLogger.h>

#include <memory>
#pragma warning(pop)

namespace WinAPI
{
class ThreadPool;
} // namespace WinAPI

namespace Ads
{
namespace DE::GeneCommon
{
class OlsLogger;
class EventLogger;
struct IDataProviderManagerDerivedAccessor;
} // namespace DE::GeneCommon

namespace BIScoring
{
namespace Config
{
class EtwConfig;
} // namespace Config

class Configuration;

class DataProviderManagerAccessorFactory
{
public:
    virtual std::shared_ptr<DE::GeneCommon::IDataProviderManagerDerivedAccessor> Create(
        const Configuration& config,
        WinAPI::ThreadPool::CleanupGroup& threadPoolCleanupGroup,
        std::shared_ptr<const DE::GeneCommon::OlsLogger> logger) const;

protected:
    DataProviderManagerAccessorFactory() = default;
};

class MetricDataLoggerFactory
{
public:
    virtual std::shared_ptr<GeneCommon::MetricDataLogger<GeneType>> Create(
        std::shared_ptr<DE::GeneCommon::ApplicationApCounters> applicationCounters,
        const DE::GeneCommon::FillLogRecordFunc<GeneType>& fillLogRecordFunc, 
        std::shared_ptr<const GeneCommon::EventLogger> appLogger) const;

protected:
    MetricDataLoggerFactory() = default;
};
} // namespace BIScoring
} // namespace Ads
