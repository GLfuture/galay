#include "../galay/galay.h"
#include <gtest/gtest.h>

class Base: public galay::GY_Base, public galay::common::GY_DynamicCreator<galay::GY_Base,Base>
{
public:
    virtual void print()
    {
        
    }
    
    virtual ~Base() {}
};

class Child:public Base,galay::common::GY_DynamicCreator<galay::GY_Base,Child>
{
public:
    virtual void print() override
    {
        
    }

    virtual ~Child() {}
};


TEST(ReflectionTest,CreateTest)
{
    EXPECT_NE(galay::common::GY_UserFactory<>::GetInstance()->Create(galay::util::GetTypeName<Child>()).get(),nullptr);
}