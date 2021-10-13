#include "DataNode.h"

bool cDataNodeAtomic< int >::ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd )
{
    return true;
}

bool cDataNodeAtomic< float >::ReadFromTextStream( char*& lpStreamPtr, const char* lpStreamEnd )
{
    return true;
}
