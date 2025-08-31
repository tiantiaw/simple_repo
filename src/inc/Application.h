#pragma once

#pragma warning(push)
#include "BIScoringTraits.h"
#include "DCacheClient.h"
#include "Utils.h"

#include <GeneCommon/Factory.h>
#include <Server/AsyncRequestContextPool.h>

#include <bond/core/blob.h>
#include <functional>
#include <memory>
#include <string_view>
#pragma warning(pop)

namespace Ads::BIScoring
{
using ScoringFunc = std::function<bond::blob(std::string_view batchId, const bond::blob& request, const APUtilities::ILogger& logger)>;

class Configuration;

class Application
{
public:
    explicit Application(
        std::unique_ptr<Configuration> config,
        ScoringFunc scoring,
        std::function<void(const bond::blob&)> etwLogger = {},
        std::shared_ptr<WinAPI::ThreadPool> threadPool = {},
        std::shared_ptr<WinAPI::ThreadPool> scoringThreadPool = {},
        std::shared_ptr<DE::GeneCommon::OlsLogger> logger = {},
        std::optional<DCacheClient> dCacheClient = {},
        std::shared_ptr<Ads::Communication::Bootstrap::ClientHost> minervaAcsHostClient = {},
        std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> remoteStoreClient = {},
        const DE::GeneCommon::ApCountersFactory<DE::GeneCommon::GeneCounters>& apCountersFactory =
            DE::GeneCommon::MakeApCounters<DE::GeneCommon::GeneCounters>);

    ~Application();

    void ProcessRequestAsync(
        Request request,
        DE::GeneCommon::SharedAsyncRequestContext<Response> asyncContext,
        const APUtilities::ILogger& logger);

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};
} // namespace Ads::BIScoring
