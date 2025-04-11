import re

STATIC_HEADER_TABLE = """1	:authority	
2	:method	GET
3	:method	POST
4	:path	/
5	:path	/index.html
6	:scheme	http
7	:scheme	https
8	:status	200
9	:status	204
10	:status	206
11	:status	304
12	:status	400
13	:status	404
14	:status	500
15	accept-charset	
16	accept-encoding	gzip, deflate
17	accept-language	
18	accept-ranges	
19	accept	
20	access-control-allow-origin	
21	age	
22	allow	
23	authorization	
24	cache-control	
25	content-disposition	
26	content-encoding	
27	content-language	
28	content-length	
29	content-location	
30	content-range	
31	content-type	
32	cookie	
33	date	
34	etag	
35	expect	
36	expires	
37	from	
38	host	
39	if-match	
40	if-modified-since	
41	if-none-match	
42	if-range	
43	if-unmodified-since	
44	last-modified	
45	link	
46	location	
47	max-forwards	
48	proxy-authenticate	
49	proxy-authorization	
50	range	
51	referer	
52	refresh	
53	retry-after	
54	server	
55	set-cookie	
56	strict-transport-security	
57	transfer-encoding	
58	user-agent	
59	vary	
60	via	
61	www-authenticate"""

def gen_enum_name(key, value):
    name = key.replace('-', '_').replace(':', '').upper()
    if value != '':
        name = name + '_' + value.replace('-', '_').replace('/', '').replace('.', '_').replace(',', '').replace(' ', '_').upper()
    return name

def gen_hpp_file(parsed_table):
    hpp_content = """
#ifndef GALAY_HTTP2_HPACK_STATIC_TABLE_HPP
#define GALAY_HTTP2_HPACK_STATIC_TABLE_HPP

#include <unordered_map>
#include <string>

namespace galay::http2 {{
enum HpackStaticHeaderKey {{
HTTP2_HEADER_UNKNOWN = 0,
{enums}
}};

const std::unordered_map<HpackStaticHeaderKey, std::pair<std::string, std::string>> HPACK_STATIC_TABLE_INDEX_TO_KV = {{
{index_to_kv}
}};

/*
    使用key和value组合键
*/
const std::unordered_map<std::string, HpackStaticHeaderKey> HPACK_STATIC_TABLE_KEY_TO_INDEX = {{
{key_to_index}
}};

extern HpackStaticHeaderKey GetIndexFromKey(const std::string& key, const std::string& value);
extern std::pair<std::string, std::string> GetKeyValueFromIndex(HpackStaticHeaderKey index);

/*
    通过key和value获取索引
*/
inline HpackStaticHeaderKey GetIndexFromKey(const std::string& key, const std::string& value) {{
    std::string rkey = key + "_" + value;
    auto iter = HPACK_STATIC_TABLE_KEY_TO_INDEX.find(rkey);
    if (iter != HPACK_STATIC_TABLE_KEY_TO_INDEX.end()) {{
        return iter->second;
    }}
    return HpackStaticHeaderKey::HTTP2_HEADER_UNKNOWN;
}}

/*
    从静态索引中获取对应key和value
*/
inline std::pair<std::string, std::string> GetKeyValueFromIndex(HpackStaticHeaderKey index) {{
    auto iter = HPACK_STATIC_TABLE_INDEX_TO_KV.find(index);
    if (iter != HPACK_STATIC_TABLE_INDEX_TO_KV.end()) {{
        return iter->second;
    }}
    return {{{{}}, {{}}}};
}}
}}
#endif
    """.format(
        enums="\n".join(
            f"    HTTP2_HEADER_{gen_enum_name(key, value)} = {index}," 
            for index, key, value in parsed_table
        ), 
        index_to_kv="\n".join(
            f"    {{HTTP2_HEADER_{gen_enum_name(key, value)}, {{\"{key}\", \"{value}\"}}}}," 
            for index, key, value in parsed_table
        ),
        key_to_index="\n".join(
            f"    {{\"{key}_{value}\", HTTP2_HEADER_{gen_enum_name(key, value)}}}," 
            for index, key, value in parsed_table
        )
    )
    hpp_content = hpp_content.rstrip(",\n") + "\n"
    with open("../galay/protocol/http2/Http2HpackStaticTable.hpp", "w") as f:
        f.write(hpp_content)

if __name__ == "__main__":
    pattern = re.compile(r"""
    ^             # 行首
    (\d+)         # 索引 (第1列)
    \t            # 制表符
    ([^\t]+)      # 键名 (第2列)
    (?:\t([^\t]*))?  # 可选的值 (第3列)
    \s*$          # 行尾可能存在的空格
    """, re.VERBOSE)

    parsed_table = []
    for line in STATIC_HEADER_TABLE.strip().split('\n'):
        if (match := pattern.match(line.strip())):  # 处理每行前后空格
            index = int(match.group(1))
            key = match.group(2)
            value = match.group(3) or ""  # 处理空值
            parsed_table.append((index, key, value))
    # 打印结果验证
    gen_hpp_file(parsed_table)
        