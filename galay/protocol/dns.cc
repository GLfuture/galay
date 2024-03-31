#include "dns.h"

galay::protocol::Dns_Header &galay::protocol::Dns_Protocol::get_header()
{
    return this->m_header;
}

std::queue<galay::protocol::Dns_Question> &galay::protocol::Dns_Protocol::get_question_queue()
{
    return this->m_questions;
}

std::queue<galay::protocol::Dns_Answer> &galay::protocol::Dns_Protocol::get_answer_queue()
{
    return this->m_answers;
}

std::string galay::protocol::Dns_Request::encode()
{
    unsigned char buffer[MAX_UDP_LENGTH];
    bzero(buffer, MAX_UDP_LENGTH);
    unsigned char *ptr = buffer;
    unsigned short id = htons(m_header.m_id);
    memcpy(ptr, &id, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    unsigned short flag = 0;
    flag |= (m_header.m_flags.m_qr << 15);
    flag |= (m_header.m_flags.m_opcode << 11);
    flag |= (m_header.m_flags.m_aa << 10);
    flag |= (m_header.m_flags.m_tc << 9);
    flag |= (m_header.m_flags.m_rd << 8);
    flag |= (m_header.m_flags.m_ra << 7);
    flag |= (m_header.m_flags.m_zero << 4);
    flag |= (m_header.m_flags.m_rcode);
    flag = htons(flag);
    memcpy(ptr, &flag, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    m_header.m_questions = 1;
    unsigned short questions = htons(m_header.m_questions);
    memcpy(ptr, &questions, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    unsigned short answers_RRs = htons(m_header.m_answers_RRs);
    memcpy(ptr, &answers_RRs, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    unsigned short authority_RRs = htons(m_header.m_authority_RRs);
    memcpy(ptr, &authority_RRs, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    unsigned short addtional_RRs = htons(m_header.m_additional_RRs);
    memcpy(ptr, &addtional_RRs, sizeof(unsigned short));
    ptr += sizeof(unsigned short);
    int len = sizeof(Dns_Header); // 12

    Dns_Question question = m_questions.front();
    m_questions.pop();
    std::string qname = modify_hostname(question.m_qname);
    memcpy(ptr, qname.c_str(), qname.length());
    ptr += qname.length();
    *(unsigned short *)ptr = htons(question.m_type);
    ptr += 2;
    *(unsigned short *)ptr = htons(question.m_class);
    ptr += 2;
    len = len + qname.length() + 4;

    char *begin = (char *)&buffer[0];
    return std::string(begin, len);
}

std::string galay::protocol::Dns_Request::modify_hostname(std::string hostname)
{
    std::vector<std::string> temp = StringUtil::Spilt_With_Char(hostname, '.');
    std::string res;
    for (auto &v : temp)
    {
        res = res + static_cast<char>(v.length()) + v;
    }
    return res + static_cast<char>(0);
}

int galay::protocol::Dns_Response::decode(const std::string &buffer)
{
    char *begin = new char[buffer.length()];
    char *temp = begin;
    bzero(temp, buffer.length());
    memcpy(temp, buffer.c_str(), buffer.length());
    unsigned short id;
    memcpy(&id, temp, sizeof(unsigned short));
    temp += sizeof(unsigned short);
    m_header.m_id = ntohs(id);
    unsigned short flag;
    memcpy(&flag, temp, sizeof(unsigned short));
    temp += sizeof(unsigned short);
    flag = ntohs(flag);
    m_header.m_flags.m_qr = (flag >> 15) & 0x01;
    m_header.m_flags.m_opcode = (flag >> 11) & 0x0F;
    m_header.m_flags.m_aa = (flag >> 10) & 0x01;
    m_header.m_flags.m_tc = (flag >> 9) & 0x01;
    m_header.m_flags.m_rd = (flag >> 8) & 0x01;
    m_header.m_flags.m_ra = (flag >> 7) & 0x01;
    m_header.m_flags.m_zero = (flag >> 4) & 0x07;
    m_header.m_flags.m_rcode = flag & 0x0F;
    unsigned short questions;
    memcpy(&questions, temp, sizeof(unsigned short));
    temp += sizeof(unsigned short);
    m_header.m_questions = ntohs(questions);
    unsigned short answers_RRs;
    memcpy(&answers_RRs, temp, sizeof(unsigned short));
    temp += sizeof(unsigned short);
    m_header.m_answers_RRs = ntohs(answers_RRs);
    unsigned short authority_RRs;
    memcpy(&authority_RRs, temp, sizeof(unsigned short));
    temp += sizeof(unsigned short);
    m_header.m_authority_RRs = ntohs(authority_RRs);
    unsigned short additional_RRs;
    memcpy(&additional_RRs, temp, sizeof(unsigned short));
    temp += sizeof(unsigned short);
    m_header.m_additional_RRs = ntohs(additional_RRs);

    for (int i = 0; i < m_header.m_questions; i++)
    {
        Dns_Question q;
        int len = dns_parse_name((unsigned char *)begin, (unsigned char *)temp, q.m_qname);
        temp += (len);
        unsigned short qtype;
        memcpy(&qtype, temp, sizeof(unsigned short));
        temp += sizeof(unsigned short);
        q.m_type = ntohs(qtype);
        unsigned short qclass;
        memcpy(&qclass, temp, sizeof(unsigned short));
        temp += sizeof(unsigned short);
        q.m_class = ntohs(qclass);
        m_questions.push(q);
    }

    for (int i = 0; i < m_header.m_answers_RRs; i++)
    {
        Dns_Answer a;
        int flag = static_cast<int>(temp[0]);
        int len = dns_parse_name((unsigned char *)begin, (unsigned char *)temp, a.m_aname);
        temp += len;
        unsigned short atype;
        memcpy(&atype, temp, sizeof(unsigned short));
        temp += sizeof(unsigned short);
        a.m_type = ntohs(atype);
        unsigned short aclass;
        memcpy(&aclass, temp, sizeof(unsigned short));
        temp += sizeof(unsigned short);
        a.m_class = ntohs(aclass);
        unsigned int ttl;
        memcpy(&ttl, temp, sizeof(unsigned int));
        temp += sizeof(unsigned int);
        a.m_ttl = ntohl(ttl);
        unsigned short datalen;
        memcpy(&datalen, temp, sizeof(unsigned short));
        temp += sizeof(unsigned short);
        a.m_data_len = ntohs(datalen);
        a.m_data = deal_answer(a.m_type, begin, std::string(temp, a.m_data_len));
        temp += a.m_data_len;
        m_answers.push(a);
    }

    delete[] begin;
    return 0;
}

bool galay::protocol::Dns_Response::is_pointer(int in)
{
    return ((in & 0xC0) == 0xC0);
}

std::string galay::protocol::Dns_Response::deal_answer(unsigned short type, char *buffer, std::string data)
{
    std::string res;
    switch (type)
    {
    case DNS_QUERY_A:
    {
        char ip[20] = {0};
        inet_ntop(AF_INET, data.c_str(), ip, 20);
        res.append(ip);
    }
    break;
    case DNS_QUERY_CNAME:
    {
        char *temp = new char[data.length()];
        memcpy(temp, data.c_str(), data.length());
        dns_parse_name((unsigned char *)buffer, (unsigned char *)temp, res);
        delete[] temp;
    }
    break;
    case DNS_QUERY_AAAA:
    {
        char ip6[40] = {0};
        inet_ntop(AF_INET6, data.c_str(), ip6, 40);
        res.append(ip6);
    }
    break;
    default:
        break;
    }
    return res;
}