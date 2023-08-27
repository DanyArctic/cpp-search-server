#pragma once
#include <map>
#include <mutex>
#include <vector>

using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap
{
private:
    struct Bucket
    {
        std::mutex mutex;
        std::map<Key, Value> map;
    };
    std::vector<Bucket> buckets_;

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access
    {
        std::lock_guard <std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            : guard(bucket.mutex)
            , ref_to_value(bucket.map[key])
        {}
    };

    explicit ConcurrentMap(size_t bucket_amount)
        : buckets_(bucket_amount)
    {}

    Access operator[](const Key& key)
    {
        auto& bucket = buckets_[static_cast<unsigned int>(key) % buckets_.size()];
        return { key, bucket };
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_)
        {
            std::lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }
        return result;
    }

    void Erase(const Key& key)
    {
        const unsigned int key_index = static_cast<unsigned int>(key) % buckets_.size();
        std::lock_guard <std::mutex> guard(buckets_[key_index].mutex);
        buckets_[key_index].map.erase(key);
        
    }
};
