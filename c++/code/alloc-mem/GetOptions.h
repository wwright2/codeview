#ifndef __GETOPTIONS_H__
#define __GETOPTIONS_H__

#include <getopt.h>

class CGetOptions
{
public:
    CGetOptions(const char *short_options, struct option long_options[], void* usr_ctx);
    virtual ~CGetOptions();

    bool ParseCmdLine(
        int argc,
        char* argv[],
        bool (*fnProcessOption)(int ch, char *optarg, void* usr_ctx)
    );

private:
    const char*     m_short_options;
    struct option*  m_long_options;
    void*           m_user_ctx;
};

#endif // __GETOPTIONS_H__
