#include "DVDRipTitlesView.h"

#include <stdlib.h>           // atoi()

DVDRipTitlesView::DVDRipTitlesView( BRect frame, DVDRipWrapper * wrapper )
    : BView( frame, "TitlesView", B_FOLLOW_ALL, B_WILL_DRAW )
{
    fWrapper = wrapper;

    BBox * box;

    // input box
    box = new BBox( BRect( 10,10,Bounds().Width()-15,85 ),
                    NULL, B_FOLLOW_NONE );
    box->SetLabel( "Input" ); 

    fVolumesPopUp = new BPopUpMenu( "Pick one :" );
    fVolumesField = new BMenuField( BRect( 10,15,box->Bounds().Width()-15,35 ),
                                    NULL, "Volume :", fVolumesPopUp, true );
    box->AddChild( fVolumesField );

    fTitlesPopUp = new BPopUpMenu( "Pick one :" );
    fTitlesField = new BMenuField( BRect( 10,45,box->Bounds().Width()-15,65 ),
                                   NULL, "Title :", fTitlesPopUp, true );
    box->AddChild( fTitlesField );

    AddChild( box );

    // output box
    box = new BBox( BRect( 10,95,Bounds().Width()-15,320 ), NULL, B_FOLLOW_ALL );
    box->SetLabel( "Output" );
    
    fStdOutRadio = new BRadioButton( BRect( 10,20,box->Bounds().Width()-15,40 ),
                                     NULL, "Write to standard output",
                                     new BMessage( RADIO_STDOUT ) );
    box->AddChild( fStdOutRadio );
    fFileRadio = new BRadioButton( BRect( 10,45,box->Bounds().Width()-15,65 ),
                                   NULL, "Write to file :",
                                   new BMessage( RADIO_FILE ) );
    fFileRadio->SetValue( true );
    box->AddChild( fFileRadio );
    fFileControl = new BTextControl( BRect( 30,70,box->Bounds().Width()-90,90 ),
                                     NULL, "", "/boot/home/Desktop/movie",
                                     new BMessage() );
    fFileControl->SetDivider( 0 );
    fFileControl->SetEnabled( false );
    box->AddChild( fFileControl );
    fBrowseButton = new BButton( BRect( box->Bounds().Width()-80,65,
                                 box->Bounds().Width()-15,90 ),
                          NULL, "Browse...", new BMessage( BROWSE_FILE ) );
    box->AddChild( fBrowseButton );
    AddChild( box );

    // hidden panel
    fPanel = new BFilePanel( B_SAVE_PANEL, NULL, NULL, 0, false );
}

void DVDRipTitlesView::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case UPDATE_TITLES:
        {
            // clean up
            BMenuItem * item;
            while( ( item = fTitlesPopUp->ItemAt( 0 ) ) )
                fTitlesPopUp->RemoveItem( (int32)0 );
            
            BList * titles = fWrapper->GetTitles( (char*)fVolumesPopUp->FindMarked()->Label() );
            if( titles )
            for( int i = 0; i < titles->CountItems(); i++ )
                fTitlesPopUp->AddItem( (BMenuItem*)titles->ItemAt( i ) );
            break;
        }

        case RADIO_STDOUT:
            fBrowseButton->SetEnabled( false );
            break;

        case RADIO_FILE:
            fBrowseButton->SetEnabled( true );
            break;

        case BROWSE_FILE:
            if( !fPanel->IsShowing() )
                fPanel->Show();
            break;
        
        case B_SAVE_REQUESTED:
        {
            entry_ref ref;
            BString string;
            if( message->FindRef( "directory", 0, &ref ) == B_OK &&
                message->FindString( "name", &string ) == B_OK )
            {
                BPath * path = new BPath( &ref );
                string.Prepend( "/" );
                string.Prepend( path->Path() );
                if( fFileControl->LockLooper() )
                {
                    fFileControl->SetText( string.String() );
                    fFileControl->UnlockLooper();
                }
            }
            break;
        }

        default:
            BView::MessageReceived( message );
            break;
    }
}

void DVDRipTitlesView::Refresh( bool force )
{
    // clean up
    BMenuItem * item;
    while( ( item = fVolumesPopUp->ItemAt( 0 ) ) )
        fVolumesPopUp->RemoveItem( (int32)0 );
    while( ( item = fTitlesPopUp->ItemAt( 0 ) ) )
        fTitlesPopUp->RemoveItem( (int32)0 );

    BList * volumes = fWrapper->GetVolumes( force );
    if( volumes )
        for( int i = 0; i < volumes->CountItems(); i++ )
            fVolumesPopUp->AddItem( (BMenuItem*)volumes->ItemAt( i ) );
    
    if( !fVolumesPopUp->FindMarked() )
        // no valid volume, don't ask about titles
        return;

    BList * titles = fWrapper->GetTitles( (char*)fVolumesPopUp->FindMarked()->Label() );
    if( titles )
        for( int i = 0; i < titles->CountItems(); i++ )
            fTitlesPopUp->AddItem( (BMenuItem*)titles->ItemAt( i ) );
}

void DVDRipTitlesView::UpdateWrapper()
{
    if( fVolumesPopUp->FindMarked() )
        fWrapper->fSelectedVolume = strdup( fVolumesPopUp->FindMarked()->Label() );
    else
        fWrapper->fSelectedVolume = NULL;
    
    if( fTitlesPopUp->FindMarked() )
        fWrapper->fSelectedTitle = atoi( fTitlesPopUp->FindMarked()->Label() );
    else
        fWrapper->fSelectedTitle = -1;

    fWrapper->fWriteToFile = fFileRadio->Value() ? true : false;
    fWrapper->fDestinationFile = strdup( fFileControl->Text() );
}

void DVDRipTitlesView::SetEnabled( bool enabled )
{
    if( fVolumesField->LockLooper() )
    {
        fVolumesField->SetEnabled( enabled );
        fVolumesField->UnlockLooper();
    }
    if( fTitlesField->LockLooper() )
    {
        fTitlesField->SetEnabled( enabled );
        fTitlesField->UnlockLooper();
    }
    if( fStdOutRadio->LockLooper() )
    {
        fStdOutRadio->SetEnabled( enabled );
        fStdOutRadio->UnlockLooper();
    }
    if( fFileRadio->LockLooper() )
    {
        fFileRadio->SetEnabled( enabled );
        fBrowseButton->SetEnabled( enabled && fFileRadio->Value() );
        fFileRadio->UnlockLooper();
    }
}
