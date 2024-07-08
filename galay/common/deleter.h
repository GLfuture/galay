#ifndef GALAY_DELETER_H
#define GALAY_DELETER_H

#include <memory>

namespace galay
{
    namespace common
    {
        class Deleter
        {
        public:
            using ptr = std::shared_ptr<Deleter>;
            using wptr = std::weak_ptr<Deleter>;
            using uptr = std::unique_ptr<Deleter>;
            
        };
 
        class GY_ObjectorDeleter: public Deleter
        {
        public:
            
        };
    }
}

#endif