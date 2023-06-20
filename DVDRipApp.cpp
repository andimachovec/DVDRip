#include "DVDRipApp.h"

#include <stdlib.h>    // atoi()

DVDRipApp::DVDRipApp( int argc, char ** argv)
    : BApplication( "application/x-vnd.titer-dvdrip" )
{
    fWrapper = new DVDRipWrapper();

    fWindow = NULL;
    if( argc < 2 )
    {
        // GUI mode
        fWindow = new DVDRipWin( APP_NAME, fWrapper );
        fWindow->Show();
        return;
    }

    // default parameters
    fWrapper->fFilesInsteadOfTitles = false;
    fWrapper->fSelectedTitle = 1;
    fWrapper->fWriteToFile = true;
    fWrapper->fDestinationFile = "/boot/home/Desktop/movie";

    // parse the command line
    int index = 1;
    char * option;
    while( index < argc )
    {
        option = argv[index];
        if( option[0] == '-' && option[1] != 0 )
        {
            if( !strcmp( option + 1, "title" ) )
            {
                if( !argv[index+1] )
                {
                    fprintf( stderr, "%s: missing argument for '%s'\n",
                             argv[0], option + 1 );
                    be_app->PostMessage( B_QUIT_REQUESTED );
                    return;
                }
                else
                {
                    fWrapper->fSelectedTitle = atoi( argv[index+1] );
                    if( fWrapper->fSelectedTitle < 1 )
                    {
                        fprintf( stderr, "%s: invalid argument for 'title'\n",
                                 argv[0] );
                        be_app->PostMessage( B_QUIT_REQUESTED );
                        return;
                    }
                    index++;
                }
            }
            else if( !strcmp( option + 1, "file" ) )
            {
                if( !argv[index+1] )
                {
                    fprintf( stderr, "%s: missing argument for '%s'\n",
                             argv[0], option + 1 );
                    be_app->PostMessage( B_QUIT_REQUESTED );
                    return;
                }
                else
                {
                    fWrapper->fDestinationFile = strdup( argv[index+1] );
                    fWrapper->fWriteToFile = true;
                    index++;
                }
            }
            else if( !strcmp( option + 1, "limit" ) )
            {
                if( !argv[index+1] )
                {
                    fprintf( stderr, "%s: missing argument for '%s'\n",
                             argv[0], option + 1 );
                    be_app->PostMessage( B_QUIT_REQUESTED );
                    return;
                }
                else
                {
                    fWrapper->fLimit = atoi( argv[index+1] );
                    index++;
                }
            }
            else if( !strcmp( option + 1, "stdout" ) )
                fWrapper->fWriteToFile = false;
            else
            {
                fprintf( stderr, "%s: no such option '%s'\n",
                         argv[0], option + 1 );
                be_app->PostMessage( B_QUIT_REQUESTED );
                return;
            }
        }
        else
        {
            fprintf( stderr, "%s: syntax error\n", argv[0] );
            be_app->PostMessage( B_QUIT_REQUESTED );
            return;
        }
        index++;
    }
    
    fprintf( stderr, "Ripping title %d to %s%s, ",
             fWrapper->fSelectedTitle,
             fWrapper->fWriteToFile ? "file " : "stdout",
             fWrapper->fWriteToFile ? fWrapper->fDestinationFile : "" );
    if( fWrapper->fLimit > 0 )
        fprintf( stderr, "limit = %d MB\n", fWrapper->fLimit );
    else
        fprintf( stderr, "no limit\n" );
    
    // look for a volume
    BList * volumeslist = NULL;
    volumeslist = fWrapper->GetVolumes( true );
    if( !volumeslist || !volumeslist->CountItems() )
    {
        fprintf( stderr, "cannot find any DVD volume\n" );
        be_app->PostMessage( B_QUIT_REQUESTED );
        return;
    }
    BMenuItem * item = NULL;
    item = (BMenuItem*)volumeslist->ItemAt( 0 );
    fWrapper->fSelectedVolume = strdup( item->Label() );
    
    // look for the asked title
    BList * titleslist = NULL;
    titleslist =
        fWrapper->GetTitles( fWrapper->fSelectedVolume );
    if( !titleslist || titleslist->CountItems() < fWrapper->fSelectedTitle )
    {
        fprintf( stderr, "no title %d on the DVD volume\n",
                 fWrapper->fSelectedTitle );
        be_app->PostMessage( B_QUIT_REQUESTED );
        return;
    }
    
    // rip !
    fWrapper->Rip();
}

void DVDRipApp::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case RIP_DONE:
            if( !fWindow )
                be_app->PostMessage( B_QUIT_REQUESTED );
        
        case B_ABOUT_REQUESTED:
        case B_SAVE_REQUESTED:
        case UPDATE_INTERFACE:
        case RIP_CANCELED:
            if( fWindow )
               fWindow->MessageReceived( message );
            break;
        
        default:
            BApplication::MessageReceived( message );
    }
}

void DVDRipApp::RefsReceived( BMessage * message )
{
    fWindow->MessageReceived( message );
}
