#pragma once

#include <sys/time.h>

class CTimer
{
public:
    /***
     * @brief Timer start record
    */
    inline void start()
    {
        gettimeofday(&startTime, NULL);
    }

    /***
     * @brief Timer stop record
     * @return: type long, Unit Microseconds
    */
    inline long end()
    {
        gettimeofday(&endTime, NULL);
        long elapsedTime = (endTime.tv_sec - startTime.tv_sec) * 1000 * 1000 +
                           (endTime.tv_usec - startTime.tv_usec);
        return elapsedTime;
    }

private:
    struct timeval startTime, endTime;
};