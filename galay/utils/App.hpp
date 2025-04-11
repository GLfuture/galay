#ifndef GALAY_APP_HPP
#define GALAY_APP_HPP

#include <list>
#include <memory>
#include <string>
#include <concepts>
#include <optional>
#include <stdexcept>
#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace galay::args
{

class App;
class Cmd;
class Arg;

enum InputType {
    InputInt,
    InputFloat,
    InputDouble,
    InputString
};

template<typename T>
concept ArgInputAcceptType = requires()
{
    std::is_same_v<T, int> || std::is_same_v<T, float> || std::is_same_v<T, double> || \
    std::is_same_v<T, std::string> || std::is_same_v<T, uint32_t>;
};


class ArgInputType
{
public:
    ArgInputType(Arg* arg);
    void IsInt();
    void IsFloat();
    void IsDouble();
    void IsString();
private:
    Arg* m_arg;
};

class InputValue
{
public:
    InputValue(Arg* arg);
    template<ArgInputAcceptType T>
    T ConvertTo() const;
private:
    Arg* m_arg;
};

//short name 相同的，后面的覆盖前面的
class Arg
{
    friend class Cmd;
    friend class App;
    friend class ArgInputType;
    friend class InputValue;
public:
    using ptr = std::shared_ptr<Arg>;
    static Arg::ptr Create(const std::string& name);

    Arg(const std::string& name);
    Arg& Short(char short_name);
    //default input type is String
    ArgInputType Input(bool input);
    Arg& Output(const std::string& output);
    Arg& Required(bool required);
    Arg& Unique(bool unique);
    Arg& Success(std::function<void(Arg*)>&& callback);
    //callback [in] errmsg, replace print
    Arg& Failure(std::function<void(std::string)>&& callback);
    // replace Failure's callback
    Arg& PrintError();

    InputValue Value();
private:
    Cmd* m_cmd = nullptr;
    std::string m_name;
    std::string m_short_name;
    bool m_required = false;
    bool m_input = false;
    bool m_unique = false;
    InputType m_input_type = InputType::InputString;
    std::string m_output;
    std::optional<std::string> m_value;
    std::function<void(Arg*)> m_success_callback;
    std::function<void(std::string)> m_failure_callback;
};

struct ArgCollector
{
    Arg::ptr m_output_arg = nullptr;
    std::list<Arg::ptr> m_complete_args;
};

class Cmd
{
public:
    using uptr = std::unique_ptr<Cmd>;
    static Cmd::uptr Create(const std::string& name);
    
    Cmd(const std::string& name);
    Cmd& AddCmd(Cmd::uptr cmd);
    Cmd& AddArg(Arg::ptr arg);
    void Help(const std::string& help_str);

    void ShowHelp();
protected:
    bool Collect(Arg::ptr arg);

    bool Parse(int argc, int index, const char** argv);
protected:
    std::string m_name;

    std::unordered_set<std::string> m_required;
    std::unordered_map<std::string, Cmd::uptr> m_sub_cmds;

    std::unordered_map<std::string, Arg::ptr> m_sub_args;
    ArgCollector m_collector;
};


class App: public Cmd
{
    friend class Arg;
public:
    App(const std::string& name); 
    bool Parse(int argc, const char** argv);
    void Help(const std::string& help_str);

    void ShowHelp();
};

template <>
inline int InputValue::ConvertTo() const
{
    if(!m_arg->m_value.has_value()) throw std::runtime_error("no input value");
    try {
        return std::stoi(m_arg->m_value.value());
    } catch(...) {
        throw std::runtime_error("convert to int failed");
    }
    return 0;
}

template <>
inline uint32_t InputValue::ConvertTo() const
{
    if(!m_arg->m_value.has_value()) throw std::runtime_error("no input value");
    try {
        return std::stoi(m_arg->m_value.value());
    } catch(...) {
        throw std::runtime_error("convert to int failed");
    }
    return 0;
}

template <>
inline float InputValue::ConvertTo() const
{
    if(!m_arg->m_value.has_value()) throw std::runtime_error("no input value");
    try {
        return std::stof(m_arg->m_value.value());
    } catch(...) {
        throw std::runtime_error("convert to float failed");
    }
    return 0.0f;
}

template <>
inline double InputValue::ConvertTo() const
{
    if(!m_arg->m_value.has_value()) throw std::runtime_error("no input value");
    try {
        return std::stod(m_arg->m_value.value());
    } catch(...) {
        throw std::runtime_error("convert to double failed");
    }
    return 0.0;
}

template <>
inline std::string InputValue::ConvertTo() const
{
    if(!m_arg->m_value.has_value()) throw std::runtime_error("no input value");
    return m_arg->m_value.value();
}



}

#endif