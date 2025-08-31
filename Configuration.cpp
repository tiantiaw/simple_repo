#include "stdafx.h"
#include "Configuration.h"
#include "Config.hxx"
#include "DynamicConfig.hxx"

#include <ListingGeneCommon\DataProviders\RankerStringTranslator.h>
#include <ListingGeneCommon\TopoFlow\Config.h>
#include <GeneCommon\Factory.h>

#include <boost/filesystem.hpp>
#include <string>
#include <vector>

namespace
{
Microsoft::AdCenter::FeatureExtraction::Common::ConfigModelsNameInfo CreateModelNames(
    const Ads::BIScoring::Config::Config& /*config*/)
{
    const auto c_emptyModelNames = std::vector<std::string>{};
    return Microsoft::AdCenter::FeatureExtraction::Common::ConfigModelsNameInfo(
        c_emptyModelNames,  // DfIdfModels
        c_emptyModelNames,  // LGFModels
        c_emptyModelNames,  // TranslationModels
        c_emptyModelNames,  // TranslationModelsV2
        c_emptyModelNames,  // WoodblocksModels
        c_emptyModelNames,  // WoodblocksModelsV2
        c_emptyModelNames,  // WoodblocksModelV3
        c_emptyModelNames,  // WoodblocksModelV4
        c_emptyModelNames,  // WoodblocksModelV5
        c_emptyModelNames,  // IniWeightMap
        c_emptyModelNames,  // StringListWeightMap
        c_emptyModelNames); // SentencePieceModel
}
} // namespace

namespace Ads::BIScoring
{

Configuration::Configuration(
    const char* configFilePath, 
    const char* configSchemaFilePath,
    const char* dynamicConfigFilePath,
    const char* dynamicConfigSchemaFilePath)
{
    if (configFilePath == nullptr)
    {
        // The config will be placed at the same level as the application bin folder.
        configFilePath = "..\\Config.config";
    }

    if (configSchemaFilePath == nullptr)
    {
        configSchemaFilePath = "..\\Config.xsd";
    }

    ::xml_schema::properties props;

    props.schema_location("urn:Gene-Config", configSchemaFilePath);

    m_config = Config::Config_(configFilePath, 0, props);
    m_modelNames = std::make_shared<const FeatureExtraction::Common::ConfigModelsNameInfo>(CreateModelNames(*m_config));

    m_dynamicConfigFilePath = (dynamicConfigFilePath == nullptr) ?
        "..\\DynamicConfig.config" : std::string(dynamicConfigFilePath);

    m_dynamicConfigSchemaFilePath = (dynamicConfigSchemaFilePath == nullptr) ?
        "DynamicConfig.xsd" : std::string(dynamicConfigSchemaFilePath);

    LoadDynamicConfig();

    m_topoflowConfig = DE::GeneCommon::TopoFlowConfigFactory().Create(m_config->TopoFlow());
}

Configuration::Configuration(const Config::Config& config, const Config::DynamicConfig& dynamicConfig)
    :
    m_config(std::make_unique<Config::Config>(config)),
    m_dynamicConfig(std::make_shared<Config::DynamicConfig>(dynamicConfig)),
    m_modelNames(std::make_shared<const FeatureExtraction::Common::ConfigModelsNameInfo>(CreateModelNames(*m_config))),
    m_topoflowConfig(DE::GeneCommon::TopoFlowConfigFactory().Create(m_config->TopoFlow()))    
{}

const Config::Config* Configuration::operator -> () const
{
    return m_config.get();
}

const Config::Config& Configuration::operator * () const
{
    assert(m_config);
    return *m_config;
}

std::shared_ptr<const FeatureExtraction::Common::ConfigModelsNameInfo> Configuration::GetModelNames() const
{
    return m_modelNames;
}

std::shared_ptr<const ListingGeneCommon::TopoFlow::Config> Configuration::GetTopoFlowConfig() const
{
    return m_topoflowConfig;
}

const std::shared_ptr<Config::DynamicConfig> Configuration::GetDynamicConfig() const
{
    return m_dynamicConfig.Load();
}

bool Configuration::LoadDynamicConfig()
{
    if (!boost::filesystem::exists(m_dynamicConfigFilePath))
    {
        return false;
    }
    if (m_dynamicConfigLastWriteTime != boost::filesystem::last_write_time(boost::filesystem::path{ m_dynamicConfigFilePath }))
    {
        const char* dynamicConfigFilePath = m_dynamicConfigFilePath.c_str();
        const char* dynamicConfigSchemaFilePath = m_dynamicConfigSchemaFilePath.c_str();

        ::xml_schema::properties props;
        props.schema_location("urn:Gene-Config", dynamicConfigSchemaFilePath);

        std::shared_ptr<Config::DynamicConfig> newDynamicConfig;
        newDynamicConfig = Config::DynamicConfig_(dynamicConfigFilePath, 0, props);
        m_dynamicConfig.Store(newDynamicConfig);
        m_dynamicConfigLastWriteTime = boost::filesystem::last_write_time(boost::filesystem::path{ m_dynamicConfigFilePath });
        return true;
    }
    return false;
}

} // namespace Ads::BIScoring
