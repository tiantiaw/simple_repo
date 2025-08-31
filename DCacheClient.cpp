#include "stdafx.h"
#include "DCacheClient.h"

#include <WinAPI/ThreadPool/Parallel/Callback.h>

#include <cassert>

namespace Ads::BIScoring
{
using namespace RemoteHashStore::Client;

constexpr std::string_view c_pvScoreStoreName{"BIScoringCache"};

DCacheClient::DCacheClient(std::shared_ptr<IRemoteStoreClient> client) noexcept
    : m_client{ std::move(client) }
{
    assert(m_client);
}

WinAPI::Parallel::Result<bool> DCacheClient::ReadImpl(
    std::allocator_arg_t, 
    DE::GeneCommon::IArenaStl,
    std::vector<Key> keys,
    std::function<void(std::span<const Value>, std::shared_ptr<IStoreContext>)> values)
{
    auto storeContext = m_client->CreateStoreContext();
    auto& tableContext = storeContext->GetTableContext(c_pvScoreStoreName.data());

    tableContext.SetKeys(std::move(keys));

    WinAPI::Parallel::Callback<std::tuple<IStoreContext&, const bool>> callback;
    if (!m_client->ReadData(*storeContext, std::ref(callback))) [[unlikely]]
    {
        throw std::runtime_error{ "DCache internal error when ReadData" };
    }
    const auto [context, result] = co_await callback;
    assert(&context == storeContext.get());
    if (result) [[likely]]
    {
        auto& valuesRead = storeContext->GetTableContext(c_pvScoreStoreName.data()).GetAllValues();
        values(std::span{valuesRead.data(), valuesRead.size()}, std::move(storeContext));
    }
    co_return result;
}

void DCacheClient::WriteImpl(std::vector<Key> keys, std::vector<Value> values)
{
    auto storeContext = m_client->CreateStoreContext();
    auto& tableContext = storeContext->GetTableContext(c_pvScoreStoreName.data());

    tableContext.SetKeys(std::move(keys));
    tableContext.SetValues(std::move(values));

    if (!m_client->WriteDataSync(std::move(storeContext))) [[unlikely]]
    {
        throw std::runtime_error{ "DCacheClient internal error when WriteData" };
    }
}

} // namespace Ads::BIScoring

