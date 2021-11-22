#include "DataNode.h"
#include <iostream>

std::map< std::string, eNodeType > cDataNode::sNodeNamesToTypes;
std::map< eNodeType, std::string > cDataNode::sNodeTypesToNames;

int cDataNodeArray::msNextNodeId = 1;

#define ValidateStreamWriteSize( size ) if( lpStreamPtr + size >= lpStreamEnd ) return false
#define ValidateStreamWritePtr( type )  ValidateStreamWriteSize( sizeof( type ) )
#define ValidateStreamPtr               ValidateStreamWriteSize( 0 )

bool WriteString( char*& lpStreamPtr, const char* lpStreamEnd, const char* lpString )
{
    std::string lString = lpString;
    int liStringLength = lString.length();
    ValidateStreamWriteSize( liStringLength );
    memcpy( lpStreamPtr, lString.c_str(), liStringLength );
    lpStreamPtr += liStringLength;
    return true;
}

bool WriteTabbedString( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth, const char* lpString )
{
    ValidateStreamWriteSize( liDepth );
    while( liDepth > 0 )
    {
        --liDepth;
        *lpStreamPtr++ = ' ';
        *lpStreamPtr++ = ' ';
    }
    return WriteString( lpStreamPtr, lpStreamEnd, lpString );
}

bool WriteJsonlike( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth, const char* lpKey, const char* lpValue, bool lbShowQuotes, bool lbAddComma, bool lbAddNewLine )
{
    if( !WriteTabbedString( lpStreamPtr, lpStreamEnd, liDepth, "\"" ) ) return false;
    if( !WriteString( lpStreamPtr, lpStreamEnd, lpKey ) )      return false;
    if( !WriteString( lpStreamPtr, lpStreamEnd, "\" : " ) )     return false;

    if( lbShowQuotes && !WriteString( lpStreamPtr, lpStreamEnd, "\"" ) ) return false;

    if( !WriteString( lpStreamPtr, lpStreamEnd, lpValue ) ) return false;

    if( lbShowQuotes && !WriteString( lpStreamPtr, lpStreamEnd, "\"" ) ) return false;
    if( lbAddComma && !WriteString( lpStreamPtr, lpStreamEnd, "," ) )  return false;
    if( lbAddNewLine && !WriteString( lpStreamPtr, lpStreamEnd, "\n" ) ) return false;

    return true;
}

bool AdvanceToCharacter( char*& lpStreamPtr, const char* lpStreamEnd, char lCharacter )
{
    while( lpStreamPtr < lpStreamEnd &&
          *lpStreamPtr != lCharacter )
    {
        ++lpStreamPtr;
    }

    return lpStreamPtr < lpStreamEnd && *lpStreamPtr == lCharacter;
}

#define AdvancePastCharacter( character ) if( !AdvanceToCharacter( lpStreamPtr, lpStreamEnd, character ) ) return false; ++lpStreamPtr; ValidateStreamPtr;

cDataNode* cDataNode::Create( eNodeType leNodeType )
{
    switch( leNodeType )
    {
        case ENodeType_Integer0:
        case ENodeType_Integer6:
        case ENodeType_Integer8:
        case ENodeType_Integer9:
            return new cDataNodeAtomic< int >( leNodeType );

        case ENodeType_Float:
            return new cDataNodeAtomic< float >( leNodeType );

        case ENodeType_String:
        case ENodeType_Id:
        case ENodeType_IncludeFile:
        case ENodeType_Define:
            return new cDataNodeString( leNodeType );

        case ENodeType_Tree1:
        case ENodeType_Tree2:
            return new cDataNodeArray( leNodeType );

        default: std::cout << "Error: Unknown data type " << (int)leNodeType << "\n";
            return nullptr;
    };
}

void cDataNode::InitialiseNodeMaps()
{
    if( !sNodeNamesToTypes.empty() )
    {
        return;
    }

#define Link( name, enumentry ) sNodeNamesToTypes[ std::string( name ) ] = enumentry; sNodeTypesToNames[ enumentry ] = std::string( name );
    
    Link( "int", ENodeType_Integer0 );
    Link( "int6", ENodeType_Integer6 );
    Link( "int8", ENodeType_Integer8 );
    Link( "int9", ENodeType_Integer9 );
    Link( "include", ENodeType_IncludeFile );
    Link( "define", ENodeType_Define );
    Link( "float", ENodeType_Float );
    Link( "string", ENodeType_String );
    Link( "id", ENodeType_Id );
    Link( "array", ENodeType_Tree1 );
    Link( "array_alt", ENodeType_Tree2 );
}

