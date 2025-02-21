#ifndef GALAY_MVCC_HPP
#define GALAY_MVCC_HPP

#include <atomic>
#include <mutex>
#include <optional>
#include <shared_mutex>
#include <unordered_map>

namespace galay::mvcc
{

using Version = uint64_t;

template<typename T>
struct VersionedValue {
    Version version;
    T* value;
    VersionedValue(Version v, T* val) : version(v), value(val) {}
};

template<typename T>
class Mvcc
{
public:
    Mvcc();
    Mvcc(size_t initial_hash_size);
    std::optional<VersionedValue<T>> GetValue(Version version);
    std::optional<VersionedValue<T>> GetCurrentValue();
    bool PutValue(T* value);
    bool RemoveValue(Version version);
    // Check if the version is valid
    bool IsValid(Version old_version);
private:
    std::atomic<Version> m_current_version;
    std::shared_mutex m_mutex;
    std::unordered_map<Version, VersionedValue<T>> m_versioned_values;
};


}

#include "Mvcc.tcc"

#endif