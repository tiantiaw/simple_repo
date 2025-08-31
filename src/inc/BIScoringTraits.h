#pragma once

#include <GeneCommon/Factory.h>
#include <GeneCommon/IRequestWrapper.h>
#include <GeneCommon/IResponseWrapper.h>
#include <GeneCommon/Traits.h>
#include <Ads.BIScoring.Request_types.h>
#include <Ads.BIScoring.Response_types.h>
#include <MockLogRecord_types.h>
#include <sdr/Reflector.bonded.h>
#include <Utilities/Array.h>

namespace DE::GeneCommon 
{
class EventLogger;

template <typename TGene>
struct LogData;
} // namespace DE::GeneCommon

namespace Ads::BIScoring
{

namespace GeneCommon = Ads::DE::GeneCommon;

class DataProviderManager;
class RequestWrapper;

using DummyLogRecord = GeneCommon::Mocks::MockLogRecord;

struct BIScoringTraitsBase : GeneCommon::DefaultTraitsBase
{
    using ItemType = std::uint8_t;

    using RequestType = Request;
    using RequestWrapper = BIScoring::RequestWrapper;
    using ResponseType = Response;

    using BondLogRecord = DummyLogRecord;
};

struct BIScoringTraits : GeneCommon::DefaultTraits<BIScoringTraitsBase>
{
    using ScoreIteratorType = std::vector<ItemScore>::iterator;
    using DataProviderManagerType = DataProviderManager;
};

using GeneType = BIScoringTraits;


class DbgXmlManager : public GeneCommon::IDbgXmlManager<GeneType::ItemType>
{
public:
    DbgXmlManager(const Request& request);

    bool IsDbgXmlRequest() const override;

    bool IsDbgXmlItemGroup(const GeneCommon::ItemGroupId itemGroupId) const override;

    bool IsDbgXmlItem(const GeneType::ItemType& item) const override;

    ~DbgXmlManager() = default;
};

class RequestWrapper :
    public GeneCommon::IRequestWrapper<GeneType>
{
public:
    RequestWrapper(
        const Request& request,
        const GeneCommon::ClassifierMap& classifierMap,
        const GeneCommon::IArenaStl& allocator,
        const GeneCommon::EventLogger& logger);

    const Request& GetRequest() const override;

    Utilities::Array<const GeneType::ItemType> GetItems() const override;

    Utilities::Array<GeneCommon::ItemData> GetItemsData() override;

    Utilities::Array<const GeneCommon::ItemData> GetItemsData() const override;

    Utilities::Array<const DE::GeneCommon::ItemGroupClassifierInfo> GetItemGroupClassifiers() const override;

    bool IsDebugRequest() const override;

    const GeneCommon::IDbgXmlManager<GeneType::ItemType>& GetDbgXmlManager() const override;

    std::string_view GetRGUID() const override;

    std::string_view GetQuery() const override;

    std::string_view GetFlightIds() const override;

    const GeneCommon::ItemGroupCountMap& GetItemGroupCountMap() const override;

    bool IsCacheEnabled() const noexcept override;

    bool IsFeatureLoggingEnabled() const override;

    ~RequestWrapper() = default;

private:
    const Request& m_request;
    GeneCommon::GeneVector<GeneCommon::ItemData> m_itemsData;
    GeneCommon::ItemGroupCountMap m_itemGroupCountsMap;
    DbgXmlManager m_dbgXmlManager;
    GeneCommon::GeneVector<GeneCommon::ItemGroupClassifierInfo> m_itemGroupClassifiersInfo;
};

class ResponseWrapper :
    public GeneCommon::IResponseWrapper<GeneType>
{
public:
    ResponseWrapper(
        std::shared_ptr<Response> response,
        std::size_t numItems,
        sfl::IAllocator& allocator,
        const GeneCommon::EventLogger& logger);

    void SetScores(const DE::GeneCommon::IRequestWrapper<GeneType>& request) override;

    void SetDbgxml(std::string dbgXml) override;

    bool IsFiltered(GeneType::ScoreIteratorType score) const override;

    Utilities::Array<GeneCommon::ItemScore> GetScores() override
    {
        return m_scoresWrapper;
    }

    void SetBondLog(const DummyLogRecord& /* bondLogRecord */) override;

    std::shared_ptr<Response> GetResponse() override;

    void ResetRemainingResponseItems(
        const GeneCommon::ItemRange<const GeneType::ItemType>& requestItemRange,
        GeneCommon::ResponseScoreIterator responseItemIt,
        const CommonDE::FilterReason filterReason) override;

private:
    std::shared_ptr<Response> m_response;
    GeneCommon::GeneVector<GeneCommon::ItemScore> m_scoresWrapper;
    const GeneCommon::EventLogger& m_logger; 
};


//
// Misc functions used to shape Gene Behaviour 
//

void PrepareLog(const GeneCommon::LogData<GeneType>& logData, DummyLogRecord& logRecord);

} // namespace Ads::BIScoring
