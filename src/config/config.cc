#include "config.h"


// void galay::Tcp_Server_Config::show()
// {
//     std::cout<<"port :"<< m_port << '(' << abi::__cxa_demangle(typeid(m_port).name(),NULL,NULL,NULL)<<")\n";
// }

// void galay::Tcp_Server_Config::show(const std::string& filepath)
// {
//     std::ofstream out(filepath.c_str(),std::ios_base::app);
//     if(out.fail()) {
//         std::cout<<"open file failed\n";
//         out.close();
//         return;
//     }
//     out<<"port :"<< m_port << '(' << abi::__cxa_demangle(typeid(m_port).name(),NULL,NULL,NULL)<<")\n";
//     out.close();
// }