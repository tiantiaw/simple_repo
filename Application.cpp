#include "stdafx.h"

#pragma warning(push)
#include "Application.h"
#include "BIScoringCounters.h"
#include "Factory.h"
#include "Config.hxx"
#include "DynamicConfig.hxx"
#include "Configuration.h"
#include "DataProviderManager.h"
#include "RemoteIDHashStore.h"
#include "RemoteIDHashStoreCounters.h"
#include "APUtilities/ApPerfCounterDefs.h"

#include <Ads.BIScoring.LogRecord_reflection.h>
#include <Ads.BIScoring.LogRecord_bond_cb.h>
#include <Ads.BIScoring.Request_bond_generic.h>
#include <Ads.BIScoring.Request_bond_cb.h>
#include <Ads.BIScoring.Response_reflection.h>
#include <Ads.BIScoring.Response_bond_cb.h>

#include <AdsClassifierRuntime/ClassifierLoadingSettings.h>
#include <GeneCommon/AsyncApplicationImplBase.h>
#include <GeneCommon/Utils.h>
#include <ListingGeneCommon/ACR/ACRPegasusCache.h>
#include <MockLogRecord_bond_cb.h>
#include <Utilities/ChainedArenaAllocatorPool.h>
#include <Warmup/Utility.h>

#include <WinAPI/ThreadPool/Environment.h>
#include <WinAPI/ThreadPool/Parallel/All.h>
#include <WinAPI/ThreadPool/Parallel/Callback.h>
#include <WinAPI/ThreadPool/Parallel/Launch.h>

#include <sdr/ext/View.h>

#include <cassert>
#include <future>
#include <ranges>
#pragma warning(pop)

