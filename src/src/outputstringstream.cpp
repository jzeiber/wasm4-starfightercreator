#include "outputstringstream.h"
#include "printf.h"
#include "wasmmalloc.h"

OutputStringStream::OutputStringStream():m_buffer{0},m_buffersize(256),m_pos(0)
{
    m_buffer=new char[m_buffersize];
}

OutputStringStream::~OutputStringStream()
{
    delete [] m_buffer;
}

void OutputStringStream::Clear()
{
    m_buffer[0]='\0';
    m_pos=0;
}

char *OutputStringStream::Buffer()
{
    return m_buffer;
}

int16_t OutputStringStream::TextLength() const
{
    return m_pos;
}

void OutputStringStream::Concat(const char *str)
{
    if(str)
    {
        int16_t i=0;
        if(m_pos>=(m_buffersize-1))
        {
            ExpandBuffer();
        }
        while(m_pos<(m_buffersize-1) && str[i])
        {
            m_buffer[m_pos++]=str[i++];
            if(m_pos>=(m_buffersize-1))
            {
                ExpandBuffer();
            }
        }
        m_buffer[m_pos]='\0';
    }
}

void OutputStringStream::ExpandBuffer()
{
    char *current=m_buffer;
    m_buffersize+=256;
    m_buffer=new char[m_buffersize];
    m_pos=0;
    Concat(current);
    delete [] current;
}

OutputStringStream &OutputStringStream::operator<<(const char *str)
{
    Concat(str);
    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const char c)
{
    if(m_pos>=(m_buffersize-1))
    {
        ExpandBuffer();
    }
    if(m_pos<(m_buffersize-1))
    {
        m_buffer[m_pos++]=c;
    }
    m_buffer[m_pos]='\0';
    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const uint32_t val)
{
    return *this << static_cast<uint64_t>(val);
}

OutputStringStream &OutputStringStream::operator<<(const int32_t val)
{
    return *this << static_cast<int64_t>(val);
}

OutputStringStream &OutputStringStream::operator<<(const uint64_t val)
{

    char buff[32];
    snprintf(buff,31,"%llu",val);
    buff[31]='\0';
    Concat(buff);

    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const int64_t val)
{

    char buff[32];
    snprintf(buff,31,"%lli",val);
    buff[31]='\0';
    Concat(buff);

    return *this;
}

OutputStringStream &OutputStringStream::operator<<(const float val)
{

    char buff[32];
    snprintf(buff,31,"%f",val);
    buff[31]='\0';
    Concat(buff);

    return *this;
}
