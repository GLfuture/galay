#include "ArgsParse.hpp"
#include "String.h"
#include <sstream>
#include <iostream>
namespace args
{
Option::Option(const std::string &description, const std::function<void(Command*, const std::string&)>& callback)
    : m_description(description), m_callback(callback)
{
}

void Option::UseThisOption(Command *appcmds, const std::string& input)
{
    m_hasParsed = true;
    m_appcmds = appcmds;
    m_input = input;
}

void Option::ExecuteCallback()
{
    m_callback(m_appcmds, m_input);
}

bool Option::HasParsed() const
{
    return m_hasParsed;
}

Command::Command(const std::string& name, const std::string &description)
    : m_name(name), m_description(description)
{
}

std::string Command::Name() const
{
    return m_name;
}

void Command::ToBeFailed()
{
    
}

void Command::CleanUp()
{
    m_description.clear();
    m_options.clear();
    m_subCommands.clear();
    m_requiredOptions.clear();
}

bool Command::AddOption(const std::string &flag, const std::string &description, const std::function<void(Command*, const std::string&)> &callback, bool isRequired)
{
    auto flags = galay::utils::StringSplitter::SpiltWithChar(flag, ',');
    if(flags.size() < 2) return false;
    if(flags.size() != 2 || flags[0].length() != 2) return false;
    char shortFlag = flags[0][1];
    std::string longFlag = flags[1].substr(2);
    m_flags[shortFlag].push_back(longFlag);
    m_options.emplace(longFlag, std::make_shared<Option>(description, callback));
    if(isRequired) m_requiredOptions.emplace(longFlag);
    return true;
}

bool Command::AddSubCommand(Command::ptr command)
{
    command->m_parent = this;
    m_subCommands.emplace(command->Name(), command);
    return true;
}

std::string Command::ToString() const
{
    std::stringstream ss;
    for(auto& subCommand: m_subCommands) {

    }
    return "";
}

bool Command::IsAllRequiredOptionsParsed() const
{
    for(auto& option : m_requiredOptions) {
        if(!m_options.at(option)->HasParsed()) return false;
    }
    return true;
}

bool Command::CmdsParse(int argc, const char *argv[])
{
    if(argc < 2) return IsAllRequiredOptionsParsed();
    if(argv[1][0] != '-') {
        return m_subCommands.at(argv[1])->CmdsParse(argc - 1, argv + 1);
    }
    for(int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if(arg.length() < 2) return false;
        if(arg[0] == '-') {
            if(arg[1] == '-') {
                std::string_view longFlag = arg.substr(2);
                if(longFlag.find('=') != std::string_view::npos) {
                    std::string_view value = arg.substr(arg.find('=') + 1);
                    longFlag = longFlag.substr(0, arg.find('='));
                    auto it = m_options.find(longFlag.cbegin());
                    if( it != m_options.end() ) {
                        if( i < argc - 1 && argv[i + 1][0] != '-' ) {
                            return false;
                        } else {
                            it->second->UseThisOption(this, value.cbegin());
                            m_usedOptions.insert(it->second);
                        }
                    } else {
                        return false;
                    }
                } else {
                    auto it = m_options.find(longFlag.cbegin());
                    if( it != m_options.end() ) {
                        if( i < argc - 1 && argv[i + 1][0] != '-' ) {
                            it->second->UseThisOption(this, argv[i + 1]);
                            m_usedOptions.insert(it->second);
                            ++i;
                        } else {
                            it->second->UseThisOption(this, "");
                            m_usedOptions.insert(it->second);
                        }
                    } else {
                        return false;
                    }
                }
            } else if ( isalpha(arg[1]) ){
                if( arg.size() > 2 ) {
                    for(int j = 1; j < arg.length(); ++j) {
                        if(!isalpha(arg[j])) return false;
                    }
                    for(int j = 1; j < arg.length(); ++j) {
                        auto it = m_flags.find(arg[j]);
                        if( it != m_flags.end() && it->second.size() == 1) {
                            m_options[it->second[0]]->UseThisOption(this, "");
                            m_usedOptions.insert(m_options[it->second[0]]);
                        } else {
                            return false;
                        }
                    }
                } 
                else 
                {
                    auto it = m_flags.find(arg[1]);
                    if (it != m_flags.end() && it->second.size() == 1) 
                    {
                        if( i < argc - 1 && argv[i + 1][0] != '-' ) {
                            m_options[it->second[0]]->UseThisOption(this, argv[i + 1]);
                            m_usedOptions.insert(m_options[it->second[0]]);
                            ++i;
                        } else {
                            m_options[it->second[0]]->UseThisOption(this, "");
                            m_usedOptions.insert(m_options[it->second[0]]);
                        }
                    } 
                    else
                    {
                        return false;
                    } 
                }
            }
        } 
        else 
        {
            return false;
        }
    }
    return IsAllRequiredOptionsParsed();
}

void Command::OptionsExecute()
{
    for( auto& option: m_usedOptions) {
        option->ExecuteCallback();
    }
}

AppCmds::ptr AppCmds::Create(const std::string &title, const std::string &description)
{
    return std::make_shared<AppCmds>(title, description);
}

AppCmds::AppCmds(const std::string &title, const std::string &description)
    : Command(title, description)
{
}

bool AppCmds::AppCmdsParse(int argc, const char *argv[])
{
    return CmdsParse(argc, argv);
}

void AppCmds::ExecuteAllParsedCmds()
{
    return OptionsExecute();
}

}