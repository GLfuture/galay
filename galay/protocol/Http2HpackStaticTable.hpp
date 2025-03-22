
#ifndef GALAY_HTTP2_HPACK_STATIC_TABLE_HPP
#define GALAY_HTTP2_HPACK_STATIC_TABLE_HPP

#include <unordered_map>
#include <string>

namespace galay::http2 {
enum HpackStaticHeaderKey {
HTTP2_HEADER_UNKNOWN = 0,
    HTTP2_HEADER_AUTHORITY = 1,
    HTTP2_HEADER_METHOD_GET = 2,
    HTTP2_HEADER_METHOD_POST = 3,
    HTTP2_HEADER_PATH_ = 4,
    HTTP2_HEADER_PATH_INDEX_HTML = 5,
    HTTP2_HEADER_SCHEME_HTTP = 6,
    HTTP2_HEADER_SCHEME_HTTPS = 7,
    HTTP2_HEADER_STATUS_200 = 8,
    HTTP2_HEADER_STATUS_204 = 9,
    HTTP2_HEADER_STATUS_206 = 10,
    HTTP2_HEADER_STATUS_304 = 11,
    HTTP2_HEADER_STATUS_400 = 12,
    HTTP2_HEADER_STATUS_404 = 13,
    HTTP2_HEADER_STATUS_500 = 14,
    HTTP2_HEADER_ACCEPT_CHARSET = 15,
    HTTP2_HEADER_ACCEPT_ENCODING_GZIP_DEFLATE = 16,
    HTTP2_HEADER_ACCEPT_LANGUAGE = 17,
    HTTP2_HEADER_ACCEPT_RANGES = 18,
    HTTP2_HEADER_ACCEPT = 19,
    HTTP2_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN = 20,
    HTTP2_HEADER_AGE = 21,
    HTTP2_HEADER_ALLOW = 22,
    HTTP2_HEADER_AUTHORIZATION = 23,
    HTTP2_HEADER_CACHE_CONTROL = 24,
    HTTP2_HEADER_CONTENT_DISPOSITION = 25,
    HTTP2_HEADER_CONTENT_ENCODING = 26,
    HTTP2_HEADER_CONTENT_LANGUAGE = 27,
    HTTP2_HEADER_CONTENT_LENGTH = 28,
    HTTP2_HEADER_CONTENT_LOCATION = 29,
    HTTP2_HEADER_CONTENT_RANGE = 30,
    HTTP2_HEADER_CONTENT_TYPE = 31,
    HTTP2_HEADER_COOKIE = 32,
    HTTP2_HEADER_DATE = 33,
    HTTP2_HEADER_ETAG = 34,
    HTTP2_HEADER_EXPECT = 35,
    HTTP2_HEADER_EXPIRES = 36,
    HTTP2_HEADER_FROM = 37,
    HTTP2_HEADER_HOST = 38,
    HTTP2_HEADER_IF_MATCH = 39,
    HTTP2_HEADER_IF_MODIFIED_SINCE = 40,
    HTTP2_HEADER_IF_NONE_MATCH = 41,
    HTTP2_HEADER_IF_RANGE = 42,
    HTTP2_HEADER_IF_UNMODIFIED_SINCE = 43,
    HTTP2_HEADER_LAST_MODIFIED = 44,
    HTTP2_HEADER_LINK = 45,
    HTTP2_HEADER_LOCATION = 46,
    HTTP2_HEADER_MAX_FORWARDS = 47,
    HTTP2_HEADER_PROXY_AUTHENTICATE = 48,
    HTTP2_HEADER_PROXY_AUTHORIZATION = 49,
    HTTP2_HEADER_RANGE = 50,
    HTTP2_HEADER_REFERER = 51,
    HTTP2_HEADER_REFRESH = 52,
    HTTP2_HEADER_RETRY_AFTER = 53,
    HTTP2_HEADER_SERVER = 54,
    HTTP2_HEADER_SET_COOKIE = 55,
    HTTP2_HEADER_STRICT_TRANSPORT_SECURITY = 56,
    HTTP2_HEADER_TRANSFER_ENCODING = 57,
    HTTP2_HEADER_USER_AGENT = 58,
    HTTP2_HEADER_VARY = 59,
    HTTP2_HEADER_VIA = 60,
    HTTP2_HEADER_WWW_AUTHENTICATE = 61,
};

const std::unordered_map<HpackStaticHeaderKey, std::pair<std::string, std::string>> HPACK_STATIC_TABLE_INDEX_TO_KV = {
    {HTTP2_HEADER_AUTHORITY, {":authority", ""}},
    {HTTP2_HEADER_METHOD_GET, {":method", "GET"}},
    {HTTP2_HEADER_METHOD_POST, {":method", "POST"}},
    {HTTP2_HEADER_PATH_, {":path", "/"}},
    {HTTP2_HEADER_PATH_INDEX_HTML, {":path", "/index.html"}},
    {HTTP2_HEADER_SCHEME_HTTP, {":scheme", "http"}},
    {HTTP2_HEADER_SCHEME_HTTPS, {":scheme", "https"}},
    {HTTP2_HEADER_STATUS_200, {":status", "200"}},
    {HTTP2_HEADER_STATUS_204, {":status", "204"}},
    {HTTP2_HEADER_STATUS_206, {":status", "206"}},
    {HTTP2_HEADER_STATUS_304, {":status", "304"}},
    {HTTP2_HEADER_STATUS_400, {":status", "400"}},
    {HTTP2_HEADER_STATUS_404, {":status", "404"}},
    {HTTP2_HEADER_STATUS_500, {":status", "500"}},
    {HTTP2_HEADER_ACCEPT_CHARSET, {"accept-charset", ""}},
    {HTTP2_HEADER_ACCEPT_ENCODING_GZIP_DEFLATE, {"accept-encoding", "gzip, deflate"}},
    {HTTP2_HEADER_ACCEPT_LANGUAGE, {"accept-language", ""}},
    {HTTP2_HEADER_ACCEPT_RANGES, {"accept-ranges", ""}},
    {HTTP2_HEADER_ACCEPT, {"accept", ""}},
    {HTTP2_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN, {"access-control-allow-origin", ""}},
    {HTTP2_HEADER_AGE, {"age", ""}},
    {HTTP2_HEADER_ALLOW, {"allow", ""}},
    {HTTP2_HEADER_AUTHORIZATION, {"authorization", ""}},
    {HTTP2_HEADER_CACHE_CONTROL, {"cache-control", ""}},
    {HTTP2_HEADER_CONTENT_DISPOSITION, {"content-disposition", ""}},
    {HTTP2_HEADER_CONTENT_ENCODING, {"content-encoding", ""}},
    {HTTP2_HEADER_CONTENT_LANGUAGE, {"content-language", ""}},
    {HTTP2_HEADER_CONTENT_LENGTH, {"content-length", ""}},
    {HTTP2_HEADER_CONTENT_LOCATION, {"content-location", ""}},
    {HTTP2_HEADER_CONTENT_RANGE, {"content-range", ""}},
    {HTTP2_HEADER_CONTENT_TYPE, {"content-type", ""}},
    {HTTP2_HEADER_COOKIE, {"cookie", ""}},
    {HTTP2_HEADER_DATE, {"date", ""}},
    {HTTP2_HEADER_ETAG, {"etag", ""}},
    {HTTP2_HEADER_EXPECT, {"expect", ""}},
    {HTTP2_HEADER_EXPIRES, {"expires", ""}},
    {HTTP2_HEADER_FROM, {"from", ""}},
    {HTTP2_HEADER_HOST, {"host", ""}},
    {HTTP2_HEADER_IF_MATCH, {"if-match", ""}},
    {HTTP2_HEADER_IF_MODIFIED_SINCE, {"if-modified-since", ""}},
    {HTTP2_HEADER_IF_NONE_MATCH, {"if-none-match", ""}},
    {HTTP2_HEADER_IF_RANGE, {"if-range", ""}},
    {HTTP2_HEADER_IF_UNMODIFIED_SINCE, {"if-unmodified-since", ""}},
    {HTTP2_HEADER_LAST_MODIFIED, {"last-modified", ""}},
    {HTTP2_HEADER_LINK, {"link", ""}},
    {HTTP2_HEADER_LOCATION, {"location", ""}},
    {HTTP2_HEADER_MAX_FORWARDS, {"max-forwards", ""}},
    {HTTP2_HEADER_PROXY_AUTHENTICATE, {"proxy-authenticate", ""}},
    {HTTP2_HEADER_PROXY_AUTHORIZATION, {"proxy-authorization", ""}},
    {HTTP2_HEADER_RANGE, {"range", ""}},
    {HTTP2_HEADER_REFERER, {"referer", ""}},
    {HTTP2_HEADER_REFRESH, {"refresh", ""}},
    {HTTP2_HEADER_RETRY_AFTER, {"retry-after", ""}},
    {HTTP2_HEADER_SERVER, {"server", ""}},
    {HTTP2_HEADER_SET_COOKIE, {"set-cookie", ""}},
    {HTTP2_HEADER_STRICT_TRANSPORT_SECURITY, {"strict-transport-security", ""}},
    {HTTP2_HEADER_TRANSFER_ENCODING, {"transfer-encoding", ""}},
    {HTTP2_HEADER_USER_AGENT, {"user-agent", ""}},
    {HTTP2_HEADER_VARY, {"vary", ""}},
    {HTTP2_HEADER_VIA, {"via", ""}},
    {HTTP2_HEADER_WWW_AUTHENTICATE, {"www-authenticate", ""}},
};

/*
    使用key和value组合键
*/
const std::unordered_map<std::string, HpackStaticHeaderKey> HPACK_STATIC_TABLE_KEY_TO_INDEX = {
    {":authority_", HTTP2_HEADER_AUTHORITY},
    {":method_GET", HTTP2_HEADER_METHOD_GET},
    {":method_POST", HTTP2_HEADER_METHOD_POST},
    {":path_/", HTTP2_HEADER_PATH_},
    {":path_/index.html", HTTP2_HEADER_PATH_INDEX_HTML},
    {":scheme_http", HTTP2_HEADER_SCHEME_HTTP},
    {":scheme_https", HTTP2_HEADER_SCHEME_HTTPS},
    {":status_200", HTTP2_HEADER_STATUS_200},
    {":status_204", HTTP2_HEADER_STATUS_204},
    {":status_206", HTTP2_HEADER_STATUS_206},
    {":status_304", HTTP2_HEADER_STATUS_304},
    {":status_400", HTTP2_HEADER_STATUS_400},
    {":status_404", HTTP2_HEADER_STATUS_404},
    {":status_500", HTTP2_HEADER_STATUS_500},
    {"accept-charset_", HTTP2_HEADER_ACCEPT_CHARSET},
    {"accept-encoding_gzip, deflate", HTTP2_HEADER_ACCEPT_ENCODING_GZIP_DEFLATE},
    {"accept-language_", HTTP2_HEADER_ACCEPT_LANGUAGE},
    {"accept-ranges_", HTTP2_HEADER_ACCEPT_RANGES},
    {"accept_", HTTP2_HEADER_ACCEPT},
    {"access-control-allow-origin_", HTTP2_HEADER_ACCESS_CONTROL_ALLOW_ORIGIN},
    {"age_", HTTP2_HEADER_AGE},
    {"allow_", HTTP2_HEADER_ALLOW},
    {"authorization_", HTTP2_HEADER_AUTHORIZATION},
    {"cache-control_", HTTP2_HEADER_CACHE_CONTROL},
    {"content-disposition_", HTTP2_HEADER_CONTENT_DISPOSITION},
    {"content-encoding_", HTTP2_HEADER_CONTENT_ENCODING},
    {"content-language_", HTTP2_HEADER_CONTENT_LANGUAGE},
    {"content-length_", HTTP2_HEADER_CONTENT_LENGTH},
    {"content-location_", HTTP2_HEADER_CONTENT_LOCATION},
    {"content-range_", HTTP2_HEADER_CONTENT_RANGE},
    {"content-type_", HTTP2_HEADER_CONTENT_TYPE},
    {"cookie_", HTTP2_HEADER_COOKIE},
    {"date_", HTTP2_HEADER_DATE},
    {"etag_", HTTP2_HEADER_ETAG},
    {"expect_", HTTP2_HEADER_EXPECT},
    {"expires_", HTTP2_HEADER_EXPIRES},
    {"from_", HTTP2_HEADER_FROM},
    {"host_", HTTP2_HEADER_HOST},
    {"if-match_", HTTP2_HEADER_IF_MATCH},
    {"if-modified-since_", HTTP2_HEADER_IF_MODIFIED_SINCE},
    {"if-none-match_", HTTP2_HEADER_IF_NONE_MATCH},
    {"if-range_", HTTP2_HEADER_IF_RANGE},
    {"if-unmodified-since_", HTTP2_HEADER_IF_UNMODIFIED_SINCE},
    {"last-modified_", HTTP2_HEADER_LAST_MODIFIED},
    {"link_", HTTP2_HEADER_LINK},
    {"location_", HTTP2_HEADER_LOCATION},
    {"max-forwards_", HTTP2_HEADER_MAX_FORWARDS},
    {"proxy-authenticate_", HTTP2_HEADER_PROXY_AUTHENTICATE},
    {"proxy-authorization_", HTTP2_HEADER_PROXY_AUTHORIZATION},
    {"range_", HTTP2_HEADER_RANGE},
    {"referer_", HTTP2_HEADER_REFERER},
    {"refresh_", HTTP2_HEADER_REFRESH},
    {"retry-after_", HTTP2_HEADER_RETRY_AFTER},
    {"server_", HTTP2_HEADER_SERVER},
    {"set-cookie_", HTTP2_HEADER_SET_COOKIE},
    {"strict-transport-security_", HTTP2_HEADER_STRICT_TRANSPORT_SECURITY},
    {"transfer-encoding_", HTTP2_HEADER_TRANSFER_ENCODING},
    {"user-agent_", HTTP2_HEADER_USER_AGENT},
    {"vary_", HTTP2_HEADER_VARY},
    {"via_", HTTP2_HEADER_VIA},
    {"www-authenticate_", HTTP2_HEADER_WWW_AUTHENTICATE},
};

extern HpackStaticHeaderKey GetIndexFromKey(const std::string& key, const std::string& value);
extern std::pair<std::string, std::string> GetKeyValueFromIndex(HpackStaticHeaderKey index);

/*
    通过key和value获取索引
*/
inline HpackStaticHeaderKey GetIndexFromKey(const std::string& key, const std::string& value) {
    std::string rkey = key + "_" + value;
    auto iter = HPACK_STATIC_TABLE_KEY_TO_INDEX.find(rkey);
    if (iter != HPACK_STATIC_TABLE_KEY_TO_INDEX.end()) {
        return iter->second;
    }
    return HpackStaticHeaderKey::HTTP2_HEADER_UNKNOWN;
}

/*
    从静态索引中获取对应key和value
*/
inline std::pair<std::string, std::string> GetKeyValueFromIndex(HpackStaticHeaderKey index) {
    auto iter = HPACK_STATIC_TABLE_INDEX_TO_KV.find(index);
    if (iter != HPACK_STATIC_TABLE_INDEX_TO_KV.end()) {
        return iter->second;
    }
    return {{}, {}};
}
}
#endif
    
