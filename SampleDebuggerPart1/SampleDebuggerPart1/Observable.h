#pragma once

#include <functional>
#include <map>
#include <vector>
#include <utility>

template <typename Event>
struct FunctionInfo
{
    Event m_event;
    unsigned int m_vectorIndex;
};

template <typename Event, typename Ret, typename... Args>
class Observable
{

public:
    Observable() = default;
    virtual ~Observable() = default;

    template <typename Observer>
    const FunctionInfo<Event> Register(const Event &event, Observer &&observer)
    {
        m_observers[event].push_back(std::forward<Observer>(observer));

        FunctionInfo<Event> FunctionInfo{ event, m_observers[event].size() - 1 };
        return FunctionInfo;
    }

    template <typename Observer>
    const FunctionInfo<Event> Register(const Event &&event, Observer &&observer)
    {
        m_observers[std::move(event)].push_back(std::forward<Observer>(observer));

        FunctionInfo<Event> FunctionInfo{ event, m_observers[event].size() - 1 };
        return FunctionInfo;
    }

    template <typename... Parameters>
    void Notify(const Event &event, Parameters... Params) const
    {
        if (m_observers.size() > 0 && m_observers.find(event) != m_observers.end())
        {
            for (const auto &observer : m_observers.at(event))
            {
                observer(Params...);
            }
        }
    }

    const bool Remove(const FunctionInfo<Event> &functionInfo)
    {
        auto callbackVectorIter = m_observers.find(functionInfo.m_event);
        if (callbackVectorIter != m_observers.end())
        {
            auto callbackVectors = m_observers[functionInfo.m_event];
            auto callbackRemove = callbackVectors.begin() + functionInfo.m_vectorIndex;
            callbackVectors.erase(callbackRemove);
            m_observers[functionInfo.m_event] = callbackVectors;
            return true;
        }
        return false;
    }

    Observable(const Observable &) = delete;
    Observable &operator=(const Observable &) = delete;

private:
    std::map<Event, std::vector<std::function<Ret(Args...)>>> m_observers;

};