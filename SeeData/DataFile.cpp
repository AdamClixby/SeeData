#include "DataFile.h"
#include <iostream>
#include <vector>

cDtaFile::cDtaFile( const char* lpFilename )
    : miDataSize( 0 )
    , mpData( nullptr )
    , mpRootNode( nullptr )
{
    FILE* lpInputFile = nullptr;
    fopen_s( &lpInputFile, lpFilename, "rb" );
    if( !lpInputFile )
    {
        mLastError = "Error reading file \"" + std::string( lpFilename ) + std::string( "\"\n" );
        return;
    }

    fseek( lpInputFile, 0, SEEK_END );
    miDataSize = ftell( lpInputFile );
    fseek( lpInputFile, 0, SEEK_SET );

    mpData = new char[ miDataSize ];
    fread( mpData, miDataSize, 1, lpInputFile );
    fclose( lpInputFile );

    ParseData();
}

cDtaFile::~cDtaFile()
{
    delete mpRootNode;
    delete[] mpData;
}

void cDtaFile::ParseData()
{
    const char* lpDataEnd = mpData + miDataSize;
    char* lpDataPtr = mpData;

    delete mpRootNode;

    bool lbIsBinaryFile = ( *lpDataPtr++ == 1 );
    if( lbIsBinaryFile )
    {
        mpRootNode = cDataNode::Create( ENodeType_Tree1 );
        mbLoadedAsBinary = mpRootNode->ReadFromBinaryStream( lpDataPtr, lpDataEnd );
    }
    else
    {
        std::string lTypeName;
        if( !ReadFromTextStream( lpDataPtr, lpDataEnd, lTypeName ) )
        {
            std::cout << "Failed to find node type\n";
            return;
        }

        eNodeType leNodeType = cDataNode::GetNodeTypeFromString( lTypeName );
        if( leNodeType != ENodeType_Invalid )
        {
            mpRootNode = cDataNode::Create( leNodeType );
            mbLoadedAsText = mpRootNode->ReadFromTextStream( lpDataPtr, lpDataEnd );
        }
    }
    if( !mbLoadedAsBinary && !mbLoadedAsText )
    {
        mLastError = "Failed to parse file";
    }

    delete[] mpData;
    mpData = nullptr;
}

bool cDtaFile::SaveAsText( const char* lpFilename )
{
    if( !mpRootNode )
    {
        mLastError = "Can't save to \"" + std::string( lpFilename ) + std::string( "\", no data is loaded" );
        return false;
    }

    constexpr int kiOutputBufferSize = 1024 * 1024;
    char* lpOutput = new char[ kiOutputBufferSize ];

    char* lpDataPtr = lpOutput;
    if( !mpRootNode->WriteToTextStream( lpDataPtr, lpOutput + kiOutputBufferSize, 0 ) )
    {
        mLastError = "Output buffer too small\n";
        return false;
    }

    FILE* lpOutputFile = nullptr;
    fopen_s( &lpOutputFile, lpFilename, "wb" );
    if( !lpOutputFile )
    {
        mLastError = "Error opening file \"" + std::string( lpFilename ) + std::string( "\" for writing" );
        return false;
    }

    fwrite( lpOutput, lpDataPtr - lpOutput, 1, lpOutputFile );
    fclose( lpOutputFile );

    delete[] lpOutput;

    return true;
}

bool cDtaFile::SaveAsBinary( const char* lpFilename )
{
    if( !mpRootNode )
    {
        mLastError = "Can't save to \"" + std::string( lpFilename ) + std::string( "\", no data is loaded" );
        return false;
    }

    constexpr int kiOutputBufferSize = 1024 * 1024;
    char* lpOutput = new char[ kiOutputBufferSize ];

    char* lpDataPtr = lpOutput;
    *lpDataPtr++ = 1;

    if( !mpRootNode->WriteToBinaryStream( lpDataPtr, lpOutput + kiOutputBufferSize ) )
    {
        mLastError = "Output buffer too small\n";
        return false;
    }

    FILE* lpOutputFile = nullptr;
    fopen_s( &lpOutputFile, lpFilename, "wb" );
    if( !lpOutputFile )
    {
        mLastError = "Error opening file \"" + std::string( lpFilename ) + std::string( "\" for writing" );
        return false;
    }

    fwrite( lpOutput, lpDataPtr - lpOutput, 1, lpOutputFile );
    fclose( lpOutputFile );

    delete[] lpOutput;

    return true;
}
