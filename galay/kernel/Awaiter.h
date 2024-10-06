#ifndef GALAY_AWAIT_H
#define GALAY_AWAIT_H

#include "Coroutine.h"

namespace galay::event
{
    class WaitEvent;
}

namespace galay::action
{
enum ActionType
{
    kActionToAddEvent,
    kActionToModEvent,
    kActionToDelEvent,
};

class WaitAction
{
public:
    virtual bool HasEventToDo() = 0;
    virtual bool DoAction(coroutine::Coroutine* co) = 0;
    virtual void ResetEvent(event::WaitEvent* event, bool is_heap) = 0;
    virtual void ResetActionType(ActionType type) = 0;
    virtual event::WaitEvent* GetBindEvent() = 0;
    virtual ~WaitAction() = default;
};
}

namespace galay::coroutine
{

class Awaiter_int
{
public:
    Awaiter_int(action::WaitAction* action);
    Awaiter_int(int result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    int await_resume() const noexcept;
    Coroutine* GetCoroutine();
private:
    int m_result;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};

class Awaiter_bool
{
public:
    Awaiter_bool(action::WaitAction* action);
    Awaiter_bool(bool result);
    bool await_ready() const noexcept;
    //true will suspend, false will not
    bool await_suspend(std::coroutine_handle<Coroutine::promise_type> handle) noexcept;
    bool await_resume() const noexcept;
    Coroutine* GetCoroutine();
private:
    bool m_result;
    action::WaitAction* m_action;
    std::coroutine_handle<Coroutine::promise_type> m_coroutine_handle;
};




}

#endif