const char* cDataNode::GetValueAsString( int leNodeType, bool lbUseEnumNames )
{
    InitialiseNodeMaps();

    switch( leNodeType )
    {
        case ENodeType_Integer0:
        case ENodeType_Integer6:
        case ENodeType_Integer8:
        case ENodeType_Integer9:
        case ENodeType_IncludeFile:
        case ENodeType_Define:
        case ENodeType_Float:
        case ENodeType_String:
        case ENodeType_Id:
        case ENodeType_Tree1:
        case ENodeType_Tree2:
            if( lbUseEnumNames )
            {
                std::map< eNodeType, std::string >::const_iterator lResult = sNodeTypesToNames.find( (eNodeType)leNodeType );
                return lResult->second.c_str();
            }
            // Fallthrough

        default:
        {
            static char laTypeBuffer[ 7 ];
            _itoa_s( leNodeType, laTypeBuffer, 10 );
            return laTypeBuffer;
        }
    };
}

eNodeType cDataNode::GetNodeTypeFromString( const std::string& lString )
{
    cDataNode::InitialiseNodeMaps();
    
    std::map< std::string, eNodeType >::const_iterator lResult = sNodeNamesToTypes.find( lString );
    if( lResult == sNodeNamesToTypes.end() )
    {
        std::cout << "Unknown node type \"" << lString.c_str() << "\"\n";
        return ENodeType_Invalid;
    }

    return lResult->second;
}

cDataNodeArray::~cDataNodeArray()
{
    for( cDataNode* lpChild : maChildren )
    {
        delete lpChild;
    }
}

bool cDataNodeArray::ReadFromBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd )
{
    int liValue = ::ReadFromBinaryStream< int >( lpStreamPtr );
    ValidateStreamPtr;
    if( liValue != 1 )
    {
        return false;
    }

    short liNumChildren = ::ReadFromBinaryStream< short >( lpStreamPtr );
    ValidateStreamPtr;
    maChildren.reserve( liNumChildren );

    msNodeId = ::ReadFromBinaryStream< short >( lpStreamPtr );
    ValidateStreamPtr;

    for( short liChildIndex = 0; liChildIndex < liNumChildren; ++liChildIndex )
    {
        ValidateStreamPtr;

        int liNodeType = ::ReadFromBinaryStream< int >( lpStreamPtr );
        ValidateStreamPtr;

        maChildren.emplace_back( cDataNode::Create( (eNodeType)liNodeType ) );
        if( !maChildren.back() )
        {
            return false;
        }

        maChildren.back()->ReadFromBinaryStream( lpStreamPtr, lpStreamEnd );
    }

    return true;
}

bool cDataNodeArray::ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd )
{
#define GetNextTextValue( variable_name ) std::string variable_name; if( !::ReadFromTextStream( lpStreamPtr, lpStreamEnd, variable_name ) ) return false;
#define GetIntegerValue( variable_name )  int variable_name; if( !::ReadFromTextStream( lpStreamPtr, lpStreamEnd, variable_name ) ) return false;

    AdvancePastCharacter( '[' );

    while( lpStreamPtr < lpStreamEnd )
    {
        std::string lTypeName;
        if( !::ReadFromTextStream( lpStreamPtr, lpStreamEnd, lTypeName ) )
        {
            std::cout << "Failed to find node type\n";
            return false;
        }

        eNodeType leNodeType = cDataNode::GetNodeTypeFromString( lTypeName );
        if( leNodeType == ENodeType_Invalid )
        {
            std::cout << "Unknown node type " << lTypeName.c_str() << "\n";
            return false;
        }

        cDataNode* lpChildNode = cDataNode::Create( leNodeType );
        if( !lpChildNode )
        {
            return false;
        }

        maChildren.push_back( lpChildNode );
        lpChildNode->ReadFromTextStream( lpStreamPtr, lpStreamEnd );

        while( lpStreamPtr < lpStreamEnd &&
              *lpStreamPtr != '\"' &&
              *lpStreamPtr != ']' )
        {
            ++lpStreamPtr;
        }
        
        if( *lpStreamPtr == ']' )
        {
            ++lpStreamPtr;
            break;
        }
    }

    return true;
}

