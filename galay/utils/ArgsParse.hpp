#ifndef GALAY_ARGSPARSE_HPP
#define GALAY_ARGSPARSE_HPP

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <string.h>
#include <typeinfo>
#include <functional>
#include <unordered_set>

namespace args
{

class Command;

class Option
{
public:
    using ptr = std::shared_ptr<Option>;
    Option(const std::string& description, const std::function<void(Command*, const std::string&)>& callback);
    void UseThisOption(Command* appcmds, const std::string& input);
    bool HasParsed() const;
    ~Option() = default;
private:
    bool m_hasParsed = false;
    std::string m_description;
    std::function<void(Command*, const std::string&)> m_callback;
};

class Command
{
public:
    using ptr = std::shared_ptr<Command>;
    Command(const std::string& name, const std::string& description);
    std::string Name() const;

    void CleanUp();
    /*
        flag: -c,--config,--context
    */
    bool AddOption(const std::string& flag, const std::string& description, const std::function<void(Command*, const std::string&)>& callback\
                    , bool isRequired = false);
    bool AddSubCommand(Command::ptr command);

    std::string ToString() const;
    virtual ~Command() = default;
//protected:
    bool IsAllRequiredOptionsParsed() const;
    bool CmdsParse(int argc, const char* argv[]);
protected:
    std::string m_name;
    std::string m_description;
    Command* m_parent = nullptr;
    std::map<std::string, Command::ptr> m_subCommands;
    std::unordered_set<std::string> m_requiredOptions;

    //short to long
    std::unordered_map<char, std::vector<std::string>> m_flags;
    std::map<std::string, Option::ptr> m_options;
};

class AppCmds: public Command
{
public:
    using ptr = std::shared_ptr<AppCmds>;
    static AppCmds::ptr Create(const std::string& title, const std::string& description);

    AppCmds(const std::string& title, const std::string& description);
    bool AppCmdsParse(int argc, const char* argv[]);
};

}


#endif