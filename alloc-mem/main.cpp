// Allocate memory
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <vector>

#include "GetOptions.h"

struct USER_SETTINGS {
    int    iBlockCnt;   // Nb of block allocating
    int    iBlockSize;  // block size in [bytes]
    int    iRunTime;    // run time in [s]
};

/****************************************************************************\
    usage
    Quick help about parameters available.
\****************************************************************************/
void usage()
{
    printf("Usage:\n");
    printf("\t-h --help. Show this help\n");
    printf("\t-c --block-count. Select how many memory block to allocate. Default 100\n");
    printf("\t-s --block-size. Select the size of a block [bytes]. Default 1 MiB\n");
    printf("\t-r --run-time. Select how long the program run [s]. Default 120\n");
    printf("Example alloc-mem -c 128 -s 32000 -r 60\n");
}

/****************************************************************************\
   Set of handler function callded from the switch statement in ProcessOption handler
   - OnUsage
   - OnBlockCount
   - OnBlockSize
   - OnRunTime
\****************************************************************************/

bool OnUsage(int ch, char *optarg, struct USER_SETTINGS* pUsrSet) 
{
    usage();
    return true;
}

bool OnBlockCount(int ch, char *optarg, struct USER_SETTINGS* pUsrSet) 
{
    pUsrSet->iBlockCnt = strtol(optarg, NULL, 0);
    return false;
}

bool OnBlockSize(int ch, char *optarg, struct USER_SETTINGS* pUsrSet) 
{
    pUsrSet->iBlockSize = strtol(optarg, NULL, 0);
    return false;
}

bool OnRunTime(int ch, char *optarg, struct USER_SETTINGS* pUsrSet) 
{
    pUsrSet->iRunTime = strtol(optarg, NULL, 0);
    return false;
}

/****************************************************************************\
   Option handler provided when calling ParseCmdLine member function of CGetOptions
\****************************************************************************/
bool ProcessOption(int ch, char *optarg, void* user_ctx)
{
    bool bDone = false;
    do {
        struct USER_SETTINGS* pUsrSet = (struct USER_SETTINGS*)user_ctx;
        if(pUsrSet == NULL) {
            break;
        }
        #define ON(msg, fn) case msg: bDone = fn(ch, optarg, pUsrSet); break;
        switch(ch) {
            ON('h', OnUsage);
            ON('c', OnBlockCount);
            ON('s', OnBlockSize);
            ON('r', OnRunTime);
        default:
            printf("Try '--help' for more information.\n");
            bDone = true;
            break;
        }
        #undef ON

    } while(false);

    return bDone;
}


/****************************************************************************\
    main

    Read command line
    Allocate memory block requested
    Randomally access block each second
    When selected elapse time is done release memory and exit

    To be confirm that physical memory is available we must commit the pages. 
    For each access we have to access the block and write to it.
\****************************************************************************/
int main(int argc, char* argv[])
{
    do {
        // Section that your application must define to use CGetOptions class
        const char *short_options    = "hc:s:r:";
        struct option long_options[] = {
            {"help"         , 0 , 0 , 'h'} ,
            {"block-count"  , 1 , 0 , 'c'} ,
            {"block-size"   , 1 , 0 , 's'} ,
            {"run-time"     , 1 , 0 , 'r'} ,
            {0              , 0 , 0 , 0}       // mark end of options
        };

        USER_SETTINGS UsrSet = {
            100,            // default block count allocated
            1024*1024,      // default block size is 1 MiB
            120             // default running time, 2 min
        };

        // Create an object of CGetOption. On the constructor pass 
        // short and long options table as well as ptr to our personal 
        // setting structure where to save our parameters.
        CGetOptions opt(short_options, long_options, &UsrSet);
        bool bRet = opt.ParseCmdLine(argc, argv, ProcessOption);
        if(bRet) {
            break;  // not fully processed or help requested
        }

        printf("Program run for %d [s]. Allocating %d chunk of %d [bytes]\n", UsrSet.iRunTime, UsrSet.iBlockCnt, UsrSet.iBlockSize);

        std::vector<unsigned char*> MEM;
        for (int i=0; i<UsrSet.iBlockCnt; i++) {
            // allocate memory by chunks and use vector to keep track of them
            unsigned char* pbyData = new unsigned char [UsrSet.iBlockSize];
            if(pbyData) {
                MEM.push_back(pbyData);
                printf("Added block[%d]: %p. Total allocated %f MiB\n", (i+1), pbyData, (float)(i+1)*UsrSet.iBlockSize/1024/1024);

                // Access memory allocated to commit them in memory
                memset(pbyData, 0xA5, UsrSet.iBlockSize);

            } else {
                // Running out of memory
                printf("Running out of memory after %f MiB\n", (float)(i+1)*UsrSet.iBlockSize/1024/1024);
                break;
            }
        }

        // While program run access allocated memory
        for(int i=0; i<UsrSet.iRunTime; i++) {
            // Access randomally memory committed
            std::vector<unsigned char*>::const_iterator chunk = MEM.begin();
            for (int j=0; j<rand()%UsrSet.iBlockCnt; j++) {
                chunk++;
            }
            // Invert all the bit in memory
            unsigned char byData = *chunk[0] ^ 0xFF;
            memset(*chunk, byData, UsrSet.iBlockSize);
            printf("On sec: %d, accessing: memset(%p, 0x%X, %d)\n",i+1 , *chunk, byData, UsrSet.iBlockSize);

            sleep(1);
        }

        // Release all memory allocated
        int iNbOfAlloc = 0;
        std::vector<unsigned char*>::const_iterator it = MEM.begin();
        while(it!=MEM.end()) {  
            unsigned char* pbyData = *it;
            delete[] pbyData;
            printf("Removed: %p, entry: %d\n", pbyData, ++iNbOfAlloc);
            MEM.erase(MEM.begin());
            it = MEM.begin();
        }  
    } while(false);

    return 0;
}
