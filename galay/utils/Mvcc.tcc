#ifndef GALAY_MVCC_TCC
#define GALAY_MVCC_TCC

#include "Mvcc.hpp"

namespace galay::mvcc
{

template<typename T>
inline Mvcc<T>::Mvcc() {
    m_current_version.store(0);
}

template <typename T>
inline Mvcc<T>::Mvcc(size_t initial_hash_size)
{
    m_current_version.store(0);
    m_versioned_values.reserve(initial_hash_size);
}

template <typename T>
inline std::optional<VersionedValue<T>> Mvcc<T>::GetValue(Version version)
{
    std::shared_lock lock(m_mutex);
    auto it = m_versioned_values.find(version);
    if (it != m_versioned_values.end()) {
        return it->second;
    } 
    return std::nullopt;
}

template <typename T>
inline std::optional<VersionedValue<T>> Mvcc<T>::GetCurrentValue()
{
    return GetValue(m_current_version.load());
}

template <typename T>
inline bool Mvcc<T>::PutValue(T *value)
{
    Version old_version = m_current_version.load();
    if(!m_current_version.compare_exchange_strong(old_version, old_version + 1)) {
        return false;
    }
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_versioned_values.emplace(m_current_version.load(), VersionedValue<T>(old_version + 1, value));
    return true;
}

template <typename T>
inline bool Mvcc<T>::RemoveValue(Version version)
{
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    auto it = m_versioned_values.find(version);
    if (it != m_versioned_values.end()) {
        m_versioned_values.erase(it);
        return true;
    }
    return false;
}

template <typename T>
inline bool Mvcc<T>::IsValid(Version old_version)
{
    std::shared_lock lock(m_mutex);
    auto it = m_versioned_values.find(old_version);
    if (it != m_versioned_values.end()) {
        return true;
    }
    return false;
}


}


#endif