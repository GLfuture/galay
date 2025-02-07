#include "Mvcc.h"

namespace galay::utils
{

Mvcc::Mvcc()
{
}

uint64_t Mvcc::GetVersion() const
{
    return m_version.load();
}

void Mvcc::IncVersion()
{
    m_version.fetch_add(1);
}

bool Mvcc::IsValid(uint64_t old_version) const
{
    return old_version != m_version.load();
}

}
