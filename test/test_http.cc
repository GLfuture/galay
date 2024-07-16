#include <gtest/gtest.h>
#include <map>
#include "../galay/galay.h"

using namespace galay::protocol::http;

// HttpRequestHeader Test
TEST(HttpRequestHeaderTest, TestMethod) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    std::string expected = "GET";
    header->Method() = expected;
    EXPECT_EQ(expected, header->Method());
}

TEST(HttpRequestHeaderTest, TestUri) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    std::string expected = "/index.html";
    header->Uri() = expected;
    EXPECT_EQ(expected, header->Uri());
}

TEST(HttpRequestHeaderTest, TestVersion) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    std::string expected = "HTTP/1.1";
    header->Version() = expected;
    EXPECT_EQ(expected, header->Version());
}

TEST(HttpRequestHeaderTest, TestArgs) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    std::map<std::string, std::string> expected = {{"key1", "value1"}, {"key2", "value2"}};
    header->Args() = expected;
    EXPECT_EQ(expected, header->Args());
}

TEST(HttpRequestHeaderTest, TestHeaders) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    std::map<std::string, std::string> expected = {{"key1", "value1"}, {"key2", "value2"}};
    header->Headers() = expected;
    EXPECT_EQ(expected, header->Headers());
}

TEST(HttpRequestHeaderTest, TestToString) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    header->Method() = "GET";
    header->Uri() = "/index.html";
    header->Version() = "1.1";
    std::map<std::string, std::string> args = {{"key1", "value1"}, {"key2", "value2"}};
    header->Args() = args;
    std::map<std::string, std::string> headers = {{"key1", "value1"}, {"key2", "value2"}};
    header->Headers() = headers;
    std::string expected = "GET /index.html?key1=value1&key2=value2 HTTP/1.1\r\nkey1: value1\r\nkey2: value2\r\n\r\n";
    EXPECT_EQ(expected, header->ToString());
}

TEST(HttpRequestHeaderTest, TestFromString) {
    HttpRequestHeader::ptr header = std::make_shared<HttpRequestHeader>();
    std::string input = "GET /index.html?key1=value1&key2=value2 HTTP/1.1\r\nkey1: value1\r\nkey2: value2\r\n\r\n";
    bool result = header->FromString(input);
    EXPECT_TRUE(result);
    EXPECT_EQ("GET", header->Method());
    EXPECT_EQ("/index.html", header->Uri());
    EXPECT_EQ("1.1", header->Version());
    std::map<std::string, std::string> expectedArgs = {{"key1", "value1"}, {"key2", "value2"}};
    EXPECT_EQ(expectedArgs, header->Args());
    std::map<std::string, std::string> expectedHeaders = {{"key1", "value1"}, {"key2", "value2"}};
    EXPECT_EQ(expectedHeaders, header->Headers());
}

// HttpRequest Test

TEST(Http1_1RequestTest, TestBody) {
    HttpRequest::ptr request = std::make_shared<HttpRequest>();
    std::string expected = "Hello, World!";
    request->Body() = expected;
    EXPECT_EQ(expected, request->Body());
}

// HttpResponseHeader Test
TEST(HttpResponseHeaderTest, TestVersion) {
    HttpResponseHeader::ptr header = std::make_shared<HttpResponseHeader>();
    std::string expected = "HTTP/1.1";
    header->Version() = expected;
    EXPECT_EQ(expected, header->Version());
}

TEST(HttpResponseHeaderTest, TestCode) {
    HttpResponseHeader::ptr header = std::make_shared<HttpResponseHeader>();
    int expected = 200;
    header->Code() = expected;
    EXPECT_EQ(expected, header->Code());
}

TEST(HttpResponseHeaderTest, TestHeaders) {
    HttpResponseHeader::ptr header = std::make_shared<HttpResponseHeader>();
    std::map<std::string, std::string> expected = {{"key1", "value1"}, {"key2", "value2"}};
    header->Headers() = expected;
    EXPECT_EQ(expected, header->Headers());
}

TEST(HttpResponseHeaderTest, TestToString) {
    HttpResponseHeader::ptr header = std::make_shared<HttpResponseHeader>();
    header->Version() = "1.1";
    header->Code() = 200;
    std::map<std::string, std::string> headers = {{"key1", "value1"}, {"key2", "value2"}};
    header->Headers() = headers;
    std::string expected = "HTTP/1.1 200 OK\r\nkey1: value1\r\nkey2: value2\r\n\r\n";
    EXPECT_EQ(expected, header->ToString());
}

TEST(HttpResponseHeaderTest, TestFromString) {
    HttpResponseHeader::ptr header = std::make_shared<HttpResponseHeader>();
    std::string input = "HTTP/1.1 200 OK\r\nkey1: value1\r\nkey2: value2\r\n\r\n";
    bool result = header->FromString(input);
    EXPECT_TRUE(result);
    EXPECT_EQ("1.1", header->Version());
    EXPECT_EQ(200, header->Code());
    std::map<std::string, std::string> expectedHeaders = {{"key1", "value1"}, {"key2", "value2"}};
    EXPECT_EQ(expectedHeaders, header->Headers());
}

// HttpResponse Test

TEST(Http1_1ResponseTest, TestBody) {
    HttpResponse::ptr response = std::make_shared<HttpResponse>();
    std::string expected = "Hello, World!";
    response->Body() = expected;
    EXPECT_EQ(expected, response->Body());
}