
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <math.h>

class Timer
{
    timeval    m_start;
    timeval    m_end;
    char *     m_tag;
    int        stopped;
    int        counter;
public:
    Timer(const char * tag = "TEST")
    {
        counter = 0;
        stopped = 0;
        if (tag)
            m_tag = strdup(tag);
        else
            m_tag = strdup("");
        m_end.tv_sec = 0;
        m_end.tv_usec = 0;
        start();
    }
    ~Timer()
    {
        if (!stopped)
        {
            show();
        }
        if (m_tag)
        {
            free(m_tag);
            m_tag = 0;
        }
    }
    std::string msStr()
    {
        std::ostringstream out;
        if (counter)
        {
            uint32_t x = ms();
            out << m_tag << " " << x << " usec [" << (int)(x / counter) << "]" ;
        }
        else
            out << m_tag << " " << ms() << " usec" ;
        return out.str();
    }
    uint32_t ms()
    {
        if  (!stopped)
            stop();

        return ((m_end.tv_sec - m_start.tv_sec) * 1000000) 
                    + (m_end.tv_usec - m_start.tv_usec);
    }
    void show()
    {
        if (!stopped) 
            stop();
        std::cout << msStr() << "\n";
    }
    inline void start()
    {
        stopped = 0;
        gettimeofday(&m_start, NULL);
    }
    inline void stop()
    {
        gettimeofday(&m_end, NULL);
        stopped = 1;
    }
    inline void setCount(int c)
    { counter = c; };

    friend std::ostream & operator<< (std::ostream & os, Timer & );
};

std::ostream & operator<< (std::ostream & os, Timer & o)
{
    os << o.msStr();
    return  os;
}

