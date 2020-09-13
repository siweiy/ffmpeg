#pragma once

#include <sys/time.h>

class CTimer
{
public:
    inline void start()
    {
        gettimeofday(&startTime, nullptr);
    }

    inline double end()
    {
        gettimeofday(&endTime, nullptr);
        double elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000 * 1000 +
                        (endTime.tv_usec - startTime.tv_usec);
        return elapsedTime;
    }

private:
    struct timeval startTime, endTime;
};