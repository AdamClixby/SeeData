#pragma once

#include <string>
#include "DataNode.h"

class cDtaFile
{
public:
    cDtaFile( const char* lpFilename );
    ~cDtaFile();

    bool LoadedAsBinary() const
    {
        return mbLoadedAsBinary;
    }

    bool LoadedAsText() const
    {
        return mbLoadedAsText;
    }

    const char* GetError() const
    {
        if( mLastError.empty() )
        {
            return nullptr;
        }
        return mLastError.c_str();
    }

    bool SaveAsText( const char* lpFilename );
    bool SaveAsBinary( const char* lpFilename );

private:
    void ParseData();

    std::string    mLastError;
    char* mpData;
    cDataNode*     mpRootNode;
    int            miDataSize;

    bool mbLoadedAsBinary = false;
    bool mbLoadedAsText = false;
};

