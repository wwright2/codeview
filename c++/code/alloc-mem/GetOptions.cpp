#include "GetOptions.h"

CGetOptions::CGetOptions(
    const char *short_options,
    struct option long_options[],
    void* user_ctx) :
    m_short_options(short_options),
    m_long_options(long_options),
    m_user_ctx(user_ctx)
{
    //ctor
}

CGetOptions::~CGetOptions()
{
    //dtor
}

bool CGetOptions::ParseCmdLine(
    int argc,
    char* argv[],
    bool (*fnProcessOption)(int ch, char *optarg, void* usr_set)
    )
{
    int option_index = 0;
    int ch = 0;
    bool bExit = false;
    while((ch = getopt_long(argc,argv,m_short_options,m_long_options,&option_index))!=-1 && !bExit)
    {
        bExit = fnProcessOption(ch, optarg, m_user_ctx);
    }
    return bExit;
}
