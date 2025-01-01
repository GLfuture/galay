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
#include "String.h"

namespace args
{

class SubCommand;
class CmdArgsParser;

enum class CmdType
{
    None,
    Input,
    Output,
    InputAndOutput,
};


class Option
{
    friend class SubCommand;
public:
    using ptr = std::shared_ptr<Option>;
    Option(CmdType type, const std::function<void(SubCommand*, Option*)>& callback);
    std::string_view GetValue() const;
    ~Option() = default;
private:
    void UseThisOption(SubCommand* appcmds, const std::string &input);
    void ExecuteCallback();
    bool HasParsed() const;
private:
    CmdType m_ioType;
    bool m_hasParsed = false;
    char m_shortName = ' ';
    SubCommand* m_appcmds;
    std::string m_input;
    std::function<void(SubCommand*, Option*)> m_callback;
};

class SubCommand
{
    friend class Option;
    friend class CmdArgsParser;
public:
    using option_callback_t = std::function<void(SubCommand*, Option*)>;
    using ptr = std::shared_ptr<SubCommand>;
    SubCommand(const std::string& name, const std::string& description);
    std::string Name() const;
    void ToBeFailed();
    void CleanUp();
    
    template<CmdType Type = CmdType::None>
    bool AddOption(const std::string& flag, const option_callback_t& callback, bool isRequired = false);
    bool AddSubCommand(const std::string& subCommandDesc, SubCommand::ptr subCommand);

    size_t GetActiveOptionCount() const;
    virtual std::string ToString() const;
    virtual ~SubCommand() = default;
protected:
    bool IsAllAlpha(std::string_view str);
    bool IsAllRequiredOptionsParsed() const;
    bool CmdsParse(int argc, const char* argv[]);
    bool CmdsParseFailed();
    void OptionsExecute();
    void AddToChooseOptions(Option* option);
protected:
    std::string m_name;
    std::string m_description;
    SubCommand* m_parent = nullptr;

    std::map<std::string, std::vector<std::string>> m_subCommandsType;
    std::map<std::string, SubCommand::ptr> m_subCommands;

    //short to long
    std::unordered_map<char, std::vector<std::string>> m_flags;
    std::map<std::string, Option::ptr> m_options;

    std::unordered_set<std::string> m_requiredOptions;

    std::vector<Option*> m_usedOptions;
};

class CmdArgsParser: public SubCommand
{
public:
    using ptr = std::shared_ptr<CmdArgsParser>;
    static CmdArgsParser::ptr Create(const std::string& description);

    CmdArgsParser(const std::string& description);
    bool AppCmdsParse(int argc, const char* argv[]);
    void ExecuteAllParsedCmds();

    std::string ToString() const override;

};

}

#include "ArgsParse.tcc"


#endif