#include "dns.h"

galay::protocol::dns::Dns_Header
galay::protocol::dns::Dns_Protocol::GetHeader()
{
    return this->m_header;
}

void 
galay::protocol::dns::Dns_Protocol::SetHeader(Dns_Header&& header)
{
    this->m_header = header;
}

void
galay::protocol::dns::Dns_Protocol::SetQuestionQueue(::std::queue<Dns_Question> &&questions)
{
    this->m_questions = std::forward<::std::queue<Dns_Question> &&>(questions);
}



::std::queue<galay::protocol::dns::Dns_Answer>
galay::protocol::dns::Dns_Protocol::GetAnswerQueue()
{
    return std::move(this->m_answers);
}

::std::string 
galay::protocol::dns::Dns_Request::EncodePdu()
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
    ::std::string qname = ModifyHostname(question.m_qname);
    memcpy(ptr, qname.c_str(), qname.length());
    ptr += qname.length();
    *(unsigned short *)ptr = htons(question.m_type);
    ptr += 2;
    *(unsigned short *)ptr = htons(question.m_class);
    ptr += 2;
    len = len + qname.length() + 4;

    char *begin = (char *)&buffer[0];
    return ::std::string(begin, len);
}

void 
galay::protocol::dns::Dns_Request::Clear()
{
    while(!this->m_answers.empty()) this->m_answers.pop();
    while(!this->m_questions.empty()) this->m_questions.pop();
    this->m_header = {};
}


::std::string 
galay::protocol::dns::Dns_Request::ModifyHostname(::std::string hostname)
{
    ::std::vector<::std::string> temp = util::StringUtil::Spilt_With_Char(hostname, '.');
    ::std::string res;
    for (auto &v : temp)
    {
        res = res + static_cast<char>(v.length()) + v;
    }
    return res + static_cast<char>(0);
}

int 
galay::protocol::dns::Dns_Response::DecodePdu(::std::string &buffer)
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
        int len = DnsParseName((unsigned char *)begin, (unsigned char *)temp, q.m_qname);
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
        int len = DnsParseName((unsigned char *)begin, (unsigned char *)temp, a.m_aname);
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
        a.m_data = DealAnswer(a.m_type, begin, ::std::string(temp, a.m_data_len));
        temp += a.m_data_len;
        m_answers.push(a);
    }

    delete[] begin;
    buffer.clear();
    return 0;
}

bool 
galay::protocol::dns::Dns_Response::IsPointer(int in)
{
    return ((in & 0xC0) == 0xC0);
}

::std::string 
galay::protocol::dns::Dns_Response::DealAnswer(unsigned short type, char *buffer, ::std::string data)
{
    ::std::string res;
    switch (type)
    {
    case kDnsQueryA:
    {
        char ip[20] = {0};
        inet_ntop(AF_INET, data.c_str(), ip, 20);
        res.append(ip);
    }
    break;
    case kDnsQueryCname:
    {
        char *temp = new char[data.length()];
        memcpy(temp, data.c_str(), data.length());
        DnsParseName((unsigned char *)buffer, (unsigned char *)temp, res);
        delete[] temp;
    }
    break;
    case kDnsQueryAAAA:
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

int 
galay::protocol::dns::Dns_Response::DnsParseName(unsigned char *buffer, unsigned char *ptr, ::std::string &out)
{

    int flag = 0, n = 0, alen = 0;
    char temp[64];
    while (1)
    {

        flag = static_cast<int>(ptr[0]);
        if (flag == 0)
            break;

        if (IsPointer(flag))
        {
            alen += 1;
            n = (int)ptr[1];
            ptr = buffer + n;
            DnsParseName(buffer, ptr, out);
            break;
        }
        else
        {
            bzero(temp, 64);
            ptr++;
            memcpy(temp, ptr, flag);
            ptr += flag;
            alen += (flag + 1);
            out += ::std::string(temp, flag);
            if (static_cast<int>(ptr[0]) != 0)
            {
                out += '.';
            }
        }
    }
    return alen + 1;
}

void 
galay::protocol::dns::Dns_Response::Clear()
{
    while(!this->m_answers.empty()) this->m_answers.pop();
    while(!this->m_questions.empty()) this->m_questions.pop();
    this->m_header = {};
}