bool cDataNodeArray::WriteToBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) const
{
    ValidateStreamWritePtr( int );
    ::WriteToBinaryStream< int >( lpStreamPtr, 1 );

    ValidateStreamWritePtr( short );
    ::WriteToBinaryStream< short >( lpStreamPtr, (short)maChildren.size() );

    ValidateStreamWritePtr( short );
    ::WriteToBinaryStream< short >( lpStreamPtr, msNodeId );

    for( cDataNode* lpChild : maChildren )
    {
        ValidateStreamWritePtr( int );
        ::WriteToBinaryStream< int >( lpStreamPtr, lpChild->GetNodeType() );

        lpChild->WriteToBinaryStream( lpStreamPtr, lpStreamEnd );
    }

    return true;
}

bool cDataNodeArray::WriteToTextStream( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth ) const
{
#define DoWriteString( string )                             if( !WriteString( lpStreamPtr, lpStreamEnd, string ) ) return false;
#define DoWriteTabbedString( string )                       if( !WriteTabbedString( lpStreamPtr, lpStreamEnd, liDepth, string ) ) return false;
#define DoWriteJsonlike( key, val, quotes, comma, newline ) if( !WriteJsonlike( lpStreamPtr, lpStreamEnd, liDepth, key, val, quotes, comma, newline ) ) return false;

    if( liDepth == 0 )
    {
        DoWriteString( "{\n" );
    }

    ++liDepth;
    {
        //DoWriteJsonlike( "id", GetValueAsString( msNodeId, false ), false, true, true );
        DoWriteJsonlike( GetValueAsString( meNodeType, true ), "[", false, false, false );
        if( maChildren.size() )
        {
            DoWriteString( "\n" );

            for( cDataNode* lpChild : maChildren )
            {
                lpChild->WriteToTextStream( lpStreamPtr, lpStreamEnd, liDepth );
            }
        }

        DoWriteTabbedString( "],\n" );
    }

    --liDepth;
    if( liDepth == 0 )
    {
        DoWriteString( "},\n" );
    }

    return true;
}

bool cDataNodeString::ReadFromBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd )
{
    int liStringLength = ::ReadFromBinaryStream< int >( lpStreamPtr );
    ValidateStreamPtr;

    if( liStringLength > ( lpStreamEnd - lpStreamPtr ) )
    {
        return false;
    }

    char* lpString = new char[ liStringLength * 2 + 1 ];
    char* lpStringPtr = lpString;
    for( int ii = 0; ii < liStringLength; ++ii )
    {
        if( *lpStreamPtr == '\"' )
        {
            *lpStringPtr++ = '\\';
        }
        *lpStringPtr++ = *lpStreamPtr++;
    }

    *lpStringPtr = 0;
    mString = lpString;
    delete[] lpString;

    ValidateStreamPtr;

    return true;
}

bool cDataNodeString::ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd )
{
    return ::ReadFromTextStream( lpStreamPtr, lpStreamEnd, mString );
}

bool cDataNodeString::WriteToBinaryStream( char*& lpStreamPtr, const char* lpStreamEnd ) const
{
    int liStringLength = mString.length();

    ValidateStreamWritePtr( int );
    int* lpLengthPtr = (int*)lpStreamPtr;
    ::WriteToBinaryStream< int >( lpStreamPtr, liStringLength );

    if( lpStreamPtr + liStringLength > lpStreamEnd )
    {
        return false;
    }

    for( int ii = 0; ii < liStringLength && lpStreamPtr < lpStreamEnd; ++ii )
    {
        if( mString[ ii ] == '\\' && mString[ ii + 1 ] == '\"' )
        {
            (*lpLengthPtr)--;
            continue;
        }
        *lpStreamPtr++ = mString[ ii ];
    }

    return true;
}

bool cDataNodeString::WriteToTextStream( char*& lpStreamPtr, const char* lpStreamEnd, int liDepth ) const
{
    return WriteJsonlike( lpStreamPtr, lpStreamEnd, liDepth + 1, GetValueAsString( meNodeType, true ), mString.c_str(), true, true, true );
}