namespace Ads::BIScoring
{
using namespace DE::GeneCommon;

using GeneAsyncContextHolderPool = GeneCommon::AsyncContextHolderPool<
    GeneType,
    RequestWrapper,
    ResponseWrapper,
    GeneCommon::SharedRequestContext<GeneType>,
    GeneCommon::SharedAsyncRequestContext<GeneType::ResponseType>,
    Ads::BIScoring::Config::DynamicConfig,
    false>;

class Application::Impl
    : public AsyncApplicationImplBase<Configuration, GeneType, RequestWrapper, ResponseWrapper, GeneAsyncContextHolderPool>
{
public:
    using has_context = void;

    using ExponentialExpansionChainedAllocatorPool = Ads::Utilities::ChainedArena::AllocatorPool<
        Utilities::ChainedArena::ThreadSafePolicies<arenalib::policy::ExponentialExpansion>>; 

    Impl(
        std::unique_ptr<Configuration> config,
        ScoringFunc scoring,
        std::function<void(const bond::blob&)> etwLogger,
        std::shared_ptr<WinAPI::ThreadPool> threadPool,
        std::shared_ptr<WinAPI::ThreadPool> scoringThreadPool,
        std::shared_ptr<OlsLogger> logger,
        std::optional<DCacheClient> dCacheClient,
        std::shared_ptr<Ads::Communication::Bootstrap::ClientHost> minervaAcsHostClient,
        std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> remoteStoreClient,
        const ApCountersFactory<GeneCounters>& apCountersFactory)
        : Impl::AsyncApplicationImplBase{
            std::move(config),
            std::move(logger),
            std::move(threadPool),
            std::make_shared<ApplicationApCounters>(apCountersFactory, "GeneCommon", ListingGeneCommon::PClick::CounterIdMap{}, LocalCountersFactory{}, "BIScoring"),
            std::make_shared<AdsClassifierRuntime::RuntimeEnvironment>(),
            nullptr} // rankingDataStore
        , m_store{ std::make_shared<RemoteIDHashStore>(
              remoteStoreClient ? std::move(remoteStoreClient) : RemoteHashStore::Client::CreateRemoteStoreClient(1ui8, {}),
              std::make_shared<APUtilities::ApPerfCounter>(APPERFCOUNTER_DEFINE_MAP(
                  RemoteIDHashStoreCounters::c_perfCategory,
                  RemoteIDHashStorePERFCOUNTER_SEQ))) }
        , m_dCacheClient{ std::move(dCacheClient) }
        , m_minervaAcsHostClient{ std::move(minervaAcsHostClient) }
        , m_scoringEnv{ std::move(scoringThreadPool) }
        , m_scoring{ std::move(scoring) }
        , m_etwLogger{ std::move(etwLogger) }
    {
        m_dataProvider = Utilities::FactoryOf<DataProviderManagerAccessorFactory>::Create(*m_config, *m_threadPoolCleanupGroup, m_logger);
        m_classifierMap = Utilities::FactoryOf<ClassifierMapFactory<BIScoringTraits>>::Create(
            m_dataProvider->GetRankerStringTranslator(),
            m_logger,
            m_dataProvider,
            m_config->GetTopoFlowConfig(),
            Microsoft::AdCenter::FeatureExtraction::Common::ClassifierInfoFactoryConfig{{}, 0});

        m_acrScoreCaches.push_back(std::make_unique<Microsoft::AdCenter::FeatureExtraction::Common::ACRPegasusCache>(
            static_cast<std::uint32_t>(
                std::chrono::duration_cast<std::chrono::seconds>(std::chrono::minutes{60}).count()),
            1024 * c_oneMb));

        m_callbacks.push_back(
            Utilities::FactoryOf<ClassifierMonitorFactory>::Create(
                std::string{(*m_config)->Classifiers().Location()},
                (*m_config)->Classifiers().FilePattern(),
                std::chrono::milliseconds{(*m_config)->Classifiers().RefreshRate()},
                *m_classifierMap,
                WinAPI::ThreadPool::GetDefault(),
                m_logger,
                nullptr,
                m_classifierRuntimeEnvironment,
                {}));

        m_fileMonitors = Utilities::FactoryOf<FileMonitorCollectionFactory>::Create(m_callbacks);
        m_metricDataLogger = Utilities::FactoryOf<MetricDataLoggerFactory>::Create(m_counters, PrepareLog, m_logger);
        m_httpServer = std::make_shared<HttpServer>(m_classifierMap, **m_logger);

        StartFileMonitor();
        StartHttpServer();
    }

    void InitRequestAsync(SharedAsyncContextHolder /*asyncContextHolder*/, Utils::AsyncCallback /*callback*/) override
    {
        assert(false);
    }

    WinAPI::Parallel::Void ProcessRequestAsync(
        Request request,
        SharedAsyncRequestContext<Response> asyncContext,
        const APUtilities::ILogger& logger,
        WinAPI::Parallel::detached_tag = {}) noexcept
    {
        try
        {
            auto response = co_await ProcessRequest(std::move(request), logger);
            asyncContext->Callback(SharedAsyncResult<Response>{ std::move(response) });
        }
        catch (const std::exception& e)
        {
            ADS_APUTILITIES_LOG_THROTTLED_ERROR(logger, "ProcessRequestAsync failed: %s", e.what());
            m_perfCounter->Add(
                ServerCounters::ToCounterId(ServerCounters::BIScoringServerCounters::EventsFailedPerSec),
                request.RequestRecords.size(),
                request.TrafficType.c_str());
            asyncContext->Callback(SharedAsyncResult<Response>{ std::current_exception() });
        }
    }

    friend IArenaStl GetContext(std::allocator_arg_t, Impl& impl)
    {
        return impl.GetAllocator();
    }

    friend WinAPI::ThreadPool::Environment* GetContext(WinAPI::Parallel::environment_arg_t, Impl& impl) noexcept
    {
        return impl.m_threadPoolCleanupGroup.get();
    }

private:
    WinAPI::Parallel::Result<std::shared_ptr<Response>> ProcessRequest(Request request, const APUtilities::ILogger& logger)
    {
        const auto requestStartTime = std::chrono::high_resolution_clock::now();
        const auto eventsSize = request.RequestRecords.size();
        m_perfCounter->Add(
            ServerCounters::ToCounterId(ServerCounters::BIScoringServerCounters::EventsPerSec),
            eventsSize,
            request.TrafficType.c_str());

        co_await WinAPI::Parallel::All(
            ReadRemoteStatsTable(request.EnableStatsLookup, request.RequestRecords),
            ReadDCache(request.EnableCacheRead, request.RequestRecords, logger));

        auto response = std::allocate_shared<Response>(GetAllocator());

        const auto scoringStartTime = std::chrono::high_resolution_clock::now();

        bond::blob responseBlob;

        if ((*m_config)->MinervaAcsHost().UseMinervaAcsHost() || request.UseMinervaAcsHost)
        {
            responseBlob = co_await GetAcsScoringResponse(cbond::SerializeCompactBinaryToBlob(request), logger);
        }
        else
        {
            responseBlob = co_await WinAPI::Parallel::Launch(
                [&, requestBlob{cbond::SerializeCompactBinaryToBlob(request)}]() noexcept
                {
                    assert(m_scoring);
                    return m_scoring(request.BatchId, requestBlob, logger);
                })(WinAPI::Parallel::resume_environment, WinAPI::Parallel::environment_arg, &m_scoringEnv);
        }

        if (!responseBlob.empty()) [[likely]]
        {
            cbond::DeserializeCompactBinary(*response, responseBlob);
        }

        if (response->Status == ScoringOperationStatus::ModelLoading)
        {
            m_perfCounter->Increment(ServerCounters::ToCounterId(ServerCounters::BIScoringServerCounters::ModelLoading));
            co_return response;
        }

        m_perfCounter->Set(ServerCounters::ToCounterId(ServerCounters::BIScoringServerCounters::ScoringLatency), ElapsedTime(scoringStartTime));

#if 0   // Disable ETW logging until a solution for handling sensitive user data is implemented
        if (m_etwLogger) [[likely]]
        {
            try
            {
                if (!request.RequestRecords.empty() && GeneCommon::Utils::ShouldSample(request.RequestRecords[0].RGUID, m_sampleRate))
                {
                    m_etwLogger(WarmUp::Serialize(sdr::ext::View<const LogRecord&>{request, *response}, GetAllocator()));
                }
            }
            catch (const std::exception& e)
            {
                ADS_APUTILITIES_LOG_THROTTLED_WARNING(logger, "ETW Write LogRecord failed: %s", e.what());
            }
        }
#endif

        m_perfCounter->Add(
            ServerCounters::ToCounterId(ServerCounters::BIScoringServerCounters::EventsCompletedPerSec),
            eventsSize,
            request.TrafficType.c_str());
        SetScoringLatency(*response, m_perfCounter);

        if (request.EnableCacheWrite && m_dCacheClient && !request.RequestRecords.empty())
        {
            m_dCacheClient->Write(
                response->ResponseRecords | std::views::transform(&ResponseRecord::RGUID),
                response->ResponseRecords | std::views::transform(
                    [](const auto& record) noexcept
                    {
                        return RemoteHashStore::Client::Value{ record.FraudInfo.content(), record.FraudInfo.size() };
                    }));
        }

        m_perfCounter->Set(ServerCounters::ToCounterId(ServerCounters::BIScoringServerCounters::RequestLatency), ElapsedTime(requestStartTime));
        co_return response;
    }

    WinAPI::Parallel::Result<> ReadRemoteStatsTable(bool enableRead, std::vector<RequestRecord>& requestRecords)
    {
        if (!enableRead)
        {
            co_return;
        }

        // TODO: Use std::views::concat after migrating to C++26
        auto concat = [](RequestRecord& rec, std::vector<RequestRecord>& refs)
        {
            return std::views::iota(0u, 1 + refs.size())
                | std::views::transform(
                    [&](std::size_t i) noexcept -> auto&
                    {
                        return i == 0 ? rec : refs[i - 1];
                    });
        };

        auto mergedRecords = requestRecords
            | std::views::transform(
                [&concat](RequestRecord& rec)
                {
                    return concat(rec, rec.References);
                })
            | std::views::join;

        for (auto& requestRecord : mergedRecords)
        {
            requestRecord.FeatureValues.resize(requestRecord.FeatureKeys.size());
        }

        auto input = mergedRecords
            | std::views::transform(&RequestRecord::FeatureKeys)
            | std::views::join
            | std::views::transform(
                [](const FeatureKey& key) noexcept
                {
                    return RemoteHashStore::Client::Key{
                        reinterpret_cast<const std::uint8_t*>(key.MainFeatureKeyBlob.content()),
                        static_cast<std::uint32_t>(key.MainFeatureKeyBlob.length())};
                });

        auto output = mergedRecords
            | std::views::transform(&RequestRecord::FeatureValues)
            | std::views::join
            | std::views::transform(&FeatureValue::MainFeatureValueBlob);

        assert(std::ranges::distance(input) == std::ranges::distance(output));

        if (std::ranges::distance(input) > 0) [[likely]]
        {
            (void)co_await m_store->ReadOneTableData(input, output.begin(), m_featuresTableName, GetAllocator());
        }
    }

    WinAPI::Parallel::Result<> ReadDCache(bool enableRead, std::vector<RequestRecord>& requestRecords, const APUtilities::ILogger& logger)
    {
        try
        {
            if (enableRead && m_dCacheClient && !requestRecords.empty())
            {
                auto output = requestRecords | std::views::transform(&RequestRecord::FraudInfo);
                (void)co_await m_dCacheClient->Read(
                    requestRecords | std::views::transform(&RequestRecord::RGUID),
                    output.begin(),
                    GetAllocator());
            }
        }
        catch (const std::exception& e)
        {
            ADS_APUTILITIES_LOG_THROTTLED_ERROR(logger, "DCache Read failed: %s", e.what());
        }
    }

    WinAPI::Parallel::Result<bond::blob> GetAcsScoringResponse(
        bond::blob requestBlob,
        const APUtilities::ILogger& logger)
    {
        if (!m_minervaAcsHostClient) [[unlikely]]
        {
            ADS_APUTILITIES_LOG_UNTHROTTLED_ERROR(logger, "MinervaAcsHost Client is not available.");
            co_return bond::blob{};
        }

        co_return co_await m_minervaAcsHostClient->GetPartition(1).ProcessRequest(
            std::allocator_arg, GetAllocator(), requestBlob);

    }

    IArenaStl GetAllocator()
    {
        return m_chainedAllocatorPool.Take();
    }

    std::shared_ptr<RemoteIDHashStore> m_store;
    std::optional<DCacheClient> m_dCacheClient;
    std::shared_ptr<Ads::Communication::Bootstrap::ClientHost> m_minervaAcsHostClient;
    std::shared_ptr<APUtilities::ApPerfCounter> m_perfCounter{std::make_shared<APUtilities::ApPerfCounter>(
        APPERFCOUNTER_DEFINE_MAP(ServerCounters::c_perfCategory, BIScoringPERFCOUNTER_SEQ))};
    WinAPI::ThreadPool::Environment m_scoringEnv;
    ScoringFunc m_scoring;
    std::function<void(const bond::blob&)> m_etwLogger;
    std::uint8_t m_sampleRate{(*m_config)->EtwLogging().SampleRate()
        ? *(*m_config)->EtwLogging().SampleRate()
        : (*m_config)->EtwLogging().SampleRate_default_value()};
    std::string m_featuresTableName{(*m_config)->IDHashClient().TableName()};
    ExponentialExpansionChainedAllocatorPool m_chainedAllocatorPool;
};

Application::~Application() = default;

Application::Application(
    std::unique_ptr<Configuration> config,
    ScoringFunc scoring,
    std::function<void(const bond::blob&)> etwLogger,
    std::shared_ptr<WinAPI::ThreadPool> threadPool,
    std::shared_ptr<WinAPI::ThreadPool> scoringThreadPool,
    std::shared_ptr<OlsLogger> logger,
    std::optional<DCacheClient> dCacheClient,
    std::shared_ptr<Ads::Communication::Bootstrap::ClientHost> minervaAcsHostClient,
    std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> remoteStoreClient,
    const ApCountersFactory<GeneCommon::GeneCounters>& apCountersFactory)
    : m_impl{std::make_unique<Impl>(
        std::move(config),
        std::move(scoring),
        std::move(etwLogger),
        std::move(threadPool),
        std::move(scoringThreadPool),
        std::move(logger),
        std::move(dCacheClient),
        std::move(minervaAcsHostClient),
        std::move(remoteStoreClient),
        apCountersFactory)}
{}

void Application::ProcessRequestAsync(
    Request request,
    SharedAsyncRequestContext<Response> asyncContext,
    const APUtilities::ILogger& logger)
{
    m_impl->ProcessRequestAsync(std::move(request), asyncContext, logger);
}

} // namespace Ads::BIScoring
