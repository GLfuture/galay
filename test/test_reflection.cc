#include "galay/galay.h"
#include <gtest/gtest.h>

class Base: public galay::Base, public galay::common::DynamicCreator<galay::Base,Base>
{
public:
    virtual void print()
    {
        
    }
    
    virtual ~Base() {}
};

class Child:public Base,galay::common::DynamicCreator<galay::Base,Child>
{
public:
    virtual void print() override
    {
        
    }

    virtual ~Child() {}
};


TEST(ReflectionTest,CreateTest)
{
    EXPECT_NE(galay::common::UserFactory<>::GetInstance()->Create(galay::utils::GetTypeName<Child>()).get(),nullptr);
}