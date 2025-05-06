#ifdef INCLUDE_NLOHMANN_JSON_HPP
#include <nlohmann/json.hpp>
#endif
#include <iostream>
#include "galay/galay.hpp"



//current directory must be build/test
int main(int argc, char *argv[])
{
    galay::parser::ParserManager parserManager;
    auto parser = parserManager.CreateParser("init.conf");
    auto confParser = std::dynamic_pointer_cast<galay::parser::ConfigParser>(parser);
    std::cout << confParser->GetValueAs<std::string>("port") << std::endl;
    auto vec = confParser->GetValueAs<std::vector<std::string>>("vec");
    for (auto& v : vec)
    {
        std::cout << v << std::endl;
    }
    auto vec2 = confParser->GetValueAs<std::vector<std::string>>("vec2");
    for (auto& v : vec2)
    {
        std::cout << v << std::endl;
    }
#ifdef INCLUDE_NLOHMANN_JSON_HPP
    parser = parserManager.CreateParser("init.json");
    auto jsonParser = std::dynamic_pointer_cast<galay::parser::JsonParser>(parser);
    if(!jsonParser) {
        return -1;
    }
    auto j = std::any_cast<galay::parser::JsonValue>(jsonParser->GetValue("dependencies"));
    for(auto it = j.begin() ; it != j.end(); it++){
        std::cout << *it << std::endl;
    }
    std::cout << std::boolalpha << jsonParser->GetValue("num").has_value() << std::endl;
    std::cout << std::boolalpha << std::any_cast<bool>(jsonParser->GetValue("is_student")) << std::endl;
#endif
    return 0;
}