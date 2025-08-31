#include "stdafx.h"

#pragma warning(push)
#include <Ads.BIScoring.Request_apply.h>
#include <Ads.BIScoring.Response_apply.h>

#include "DataProviderManager.h"
#include "Configuration.h"
#include "Config.hxx"

#include <GeneCommon/OlsLogger.h>
#include <Extractor/Inc/Feature.h>

#include <bond/core/bond.h>
#pragma warning(pop)

namespace Ads::BIScoring
{

DataProviderManager::Accessor::Accessor(
    const Configuration& config,
    WinAPI::ThreadPool::CleanupGroup& threadPoolCleanupGroup,
    std::shared_ptr<const GeneCommon::OlsLogger> logger)
    :
    m_logger(std::move(logger))
{
    assert(m_logger);
    if (!DataProviderManager::Initialize(
            *config.GetModelNames(),
            Microsoft::AdCenter::FeatureExtraction::Common::ConfigModelsPathInfo(
                "", // DomainListFile
                "", // DfIdfModels
                "", // LGFModels
                "", // TranslationModels
                "", // TranslationModelsV2
                "", // WB
                "", // WB_V2
                "", // WB_V3
                "", // WB_V4
                "", // WB_V5
                nullptr,
                "", // ACRStoresConfigFile
                "", // DomainHashKeyFile
                "", // IniWeightMapModels
                "", // StringListWeightMapModels
                ""), // SentencePieceModels
            std::chrono::seconds{ config->ModelScanInterval() },
            config->EnableModelRefresh(),
            ::TopoFlow::SignalLogger::Config{}, // topoFlowSignalLoggerConfig
            **m_logger,
            *GetAlertableEventLogManager(),
            threadPoolCleanupGroup))
    {
        throw std::logic_error("Failed to initialize DataProviderManager.");
    }
}

DataProviderManager::DataProviderManager()
    :   m_keyManager(m_dataProviderManager.GetStoreKeyManager(), m_dataProviderManager.GetKeySourceManager())
{
}

bool DataProviderManager::Initialize(
    const Microsoft::AdCenter::FeatureExtraction::Common::ConfigModelsNameInfo& nameInfo,
    const Microsoft::AdCenter::FeatureExtraction::Common::ConfigModelsPathInfo& pathInfo,
    std::chrono::milliseconds modelScanInterval,
    bool enableModelRefresh,
    const ::TopoFlow::SignalLogger::Config& topoFlowSignalLoggerConfig,
    IEventLogManager& logger,
    IEventLogManager& alertableLogger,
    WinAPI::ThreadPool::CleanupGroup& threadPoolCleanupGroup)
{
    return Microsoft::AdCenter::FeatureExtraction::Common::DataProviderManager::InitializeModelManager(
            nameInfo,
            pathInfo,
            0,
            false,
            modelScanInterval,
            logger,
            alertableLogger,
            c_serviceName,
            enableModelRefresh,
            topoFlowSignalLoggerConfig,
            threadPoolCleanupGroup,
            true /*isTopoFlowEnabled*/)
        && Microsoft::AdCenter::FeatureExtraction::Common::DataProviderManager::GetRankerStringTranslator() != nullptr;
}

GeneCommon::IKeyManager& DataProviderManager::GetKeyManager()
{
    return m_keyManager;
}

void DataProviderManager::ResetItemData(const GeneCommon::ItemData& itemData, const Item& item)
{
    // Invoke GeneCommon's general item data reset
    IDataProviderManager<GeneType>::ResetItemData(itemData, item);
}

} // namespace Ads::BIScoring
