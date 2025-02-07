#ifndef GALAY_MVCC_H
#define GALAY_MVCC_H

#include <atomic>

namespace galay::utils
{

class Mvcc
{
public:
    Mvcc();
    uint64_t GetVersion() const;
    void IncVersion();
    bool IsValid(uint64_t old_version) const;
private:
    std::atomic_uint64_t m_version;
};



}

#endif