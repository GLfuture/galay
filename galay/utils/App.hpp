#ifndef GALAY_APP_HPP
#define GALAY_APP_HPP

#include <list>
#include <string>
#include <optional>
#include <functional>
#include <unordered_set>
#include <unordered_map>

namespace args
{

class App;
class Cmd;


//short name 相同的，后面的覆盖前面的
class Arg
{
    friend class Cmd;
    friend class App;
public:
    static Arg* Create(const std::string& name);

    Arg(const std::string& name);
    Arg& Short(const std::string& short_name);
    Arg& Input(bool input);
    Arg& Output(const std::string& output);
    Arg& Required(bool required);
    Arg& Success(std::function<void(Arg*)>&& callback);
    //callback [in] errmsg, replace print
    Arg& Failure(std::function<void(std::string)>&& callback);
    // replace Failure's callback
    Arg& Print();

    std::optional<std::string>& Value();
private:
    bool m_auto_free = false;
    Cmd* m_cmd = nullptr;
    std::string m_name;
    std::string m_short_name;
    bool m_required = false;
    bool m_input = false;
    std::string m_output;
    std::optional<std::string> m_value;
    std::function<void(Arg*)> m_success_callback;
    std::function<void(std::string)> m_failure_callback;
};

struct ArgCollector
{
    Arg* m_output_arg = nullptr;
    std::list<Arg*> m_complete_args;
};

class Cmd
{
public:
    static Cmd* Create(const std::string& name);
    
    Cmd(const std::string& name);
    Cmd& AddCmd(Cmd* cmd, bool auto_free = false);
    Cmd& AddArg(Arg* arg, bool auto_free = false);

    ~Cmd();
protected:
    bool Collect(Arg* arg);

    bool Parse(int argc, int index, const char** argv);
protected:
    std::string m_name;

    bool m_auto_free = false;
    std::unordered_set<std::string> m_required;
    std::unordered_map<std::string, Cmd*> m_sub_cmds;

    std::unordered_map<std::string, Arg*> m_sub_args;

    ArgCollector m_collector;
};


class App: public Cmd
{
    friend class Arg;
public:
    App(const std::string& name); 
    bool Parse(int argc, const char** argv);
};

}


#endif