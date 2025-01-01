#ifndef GALAY_ARGPARSE_TCC
#define GALAY_ARGPARSE_TCC


#include "ArgsParse.hpp"


namespace args
{

template<CmdType Type>
inline bool SubCommand::AddOption(const std::string& flag, \
                    const option_callback_t& callback, bool isRequired)
{
    auto flags = galay::utils::StringSplitter::SpiltWithChar(flag, ',');
    if(flags[0].size() < 2) return false;
    if(flags[0][0] == '-' && flags[0][1] != '-') {
        if(flags.size() != 2) return false;
        char shortFlag = flags[0][1];
        std::string longFlag = flags[1].substr(2);
        m_flags[shortFlag].push_back(longFlag);
        auto option = std::make_shared<Option>(Type, callback);
        option->m_shortName = shortFlag;
        m_options.emplace(longFlag, option);
        if(isRequired) m_requiredOptions.emplace(longFlag);
    } else if(flags[0][0] == '-' && flags[0][1] == '-') {
        std::string longFlag = flags[0].substr(2);
        m_options.emplace(longFlag, std::make_shared<Option>(Type, callback));
        if(isRequired) m_requiredOptions.emplace(longFlag);
    } else {
        return false;
    }
    
    return true;
}



}



#endif
