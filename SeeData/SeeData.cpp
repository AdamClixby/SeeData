#include <iostream>
#include "DataFile.h"

using namespace std;

int main( int argc, const char *argv[], const char *envp[] )
{
    if( argc != 2 )
    {
        cout << "Usage : seedata <filename> \n";
        return 1;
    }

    string lFilename = argv[ 1 ];
    int liSlashIndex = lFilename.rfind( '/' );
    int liBackslashIndex = lFilename.rfind( '\\' );
    int liExtensionIndex = lFilename.rfind( '.' );
    if( liExtensionIndex < liSlashIndex ||
        liExtensionIndex < liBackslashIndex ||
        liExtensionIndex < 0 )
    {
        liExtensionIndex = lFilename.length();
    }

    string lBinaryOutputFilename = lFilename.substr( 0, liExtensionIndex ) + ".bin";
    string lTextOutputFilename = lFilename.substr( 0, liExtensionIndex ) + ".txt";

    cDtaFile lDataFile( argv[ 1 ] );
    if( lDataFile.GetError() )
    {
        cout << lDataFile.GetError() << "\n";
        return 2;
    }

    if( ( lDataFile.LoadedAsBinary() && !lDataFile.SaveAsText( lTextOutputFilename.c_str() ) ) ||
        ( lDataFile.LoadedAsText() && !lDataFile.SaveAsBinary( lBinaryOutputFilename.c_str() ) ) )
    {
        cout << lDataFile.GetError() << "\n";
        return 3;
    }

    std::cout << "Converted " << argv[ 1 ] << " to " << ( lDataFile.LoadedAsBinary() ? lTextOutputFilename.c_str() : lBinaryOutputFilename.c_str() ) << "\n";

    return 0;
}
