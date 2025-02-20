#include "App.hpp"
#include <iostream>


namespace galay::args 
{

args::ArgInputType::ArgInputType(Arg* arg)
    :m_arg(arg)
{
}

void args::ArgInputType::IsInt()
{
    m_arg->m_input_type = InputType::InputInt;
}

void args::ArgInputType::IsFloat()
{
    m_arg->m_input_type = InputType::InputFloat;
}

void args::ArgInputType::IsDouble()
{
    m_arg->m_input_type = InputType::InputDouble;
}

void args::ArgInputType::IsString()
{
    m_arg->m_input_type = InputType::InputString;
}


args::InputValue::InputValue(Arg *arg)
    :m_arg(arg)
{
}

Arg *Arg::Create(const std::string &name)
{
    return new Arg(name);
}

Arg::Arg(const std::string &name)
    : m_name(name)
{
}

Arg &Arg::Short(char short_name)
{
    if(m_short_name == m_name) throw std::runtime_error("short name can not be same as long name");
    m_short_name = std::string(short_name, 1);
    return *this;
}


ArgInputType Arg::Input(bool input)
{
    m_input = input;
    return ArgInputType(this);
}

Arg &Arg::Output(const std::string &output)
{
    m_output = output;
    return *this;
}

Arg &Arg::Required(bool required)
{
    m_required = required;
    return *this;
}

Arg &Arg::Success(std::function<void(Arg*)>&& callback)
{
    m_success_callback = std::move(callback);
    return *this;
}

Arg &Arg::Failure(std::function<void(std::string)> &&callback)
{
    m_failure_callback = std::move(callback);
    return *this;
}

Arg &Arg::PrintError()
{
    m_failure_callback = [](std::string errmsg) {
        std::cout << errmsg << std::endl;
    };
    return *this;
}

InputValue args::Arg::Value()
{
    return InputValue(this);
}

Cmd *Cmd::Create(const std::string &name)
{
    return new Cmd(name);
}

Cmd::Cmd(const std::string &name)
    : m_name(name)
{
}

Cmd& Cmd::AddCmd(Cmd* cmd, bool auto_free)
{
    m_sub_cmds.emplace(cmd->m_name, cmd);
    return *this;
}

Cmd& Cmd::AddArg(Arg *arg, bool auto_free)
{
    if(m_sub_args.find(arg->m_name) != m_sub_args.end()) {
        throw std::runtime_error("Arg already exists");
    }
    if(m_sub_args.find(arg->m_short_name) != m_sub_args.end()) {
        throw std::runtime_error("Arg short name already exists");
    }
    arg->m_cmd = this;
    m_sub_args[arg->m_name] = arg;
    if(!arg->m_short_name.empty()) {
        m_sub_args[arg->m_short_name] = arg;
    }
    if(arg->m_required) {
        m_required.insert(arg->m_name);
    }
    return *this;
}

void args::Cmd::Help(const std::string &help_str)
{
    Arg* arg = Arg::Create("help");
    arg->Output(help_str).Short('h');
    AddArg(arg, true);
}

void args::Cmd::ShowHelp()
{
    if(m_sub_args.find("help") == m_sub_args.end()) {
        std::cout << "Not set help string" << std::endl;
        return;
    }
    auto it = m_sub_args.find("help");
    std::cout << it->second->m_output << std::endl;
}

Cmd::~Cmd()
{
    for(auto& [_ , arg]: m_sub_args) {
        if(arg->m_auto_free) {
            delete arg;
        }
    }
    m_sub_args.clear();
    for(auto& [_, sub_cmd]: m_sub_cmds) {
        if(sub_cmd->m_auto_free) {
            delete sub_cmd;
        }
    }
}

bool Cmd::Collect(Arg *arg)
{
    if( m_collector.m_output_arg && !arg->m_output.empty()) {
        std::string msg = "--" + m_collector.m_output_arg->m_name + " does not use with --" + arg->m_name;
        if(arg->m_failure_callback) arg->m_failure_callback(msg);
        return false;
    }
    if ( !arg->m_output.empty() ) {
        m_collector.m_output_arg = arg;
    }
    m_collector.m_complete_args.push_back(arg);
    return true;
}

bool Cmd::Parse(int argc, int index, const char **argv)
{
    for(int i = index; i < argc; ++i) {
        std::string arg = argv[i];
        if(arg.length() > 2 && arg[0] == '-' && arg[1] == '-') {
            //long name
            std::string real_name = arg.substr(2);
            if(m_sub_args.find(real_name) != m_sub_args.end()) {
                Arg* arg_ptr = m_sub_args[real_name];
                m_required.erase(arg_ptr->m_name);
                //has input
                if(arg_ptr->m_input) {
                    if( i + 1 < argc && argv[i + 1][0] != '-' ) {
                        arg_ptr->m_value = argv[++i];
                        if(!Collect(arg_ptr)) {
                            return false;
                        }
                    } else {
                        if(arg_ptr->m_failure_callback) {
                            std::string errmsg = "Error: --" + arg_ptr->m_name + " need input, but no input found.";
                            arg_ptr->m_failure_callback(errmsg);
                        }
                        return false;
                    }
                } else {
                    if(!Collect(arg_ptr)) {
                        return false;
                    }
                }
            } else {
                std::cout << arg << " is invaild argument" << std::endl;
                return false;
            }
        } else if ( arg.length() > 1 && arg[0] == '-' && std::isalpha(arg[1])) {
            //short name
            std::string real_name = arg.substr(1);
            if( real_name.length() == 1) {
                //只有一个字母
                if(m_sub_args.find(real_name) != m_sub_args.end()) {
                    Arg* arg_ptr = m_sub_args[real_name];
                    m_required.erase(arg_ptr->m_name);
                    //has input
                    if(arg_ptr->m_input) {
                        if( i + 1 < argc && argv[i + 1][0] != '-' ) {
                            arg_ptr->m_value = argv[++i];
                            if(!Collect(arg_ptr)) {
                                return false;
                            }
                        } else {
                            if(arg_ptr->m_failure_callback) {
                                std::string errmsg = "Error: --" + arg_ptr->m_name + " need input, but no input found.";
                                arg_ptr->m_failure_callback(errmsg);
                            }
                            return false;
                        }
                    } else {
                        if(!Collect(arg_ptr)) {
                            return false;
                        }
                    }
                } else {
                    std::cout << arg << " is invaild argument" << std::endl;
                    return false;
                }
            } else {
                for( auto& c: real_name) {
                    if(!std::isalpha(c)) {
                        std::cout << "invaild argument -" << c << std::endl;
                        return false; 
                    }
                    std::string short_name(1, c);
                    if(m_sub_args.find(short_name) != m_sub_args.end()) {
                        Arg* arg_ptr = m_sub_args[short_name];
                        m_required.erase(arg_ptr->m_name);
                        //has input
                        if(arg_ptr->m_input) {
                            std::cout << real_name << " contains input argument: -" << short_name << std::endl; 
                            return false;
                        } else {
                            if(!Collect(arg_ptr)) {
                                return false;
                            }
                        }
                    } else {
                        std::cout << "-" << c << " is invaild argument" << std::endl;
                        return false;
                    }
                }
            }
        } else if (i == index && std::isalpha(arg[i])) {
            //cmd
            std::string real_name = arg;
            if(m_sub_cmds.find(real_name) != m_sub_cmds.end()) {
                Cmd* cmd_ptr = m_sub_cmds[real_name];
                return cmd_ptr->Parse(argc, index + 1, argv);
            } else {
                std::cout << real_name << " is invaild argument" << std::endl;
                return false;
            }
        } else {
            std::cout << "invalid argument: " << arg << std::endl;
            return false;
        }
    }
    if( !m_required.empty() ) {
        std::string required;
        for(auto& req: m_required) {
            required += "--" + req + " ";
        }
        required += "is required";
        std::cout << required << std::endl;
        return false;
    }
    for( auto& arg: m_collector.m_complete_args ) {
        if( arg->m_success_callback ) {
            arg->m_success_callback(arg);
        }
    }
    if( m_collector.m_output_arg ) {
        std::cout << m_collector.m_output_arg->m_output << std::endl;
    }
    return true;
}


App::App(const std::string &name)
    : Cmd(name)
{
}

bool App::Parse(int argc, const char **argv)
{
    bool res = Cmd::Parse(argc, 1, argv);
    if(!res) return false;
    return res;
}

void args::App::Help(const std::string &help_str)
{
    Cmd::Help(help_str);
}

void args::App::ShowHelp()
{
    Cmd::ShowHelp();
}



}