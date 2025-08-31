#include "stdafx.h"

#include <ListingGeneCommon/Utils/GenericLoggingMacros.h>
#include <Protocol_reflection.h>

#include "RemoteIDHashStore.h"
#include "RemoteIDHashStoreCounters.h"
#include "Utils.h"

#include <WinAPI/ThreadPool/Parallel/Callback.h>

#include <cassert>
#include <chrono>
#include <format>

namespace Ads::BIScoring
{
using SystemClock = std::chrono::system_clock;
using namespace RemoteIDHashStoreCounters;
using namespace RemoteHashStore::Client;

RemoteIDHashStore::RemoteIDHashStore(
    std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> client,
    std::shared_ptr<APUtilities::IPerfCounter> counters) noexcept
    : m_client{std::move(client)}
    , m_counters{std::move(counters)}
{
    assert(m_client);
}

WinAPI::Parallel::Result<bool> RemoteIDHashStore::ReadOneTableDataImpl(
    std::allocator_arg_t,
    DE::GeneCommon::IArenaStl alloc,
    std::vector<Key> keys,
    std::string tableName,
    std::function<void(
        std::span<const RemoteHashStore::Client::Value>,
        std::shared_ptr<RemoteHashStore::Client::IStoreContext>)> values)
{
    const SystemClock::time_point startTime = SystemClock::now();

    auto storeContext = m_client->CreateStoreContext();
    auto& tableContext = storeContext->GetTableContext(tableName);

    m_counters->Add(ToCounterId(Counters::RequestKeysRate), keys.size(), tableName.c_str());
    tableContext.SetKeys(std::move(keys));

    WinAPI::Parallel::Callback<std::tuple<IStoreContext&, const bool>> callback;
    if (!m_client->ReadData(*storeContext, std::ref(callback))) [[unlikely]]
    {
        throw std::runtime_error{ "RemoteIDHash internal error when ReadOneTableData" };
    }
    const auto [context, result] = co_await callback;
    assert(&context == storeContext.get());
    if (result) [[likely]]
    {
        auto& valuesRead = storeContext->GetTableContext(tableName).GetAllValues();
        for (auto i = 0; i < valuesRead.size(); ++i)
        {
            UpdateCounter(valuesRead[i], tableName.c_str());
        }
        values(std::span{valuesRead.data(), valuesRead.size()}, std::move(storeContext));
        m_counters->Set(
            ToCounterId(Counters::StoreLatency),
            std::chrono::duration<double, std::micro>(SystemClock::now() - startTime).count(),
            tableName.c_str());
    }
    else [[unlikely]]
    {
        m_counters->Increment(ToCounterId(Counters::ReadDataRateFailed), tableName.c_str());
        throw std::runtime_error{ std::format("ReadData failed for RemoteStore table [{}]", tableName) };
    }
    co_return result;
}

void RemoteIDHashStore::UpdateCounter(const RemoteHashStore::Client::Value& val, const char* tableName)
{
    if (val.m_status == Value::Status::Succeed || val.m_status == Value::Status::InCache)
    {
        m_counters->Increment(ToCounterId(Counters::RequestKeysRateSuccess), tableName);
    }
    else if (val.m_status == Value::Status::NotFound)
    {
        m_counters->Increment(ToCounterId(Counters::RequestKeysRateNotFound), tableName);
    }
    else if (val.m_status == Value::Status::Timeout)
    {
        m_counters->Increment(ToCounterId(Counters::RequestKeysRateTimeout), tableName);
    }
    else
    {
        m_counters->Increment(ToCounterId(Counters::RequestKeysRateUnexpectedError), tableName);
    }
}

} // namespace Ads::BIScoring
