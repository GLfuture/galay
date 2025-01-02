#include "ArgsParse.hpp"
#include <sstream>
#include <iostream>
#include <iomanip>

namespace args
{
Option::Option(CmdType type, const std::function<void(SubCommand*, Option*)>& callback)
    : m_ioType(type), m_callback(callback)
{
}

std::string_view Option::GetValue() const
{
    return m_input;
}

void Option::UseThisOption(SubCommand *appcmds, const std::string &input)
{
    m_hasParsed = true;
    m_appcmds = appcmds;
    m_input = input;
    appcmds->AddToChooseOptions(this);
}

void Option::ExecuteCallback()
{
    m_callback(m_appcmds, this);
}

bool Option::HasParsed() const
{
    return m_hasParsed;
}

SubCommand::SubCommand(const std::string& name, const std::string &description)
    : m_name(name),  m_description(description)
{
}

std::string SubCommand::Name() const
{
    return m_name;
}

void SubCommand::ToBeFailed()
{
    
}

void SubCommand::CleanUp()
{
    m_options.clear();
    m_subCommands.clear();
    m_requiredOptions.clear();
}



bool SubCommand::AddSubCommand(const std::string& subCommandDesc, SubCommand::ptr subCommand)
{
    subCommand->m_parent = this;
    m_subCommandsType[subCommandDesc].push_back(subCommand->Name());
    m_subCommands.emplace(subCommand->Name(), subCommand);
    return true;
}

std::string SubCommand::ToString() const
{
    return m_description;
    // std::stringstream ss;
    // ss << m_usage << std::endl << std::endl;
    // ss << m_description << std::endl << std::endl;
    // for(auto& cmd : m_subCommandsType) {
    //     ss << cmd.first << ':' << std::endl;
    //     auto subcmds = cmd.second;
    //     for(auto subcmd: subcmds) {
    //         auto it = m_subCommands.find(subcmd);
    //         ss << "  " << std::left << std::setw(15) << std::setfill(' ') << it->second->m_name << std::left << it->second->m_description << std::endl;
    //     }
    //     ss << std::endl;
    // }
    // ss << std::right << std::setw(0) << std::setfill(' ');
    // ss <<  "Options:" << std::endl;

    // for(auto option: m_options) {
    //     std::string shortName;
    //     shortName = option.second->m_shortName == ' ' ? "" : (std::string("-") + option.second->m_shortName + ',');
    //     ss << std::right << std::setw(5) << std::setfill(' ') << shortName << ' ';
    //     std::string longName = "--";
    //     longName += option.first;
    //     longName = longName + ' ' + option.second->
    //     ss << std::left << std::setw(24) << std::setfill(' ') << 
    // }

}

size_t SubCommand::GetActiveOptionCount() const
{
    return m_usedOptions.size();
}

bool SubCommand::IsAllAlpha(std::string_view str)
{
    for(auto& c : str) {
        if(!std::isalpha(c)) {
            return false;
        }
    }
    return true;
}

bool SubCommand::IsAllRequiredOptionsParsed() const
{
    for(auto& option : m_requiredOptions) {
        if(!m_options.at(option)->HasParsed()) return false;
    }
    return true;
}

bool SubCommand::CmdsParse(int argc, const char *argv[])
{
    if(argc < 2) return IsAllRequiredOptionsParsed();
    if(argv[1][0] != '-') {
        return m_subCommands.at(argv[1])->CmdsParse(argc - 1, argv + 1);
    }
    int outnum = 0, inputnum = 0; 
    for(int i = 1; i < argc; ++i) {
        std::string_view arg = argv[i];
        if(arg.length() < 2) return CmdsParseFailed();
        if(arg[0] == '-') {
            if(arg[1] == '-') {
                std::string_view option = arg.substr(2), input;
                auto pos = arg.find('=');
                if(pos != std::string_view::npos) {
                    option = arg.substr(2, pos - 2);
                    input = arg.substr(pos + 1);
                }
                if(!IsAllAlpha(option)) return CmdsParseFailed();
                auto it = m_options.find(option.cbegin());
                if(it != m_options.end()) {
                    switch (it->second->m_ioType)
                    {
                    case CmdType::None: 
                        it->second->UseThisOption(this, "");
                        break;
                    case CmdType::Input:
                    case CmdType::InputAndOutput:
                    {
                        if(!input.empty()) {
                            it->second->UseThisOption(this, input.cbegin());
                        } else {
                            if( i < argc - 1 && argv[i + 1][0] != '-') {
                                if(outnum++ > 0) return CmdsParseFailed();
                                it->second->UseThisOption(this, argv[++i]);
                            } else {
                                std::cout << argv[i] << ": option requires an argument" << std::endl;
                                return CmdsParseFailed();
                            }
                        }
                    }
                        break;
                    case CmdType::Output:
                    {
                        if(outnum++ > 0) return CmdsParseFailed();
                        it->second->UseThisOption(this, "");
                    }
                        break;
                    default:
                        return CmdsParseFailed();
                    }
                } else {
                    return CmdsParseFailed();
                }
            } else {
                std::string_view flags = arg.substr(1);
                if(!IsAllAlpha(flags)) return CmdsParseFailed();
                for(auto& flag : flags) {
                    //find long flag
                    auto it = m_flags.find(flag);
                    if(it != m_flags.end()) {
                        if(it->second.size() != 1) return CmdsParseFailed();
                        auto optionIt = m_options.find(it->second[0]);
                        auto option = optionIt->second;
                        switch (option->m_ioType)
                        {
                        case CmdType::None: 
                            option->UseThisOption(this, "");
                            break;
                        case CmdType::Input:
                        case CmdType::InputAndOutput:
                        {
                            if( flags.length() != 1) return CmdsParseFailed();
                            if( i < argc - 1 && argv[i + 1][0] != '-') {
                                if(outnum++ > 0) return CmdsParseFailed();
                                option->UseThisOption(this, argv[++i]);
                            } else {
                                std::cout << argv[i] << ": option requires an argument" << std::endl;
                                return CmdsParseFailed();
                            }
                        }
                            break;
                        case CmdType::Output:
                        {
                            if(outnum++ > 0) return CmdsParseFailed();
                            option->UseThisOption(this, "");
                        }
                            break;
                        default:
                            return CmdsParseFailed();
                        }
                    } else {
                        return CmdsParseFailed();
                    }
                }

            }
        } 
        else 
        {
            return CmdsParseFailed();
        }
    }
    return IsAllRequiredOptionsParsed();
}

void SubCommand::OptionsExecute()
{
    for( auto& option: m_usedOptions) {
        option->ExecuteCallback();
    }
}

void SubCommand::AddToChooseOptions(Option *option)
{
    m_usedOptions.push_back(option);
}

bool SubCommand::CmdsParseFailed()
{
    m_usedOptions.clear();
    return false;
}

CmdArgsParser::ptr CmdArgsParser::Create(const std::string &description)
{
    return std::make_shared<CmdArgsParser>(description);
}

CmdArgsParser::CmdArgsParser(const std::string &description)
    : SubCommand("", description)
{
}

bool CmdArgsParser::AppCmdsParse(int argc, const char *argv[])
{
    return CmdsParse(argc, argv);
}

void CmdArgsParser::ExecuteAllParsedCmds()
{
    return OptionsExecute();
}

std::string CmdArgsParser::ToString() const
{
    return m_description;
}


}