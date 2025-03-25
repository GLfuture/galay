#include <gtest/gtest.h>
#include "galay/protocol/Http.hpp"

using namespace galay;

TEST(HttpFormParser, RequestParser)
{
    auto request = std::make_shared<http::HttpRequest>();
    std::string msg = "POST /forms HTTP/1.1\r\n\
Content-Length: 624\r\n\
Content-Type: multipart/form-data; boundary=--------------------------225416638845415405984661\r\n\
\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"name\"\r\n\
\r\n\
adsdas\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"category\"; filename=\"token.proto\"\r\n\
Content-Type: application/octet-stream\r\n\
\r\n\
syntax = \"proto3\";\r\n\
option go_package = \"./token\";\r\n\
message TokenReq {\r\n\
}\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"id\"\r\n\
\r\n\
1.1231\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"status\"\r\n\
\r\n\
1\r\n\
----------------------------225416638845415405984661--\r\n\r\n";
    auto result = request->DecodePdu(msg);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second, (int)msg.length());
    ASSERT_TRUE(http::HttpFormDataHelper::IsFormData(request));
    std::vector<http::FormDataValue> values;
    http::HttpFormDataHelper::ParseFormData(request, values);
    ASSERT_EQ(values.size(), 4);
    ASSERT_EQ(values[0].Name(), "name");
    ASSERT_TRUE(values[0].IsString());
    ASSERT_EQ(values[0].ToString(), "adsdas");
    ASSERT_EQ(values[1].Name(), "category");
    ASSERT_TRUE(values[1].IsFile());
    auto file = values[1].ToFile();
    ASSERT_EQ(file.m_fileName, "token.proto");
    ASSERT_EQ(file.m_body, "syntax = \"proto3\";\r\noption go_package = \"./token\";\r\nmessage TokenReq {\r\n}");
    ASSERT_EQ(values[2].Name(), "id");
    ASSERT_TRUE(values[2].IsDouble());
    ASSERT_EQ(values[2].ToDouble(), 1.1231);
    ASSERT_EQ(values[3].Name(), "status"); 
    ASSERT_TRUE(values[3].IsNumber());
    ASSERT_EQ(values[3].ToNumber(), 1);
    http::HttpRequest::ptr request2 = std::make_shared<http::HttpRequest>();
    http::HttpFormDataHelper::FormDataToString(request2, "--------------------------225416638845415405984661", values);
    request2->Header()->Method() = http::HttpMethod::Http_Method_Post;
    request2->Header()->Uri() = "/forms";
    request2->Header()->Version() = http::HttpVersion::Http_Version_1_1;
    request2->Header()->HeaderPairs().AddHeaderPair("Content-Type", "multipart/form-data; boundary=--------------------------225416638845415405984661");
    ASSERT_EQ(request2->EncodePdu(), msg);
}

TEST(HttpFormParser, ResponseParser)
{
    auto response = std::make_shared<http::HttpResponse>();
    std::string msg = "HTTP/1.1 200 OK\r\n\
Content-Length: 624\r\n\
Content-Type: multipart/form-data; boundary=--------------------------225416638845415405984661\r\n\
\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"name\"\r\n\
\r\n\
adsdas\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"category\"; filename=\"token.proto\"\r\n\
Content-Type: application/octet-stream\r\n\
\r\n\
syntax = \"proto3\";\r\n\
option go_package = \"./token\";\r\n\
message TokenReq {\r\n\
}\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"id\"\r\n\
\r\n\
1.1231\r\n\
----------------------------225416638845415405984661\r\n\
Content-Disposition: form-data; name=\"status\"\r\n\
\r\n\
1\r\n\
----------------------------225416638845415405984661--\r\n\r\n";

    auto result = response->DecodePdu(msg);
    ASSERT_TRUE(result.first);
    ASSERT_EQ(result.second, (int)msg.length());
    ASSERT_EQ(response->Header()->HeaderPairs().GetValue("Content-Length"), std::string("624"));
    ASSERT_EQ(response->Header()->Version(), http::HttpVersion::Http_Version_1_1);
    ASSERT_EQ(msg, response->EncodePdu());
}


TEST(HttpFormDataHelper, ParseHeadData)
{
    std::string msg = "GET / HTTP/1.1\r\n\
Host: localhost:8080\r\n\r\n";
    http::HttpRequest request;
    request.DecodePdu(msg);
}