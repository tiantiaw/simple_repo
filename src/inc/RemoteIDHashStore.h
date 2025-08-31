#pragma once
#include <PublicRemoteHashStoreInterface.h>
#include <APUtilities/IPerfCounter.h>
#include <GeneCommon/AsyncUtils.h>
#include <GeneCommon/GeneArena.h>

#include <WinAPI/ThreadPool/Parallel/Result.h>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

namespace Ads::BIScoring
{
class RemoteIDHashStore
{
public:
    RemoteIDHashStore(
        std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> client,
        std::shared_ptr<APUtilities::IPerfCounter> counters) noexcept;

    template <std::weakly_incrementable Output>
        requires std::indirectly_writable<Output, bond::blob>
    WinAPI::Parallel::Result<bool> ReadOneTableData(
        std::ranges::input_range auto&& inputKeys,
        Output outputValues,
        const std::string& tableName,
        const DE::GeneCommon::IArenaStl& alloc)
    {
        auto keys = std::views::common(inputKeys);
        auto assignValues = [outputValues{ std::move(outputValues) }, alloc](
            std::span<const RemoteHashStore::Client::Value> values,
            std::shared_ptr<RemoteHashStore::Client::IStoreContext> storeContext) 
        {
            std::ranges::copy(
                values | std::views::transform(
                    [storeContext{ boost::allocate_shared<std::shared_ptr<RemoteHashStore::Client::IStoreContext>>(alloc, std::move(storeContext)) }](
                        const RemoteHashStore::Client::Value& val)
                    {
                        switch (val.m_status)
                        {
                        using enum RemoteHashStore::Client::Value::Status;
                        case Succeed:
                        case InCache:
                            if (val.IsValid()) [[likely]]
                            {
                                return bond::blob{ boost::shared_ptr<const char>{ storeContext, val.GetData() }, val.GetSize() };
                            }
                            [[fallthrough]];
                        [[unlikely]] case Timeout:
                        [[unlikely]] case UnexpectedError:
                            throw std::runtime_error{ "At least one feature key lookup failed: Timeout or UnexpectedError." };
                        default:
                            return bond::blob{};
                        }
                    }),
                outputValues);
        };
        return ReadOneTableDataImpl(std::allocator_arg, alloc, { keys.begin(), keys.end() }, tableName, std::move(assignValues));
    }

private:
    WinAPI::Parallel::Result<bool> ReadOneTableDataImpl(
        std::allocator_arg_t,
        DE::GeneCommon::IArenaStl alloc,
        std::vector<RemoteHashStore::Client::Key> input,
        std::string tableName,
        std::function<void(
            std::span<const RemoteHashStore::Client::Value>,
            std::shared_ptr<RemoteHashStore::Client::IStoreContext>)> output);

    void UpdateCounter(const RemoteHashStore::Client::Value& val, const char* tableName);

    std::shared_ptr<APUtilities::IPerfCounter> m_counters;
    std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> m_client;
};
} // namespace Ads::BIScoring
