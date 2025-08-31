#pragma once

#pragma warning(push)
#include "BIScoringTraits.h"
#include <GeneCommon\DataProviderManager.h>
#pragma warning(pop)

#include <cbond/schema_fwd.h>

CBOND_FWD_STRUCT_SLOW(DE::SnR::RankingServer, ListingInfo)

namespace Ads
{
namespace DE::GeneCommon
{
class OlsLogger;
} // namespace DE::GeneCommon

namespace BIScoring
{

namespace FeatureExtraction = ::Microsoft::AdCenter::FeatureExtraction;
namespace Listing = ::Microsoft::AdCenter::Listing;

class FeatureExtraction::Export::FeatureNameParser;

class Configuration;


class DataProviderManager : public GeneCommon::IDataProviderManager<GeneType>, public FeatureExtraction::Utils::ThreadLocal::Singleton<DataProviderManager>
{
public:
    class Accessor final : public GeneCommon::DataProviderManagerDerivedAccessor<GeneType>
    {
    public:
        Accessor(
            const Configuration& config,
            WinAPI::ThreadPool::CleanupGroup& threadPoolCleanupGroup,
            std::shared_ptr<const GeneCommon::OlsLogger> logger);

    private:
        std::shared_ptr<const GeneCommon::OlsLogger> m_logger;
    };

    DataProviderManager();

    GeneCommon::IKeyManager& GetKeyManager() override;

    bool LookUp() override
    {
        return true;
    }

    void ResetItemData(const GeneCommon::ItemData& itemData, const Item& item) override;

private:
    static bool Initialize(
        const FeatureExtraction::Common::ConfigModelsNameInfo& nameInfo,
        const FeatureExtraction::Common::ConfigModelsPathInfo& pathInfo,
        std::chrono::milliseconds modelScanInterval,
        bool enableModelRefresh,
        const ::TopoFlow::SignalLogger::Config& topoFlowSignalLoggerConfig,
        IEventLogManager& logger,
        IEventLogManager& alertableLogger,
        WinAPI::ThreadPool::CleanupGroup& threadPoolCleanupGroup);

    // If this assumption changes, we may need to call reset on it.
    DE::GeneCommon::KeyManager m_keyManager;
    static constexpr const char* c_serviceName = "BIScoring";
};

} // namespace BIScoring
} // namespace Ads
