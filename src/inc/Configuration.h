#pragma once

#include <Utilities/AtomicSharedPtr.h>
#include <memory>

namespace Microsoft::AdCenter::FeatureExtraction::Common
{
    class ConfigModelsNameInfo;
} // namespace Microsoft::AdCenter::FeatureExtraction::Common

namespace ListingGeneCommon::TopoFlow
{
class Config;
} // namespace ListingGeneCommon::TopoFlow


namespace Ads::BIScoring
{
namespace Config
{

class Config;
class DynamicConfig;

} // namespace Config

namespace FeatureExtraction = ::Microsoft::AdCenter::FeatureExtraction;

class Configuration
{
public:
    Configuration(
        const char* configFilePath,
        const char* configSchemaFilePath,
        const char* dynamicConfigFilePath,
        const char* dynamicConfigSchemaFilePath);
    Configuration(const Config::Config& config, const Config::DynamicConfig& dynamicConfig);
    Configuration(Configuration&& config) = default;

    const Config::Config* operator -> () const;

    const Config::Config& operator * () const;

    std::shared_ptr<const FeatureExtraction::Common::ConfigModelsNameInfo> GetModelNames() const;
    std::shared_ptr<const ListingGeneCommon::TopoFlow::Config> GetTopoFlowConfig() const;

    bool LoadDynamicConfig();
    const std::shared_ptr<Config::DynamicConfig> GetDynamicConfig() const;

private:
    std::unique_ptr<Config::Config> m_config;
    std::shared_ptr<const FeatureExtraction::Common::ConfigModelsNameInfo> m_modelNames;
    std::shared_ptr<const ListingGeneCommon::TopoFlow::Config> m_topoflowConfig;
    Utilities::AtomicSharedPtr<Config::DynamicConfig> m_dynamicConfig;

    std::string m_dynamicConfigFilePath;
    std::string m_dynamicConfigSchemaFilePath;
    std::time_t m_dynamicConfigLastWriteTime;
};

} // namespace Ads::BIScoring
