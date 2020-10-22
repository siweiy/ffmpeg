#pragma once

#include <stdio.h>
#include "flv_fmt_encap.h"

class AV_Flv
{

public:
    AV_Flv();
    ~AV_Flv();

    /**
     * @brief Open 'flv' file
     * @param flv_name: only open flv file
    */
    bool Open(const char *flv_name);

    /**
     * @brief External get header information
     * @return type T_FLV_HEADER
    */  
    T_FLV_HEADER FlvHeader();

    /**
     * @brief Get header information
     * @return type T_FLV_HEADER
    */     
    bool TagHeader(T_FLV_TAG_HEADER *tagHeader);

    /**
     * @brief Get header information
     * @return type T_FLV_HEADER
    */  
    bool TagBody(unsigned char **pTagBody, bool toShow=false);

    /**
     * @brief Internal close file descriptor
    */ 
    void Close();

private:
    /**
     * @brief Inside get header information
     * @return true is success 
    */  
    bool flvHeader();

private:
    FILE *m_pFile;
    T_FLV_HEADER m_flvHeader;
    T_FLV_TAG_HEADER m_tagHeader;
};
