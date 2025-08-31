#pragma once
#include <GeneCommon/GeneArena.h>
#include <PublicRemoteHashStoreInterface.h>
#include <WinAPI/ThreadPool/Parallel/Result.h>

#include <boost/make_shared.hpp>

#include <algorithm>
#include <functional>
#include <iterator>
#include <ranges>
#include <span>
#include <vector>

namespace Ads::BIScoring
{
class DCacheClient
{
public:
    DCacheClient(std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> client) noexcept;

    WinAPI::Parallel::Result<bool> Read(
        std::ranges::input_range auto&& inputKeys,
        std::output_iterator<bond::blob> auto outputValues,
        const DE::GeneCommon::IArenaStl& alloc)
    {
        auto keys = std::views::common(inputKeys);
        auto assignValues = [outputValues{ std::move(outputValues) }, alloc](
                                std::span<const RemoteHashStore::Client::Value> values,
                                std::shared_ptr<RemoteHashStore::Client::IStoreContext> storeContext) 
        {
            std::ranges::copy(values | std::views::transform(
                [storeContext{ boost::allocate_shared<std::shared_ptr<RemoteHashStore::Client::IStoreContext>>(alloc, std::move(storeContext)) }](
                    const RemoteHashStore::Client::Value& val) noexcept
                {
                    return (val.m_status == RemoteHashStore::Client::Value::Status::Succeed 
                        || val.m_status == RemoteHashStore::Client::Value::Status::InCache) && val.IsValid()
                        ? bond::blob{ boost::shared_ptr<const char>{ storeContext, val.GetData() }, val.GetSize() }
                        : bond::blob{};
                }),
            outputValues);
        };
        return ReadImpl(std::allocator_arg, alloc, { keys.begin(), keys.end() }, std::move(assignValues));
    }

    void Write(std::ranges::input_range auto&& inputKeys, std::ranges::input_range auto&& inputValues)
    {
        auto keys = std::views::common(inputKeys);
        auto values = std::views::common(inputValues);
        WriteImpl({ keys.begin(), keys.end() }, { values.begin(), values.end() });
    }

private:
    WinAPI::Parallel::Result<bool> ReadImpl(
        std::allocator_arg_t,
        DE::GeneCommon::IArenaStl alloc,
        std::vector<RemoteHashStore::Client::Key> input,
        std::function<void(
            std::span<const RemoteHashStore::Client::Value>,
            std::shared_ptr<RemoteHashStore::Client::IStoreContext>)> output);

    void WriteImpl(std::vector<RemoteHashStore::Client::Key> keys, std::vector<RemoteHashStore::Client::Value> values);

    std::shared_ptr<RemoteHashStore::Client::IRemoteStoreClient> m_client;
};

} // Ads::BIScoring
