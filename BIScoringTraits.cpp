#include "stdafx.h"

#include "BIScoringTraits.h"
#include <GeneCommon/EventLogger.h>
#include <GeneCommon/ItemStructures.h>
#include <GeneCommon/AsyncContextHolderPool.h>
#include <future>

static const std::string c_emptyString = "";

namespace Ads::BIScoring
{
using namespace DE::GeneCommon;

RequestWrapper::RequestWrapper(
    const Request& request,
    const ClassifierMap& /*classifierMap*/,
    const IArenaStl& allocator,
    const EventLogger& /*logger*/)
    : m_request{request}
    , m_itemsData{allocator}
    , m_itemGroupCountsMap(allocator)
    , m_dbgXmlManager{request}
    , m_itemGroupClassifiersInfo{allocator}
{}

const Request& RequestWrapper::GetRequest() const
{
    return m_request;
}

Utilities::Array<const GeneType::ItemType> RequestWrapper::GetItems() const
{
    return {};
}

Utilities::Array<ItemData> RequestWrapper::GetItemsData()
{
    return m_itemsData;
}

Utilities::Array<const GeneCommon::ItemData> RequestWrapper::GetItemsData() const
{
    return m_itemsData;
}

Utilities::Array<const ItemGroupClassifierInfo> RequestWrapper::GetItemGroupClassifiers() const
{
    return m_itemGroupClassifiersInfo;
}

bool RequestWrapper::IsDebugRequest() const
{
    return true;
}

const GeneCommon::IDbgXmlManager<GeneType::ItemType>& RequestWrapper::GetDbgXmlManager() const
{
    return m_dbgXmlManager;
}

std::string_view RequestWrapper::GetRGUID() const
{
    return c_emptyString;
}

std::string_view RequestWrapper::GetQuery() const
{
    return c_emptyString;
}

std::string_view RequestWrapper::GetFlightIds() const
{
    return c_emptyString;
}

bool RequestWrapper::IsCacheEnabled() const noexcept
{
    return false;
}

const GeneCommon::ItemGroupCountMap& RequestWrapper::GetItemGroupCountMap() const
{
    return m_itemGroupCountsMap;
}

bool RequestWrapper::IsFeatureLoggingEnabled() const
{
    return false;
}

//######################### Response Wrapper ###########################################

ResponseWrapper::ResponseWrapper(
    std::shared_ptr<Response> response,
    std::size_t /*numItems*/,
    sfl::IAllocator& allocator,
    const GeneCommon::EventLogger& logger)
    : m_response{std::move(response)}
    , m_scoresWrapper{allocator}
    , m_logger{logger}
{}

void ResponseWrapper::SetScores(const IRequestWrapper<GeneType>& /*irequestWrapper*/)
{}

void ResponseWrapper::SetDbgxml(std::string /*dbgXml*/)
{}

bool ResponseWrapper::IsFiltered(GeneType::ScoreIteratorType /*score*/) const
{
    return false;
}

std::shared_ptr<Response> ResponseWrapper::GetResponse()
{
    return m_response;
}

void ResponseWrapper::ResetRemainingResponseItems(
    const GeneCommon::ItemRange<const GeneType::ItemType>& /*requestItemRange*/,
    GeneCommon::ResponseScoreIterator /*responseItemIt*/,
    const CommonDE::FilterReason /*filterReason*/)
{}

void ResponseWrapper::SetBondLog(const DummyLogRecord& /*bondLogRecord*/)
{
     // Not implemented
}
//
////######################### Misc ###########################################

void PrepareLog(const LogData<GeneType>& /*logData*/, DummyLogRecord& /*logRecord*/)
{}

DbgXmlManager::DbgXmlManager(const Request& /*request*/)
{}

bool DbgXmlManager::IsDbgXmlRequest() const
{
    return false;
}

bool DbgXmlManager::IsDbgXmlItemGroup(const ItemGroupId /*itemGroupId*/) const
{
    return IsDbgXmlRequest();
}

bool DbgXmlManager::IsDbgXmlItem(const GeneType::ItemType& /*item*/) const
{
    return false;
}

} // namespace Ads::BIScoring
