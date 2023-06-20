#include "DVDRipView.h"

DVDRipView::DVDRipView( BRect frame, DVDRipWrapper * wrapper )
    : BView( frame, "View", B_FOLLOW_ALL, B_WILL_DRAW )
{
    fWrapper = wrapper;

    SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    // add tabs
    fTitlesView = new DVDRipTitlesView( BRect( Bounds().left,Bounds().top,
                                               Bounds().right,Bounds().bottom-90 ),
                                        fWrapper );
    fFilesView = new DVDRipFilesView( BRect( Bounds().left,Bounds().top,
                                             Bounds().right,Bounds().bottom-90 ),
                                      fWrapper );
    fTitlesView->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
    fFilesView->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );

    fTabView = new BTabView( BRect( Bounds().left,Bounds().top+10,
                                    Bounds().right,Bounds().bottom-80 ),
                            "TabView" );
    
    fTitlesTab = new BTab();
    fTabView->AddTab( fTitlesView, fTitlesTab );
    fTitlesTab->SetLabel( "Titles" );
    
    fFilesTab = new BTab();
    fTabView->AddTab( fFilesView, fFilesTab );
    fFilesTab->SetLabel( "Files" );
    
    AddChild( fTabView );
    
    // add status bar
    fStatusBar = new BStatusBar( BRect( Bounds().left+10, Bounds().bottom-75,
                                        Bounds().right-10,Bounds().bottom-45 ),
                                 "Status", NULL, NULL );
    AddChild( fStatusBar );
    
    // add buttons
    fRipButton = new BButton( BRect( Bounds().right-90,Bounds().bottom-35,
                                     Bounds().right-10,Bounds().bottom-10 ),
                              NULL, "Rip !", new BMessage( RIP_BUTTON ) );
    AddChild( fRipButton );
    fRefreshButton = new BButton( BRect( Bounds().right-180,Bounds().bottom-35,
                                        Bounds().right-100,Bounds().bottom-10 ),
                                  NULL, "Refresh", new BMessage( REFRESH_VOLUMES ) );
    AddChild( fRefreshButton );
    fAboutButton = new BButton( BRect( Bounds().right-270,Bounds().bottom-35,
                                      Bounds().right-190,Bounds().bottom-10 ),
                                NULL, "About", new BMessage( B_ABOUT_REQUESTED ) );
    AddChild( fAboutButton );
    
    // refresh pop-ups
    fTitlesView->Refresh( true );
    fFilesView->Refresh( false );
}

void DVDRipView::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case UPDATE_INTERFACE:
        {
            // update the slider
            char string[1024];
            memset( string, 0, 1024 );
            if( fWrapper->fStatus == 0 )
            {
                sprintf( string, "Opening..." );
                fStatusBar->LockLooper();
                fStatusBar->Reset();
                fStatusBar->SetMaxValue( 1.0 );
                fStatusBar->UnlockLooper();
            }    
            else if( fWrapper->fTotal > 1024 * 1024 )
                sprintf( string, "Total: %d MB - Speed: %.2f MB/s",
                         (int)( fWrapper->fTotal / ( 1024 * 1024 ) ),
                         fWrapper->fSpeed );
            else if( fWrapper->fTotal > 1024 )
                sprintf( string, "Total: %d KB - Speed: %.2f MB/s",
                         (int)( fWrapper->fTotal / 1024 ),
                         fWrapper->fSpeed );
            else
                sprintf( string, "Total: %d bytes - Speed: %.2f MB/s",
                         (int)fWrapper->fTotal,
                         fWrapper->fSpeed );
            
            char trailingstring[1024];
            memset( trailingstring, 0, 1024 );
            if( fWrapper->fStatus > 0 )
                sprintf( trailingstring, "%.2f %%", fWrapper->fStatus * 100 );
            
            fStatusBar->LockLooper();
            fStatusBar->Update( fWrapper->fStatus - fStatusBar->CurrentValue(),
                                string, trailingstring );
            fStatusBar->UnlockLooper();
            break;
        }
        
        case RIP_DONE:
            fStatusBar->LockLooper();
            fStatusBar->Update( 0, NULL, "Done." );
            fStatusBar->UnlockLooper();
            SetEnabled( true );
            break;
            
        case RIP_CANCELED:
            fStatusBar->LockLooper();
            fStatusBar->Update( 0, NULL, "Canceled." );
            fStatusBar->UnlockLooper();
            SetEnabled( true );
            break;

        case REFRESH_VOLUMES:
            fTitlesView->Refresh( true );
            fFilesView->Refresh( false );
            break;
            
        case B_SAVE_REQUESTED:
        case UPDATE_TITLES:
        case RADIO_STDOUT:
        case RADIO_FILE:
        case BROWSE_FILE:
            fTitlesView->MessageReceived( message );
            break;
        
        case B_REFS_RECEIVED:
        case SELECT_NONE:
        case SELECT_ALL:
        case BROWSE_FOLDER:
            fFilesView->MessageReceived( message );
            break;

        default:
            BView::MessageReceived( message );
            break;
    }
}

void DVDRipView::UpdateWrapper()
{
    if( fTitlesTab->IsSelected() )
    {
        fWrapper->fFilesInsteadOfTitles = false;
        fTitlesView->UpdateWrapper();
    }
    else
    {
        fWrapper->fFilesInsteadOfTitles = true;
        fFilesView->UpdateWrapper();
    }
}

void DVDRipView::SetEnabled( bool enabled )
{
    fTitlesTab->SetEnabled( enabled );
    fFilesTab->SetEnabled( enabled );
    fTitlesView->SetEnabled( enabled );
    fFilesView->SetEnabled( enabled );
    if( fTabView->LockLooper() )
    {
        fTabView->Draw( fTabView->Bounds() );
        fTabView->DrawTabs();
        fTabView->UnlockLooper();
    }
    if( fAboutButton->LockLooper() )
    {
        fAboutButton->SetEnabled( enabled );
        fAboutButton->UnlockLooper();
    }
    if( fRefreshButton->LockLooper() )
    {
        fRefreshButton->SetEnabled( enabled );
        fRefreshButton->UnlockLooper();
    }
    if( fRipButton->LockLooper() )
    {
        fRipButton->SetLabel( enabled ? "Rip !" : "Cancel" );
        if( enabled ) fRipButton->SetEnabled( true );
        fRipButton->UnlockLooper();
    }
}

void DVDRipView::Canceling()
{
    fRipButton->SetLabel( "Wait..." );
    fRipButton->SetEnabled( false );
}
