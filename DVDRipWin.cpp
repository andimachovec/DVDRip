#include <AppKit.h>    // be_app

#include "DVDRipWin.h"
#include "DVDRipApp.h" // APP_NAME

DVDRipWin::DVDRipWin( const char * name, DVDRipWrapper * wrapper )
    : BWindow( WINDOW_RECT, name, B_TITLED_WINDOW,
               B_NOT_RESIZABLE | B_NOT_ZOOMABLE )
{
    fWrapper = wrapper;

    // add the main view 
    fView = new DVDRipView( Bounds(), fWrapper );
    AddChild( fView );
    
    // center the window
    BScreen screen;
    MoveTo( ( screen.Frame().Width() - Bounds().Width() ) / 2,
            ( screen.Frame().Height() - Bounds().Height() ) / 2 );
}

void DVDRipWin::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case B_ABOUT_REQUESTED:
            BAlert * alert;
            alert = new BAlert( "About DVDRip",
                                APP_NAME "\n\n"
                                "by Eric Petit <titer@videolan.org>\n"
                                "Homepage : <http://beos.titer.org/dvdrip/>",
                                "OK" );
            alert->Go();
            break;

        case RIP_BUTTON:
            if( fWrapper->fStatus < 0 )
            {
                fView->UpdateWrapper();
                if( fWrapper->Rip() )
                    fView->SetEnabled( false );
            }
            else
            {
                fView->Canceling();
                fWrapper->fCanceled = true;
            }
            break;
        
        case B_REFS_RECEIVED:
        case B_SAVE_REQUESTED:
        case UPDATE_INTERFACE:
        case RIP_DONE:
        case RIP_CANCELED:
        case UPDATE_TITLES:
        case REFRESH_VOLUMES:
        case RADIO_STDOUT:
        case RADIO_FILE:
        case BROWSE_FILE:
        case SELECT_NONE:
        case SELECT_ALL:
        case BROWSE_FOLDER:
            fView->MessageReceived( message );
            break;
        
        default:
            BWindow::MessageReceived( message );
            break;
    }
}

bool DVDRipWin::QuitRequested()
{
    be_app->PostMessage( B_QUIT_REQUESTED );
    return true;
}
