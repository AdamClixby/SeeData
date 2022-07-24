#pragma once

#include <map>
#include <string>
#include <vector>

template <typename T>
static void WriteToBinaryStream( char*& lpStream, const T& lValue )
{
    *(T*)lpStream = lValue;
    lpStream += sizeof( T );
}

template <typename T>
static T ReadFromBinaryStream( char*& lpStream )
{
    T lValue = *(T*)lpStream;
    lpStream += sizeof( T );
    return lValue;
}

// Read the next quoted text string in the current block
static bool ReadFromTextStream( char*& lpStream, const char* lpStreamEnd, std::string& lResult )
{
    while( lpStream < lpStreamEnd &&
          *lpStream != '[' &&
          *lpStream != '{' &&
        ( *lpStream != '\"' || *(lpStream - 1) == '\\' ) )
    {
        ++lpStream;
    }

    if( *lpStream != '\"' )
    {
        return false;
    }
    ++lpStream;

    const char* lpStringStart = lpStream;
    while( lpStream < lpStreamEnd &&
        ( *lpStream != '\"' || *( lpStream - 1 ) == '\\' ) )
    {
        ++lpStream;
    }

    if( lpStream >= lpStreamEnd )
    {
        return false;
    }

    *lpStream = 0;
    lResult = lpStringStart;
    *lpStream++ = '\"';
    
    return true;
}

static bool ReadFromTextStream( char*& lpStream, const char* lpStreamEnd, int& liResult )
{
    while( lpStream < lpStreamEnd &&
        ( *lpStream < '0' || *lpStream > '9' ) &&
          *lpStream != '-' &&
          *lpStream != ',' )
    {
        ++lpStream;
    }

    if( ( *lpStream < '0' || *lpStream > '9' ) && *lpStream != '-' )
    {
        return false;
    }

    liResult = atoi( lpStream );

    while( lpStream < lpStreamEnd && ( *lpStream >= '0' && *lpStream <= '9' ) || *lpStream == '-' )
    {
        ++lpStream;
    }

    return true;
}

static bool ReadFromTextStream( char*& lpStream, const char* lpStreamEnd, float& lfResult )
{
    while( lpStream < lpStreamEnd &&
        ( *lpStream < '0' || *lpStream > '9' ) &&
        *lpStream != '-' &&
        *lpStream != ',' )
    {
        ++lpStream;
    }

    if( ( *lpStream < '0' || *lpStream > '9' ) && *lpStream != '-' )
    {
        return false;
    }

    lfResult = (float)atof( lpStream );

    while( lpStream < lpStreamEnd && ( *lpStream >= '0' && *lpStream <= '9' ) || *lpStream == '-' || *lpStream == '.' )
    {
        ++lpStream;
    }

    return true;
}

bool WriteString( char*& lpStreamPtr, const char* lpStreamEnd, const char* lpString );
bool WriteTabbedString( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth, const char* lpString );
bool WriteJsonlike( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth, const char* lpKey, const char* lpValue, bool lbShowQuotes, bool lbAddComma, bool lbAddNewLine );

bool AdvanceToCharacter( char*& lpStreamPtr, const char* lpStreamEnd, char lCharacter );

enum eNodeType {
    ENodeType_Integer0 = 0,
    ENodeType_Float = 1,
    ENodeType_Text = 2,
    ENodeType_String = 5,
    ENodeType_Integer6 = 6,
    ENodeType_Integer8 = 8,
    ENodeType_Integer9 = 9,
    ENodeType_Tree1 = 16,
    ENodeType_Tree2 = 17,
    ENodeType_Id = 18,
    ENodeType_IncludeFile = 33,
    ENodeType_Define = 35,
    ENodeType_Invalid
};

class cDataNode
{
public:
    static cDataNode* Create( eNodeType leNodeType );

    static const char* GetValueAsString( int leNodeType, bool lbUseEnumNames );
    static eNodeType   GetNodeTypeFromString( const std::string& lString );

    cDataNode( eNodeType leNodeType ) : meNodeType( leNodeType ) {}
    virtual ~cDataNode() {};

    virtual bool ReadFromBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) = 0;
    virtual bool ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd ) = 0;

    virtual bool WriteToBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) const = 0;
    virtual bool WriteToTextStream( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth ) const = 0;

    eNodeType GetNodeType() const
    {
        return meNodeType;
    }

protected:
    eNodeType meNodeType;

private:
    static void InitialiseNodeMaps();
    static std::map< std::string, eNodeType > sNodeNamesToTypes;
    static std::map< eNodeType, std::string > sNodeTypesToNames;
};

class cDataNodeArray : public cDataNode
{
public:
    cDataNodeArray( eNodeType leNodeType ) : cDataNode( leNodeType ), msNodeId( msNextNodeId++ ) {}
    virtual ~cDataNodeArray();

    virtual bool ReadFromBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) final;
    virtual bool ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd ) final;

    virtual bool WriteToBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) const final;
    virtual bool WriteToTextStream( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth ) const;

    static int msNextNodeId;

private:
    std::vector< cDataNode* > maChildren;
    short msNodeId;
};

class cDataNodeString : public cDataNode
{
public:
    cDataNodeString( eNodeType leNodeType ) : cDataNode( leNodeType ) {}

    virtual bool ReadFromBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) final;
    virtual bool ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd ) final;

    virtual bool WriteToBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) const final;
    virtual bool WriteToTextStream( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth ) const;

private:
    std::string mString;
};

template <typename T>
class cDataNodeAtomic : public cDataNode
{
public:
    cDataNodeAtomic( eNodeType leNodeType ) : cDataNode( leNodeType ) {}

    virtual bool ReadFromBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) final
    {
        mValue = ::ReadFromBinaryStream< T >( lpStreamPtr );
        return lpStreamPtr <= lpStreamEnd;
    }

    virtual bool ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd ) final
    {
        return ::ReadFromTextStream( lpStreamPtr, lpStreamEnd, mValue );
    }
    
    virtual bool WriteToBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) const final
    {
        if( lpStreamPtr + sizeof( T ) > lpStreamEnd )
        {
            return false;
        }
        ::WriteToBinaryStream( lpStreamPtr, mValue );
        return true;
    }

    virtual bool WriteToTextStream( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth ) const
    {
        const char* lpValueString = ValueAsString();
        return WriteJsonlike( lpStreamPtr, lpStreamEnd, liDepth + 1, GetValueAsString( meNodeType, true ), lpValueString, false, true, true );
    }

private:
    const char* ValueAsString() const;

    T mValue;
};

inline const char* cDataNodeAtomic< int >::ValueAsString() const
{
    static char lAsString[ 8 ];
    _itoa_s( mValue, lAsString, 10 );
    return lAsString;
}

inline const char* cDataNodeAtomic< float >::ValueAsString() const
{
    static char lAsString[ 16 ];
    sprintf_s( lAsString, "%.6f", mValue );
    return lAsString;
}
