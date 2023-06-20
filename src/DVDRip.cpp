#include "DVDRipApp.h"

int main( int argc, char ** argv )
{
    DVDRipApp * app;
    app = new DVDRipApp( argc, argv );
    app->Run();
    delete app;
    return 0;
}
