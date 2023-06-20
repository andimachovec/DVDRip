#include "DVDRipFilesView.h"

DVDRipFilesView::DVDRipFilesView( BRect frame, DVDRipWrapper * wrapper )
    : BView( frame, "FilesView", B_FOLLOW_ALL, B_WILL_DRAW )
{
    fWrapper = wrapper;

    BBox * box;
    BStringView * string;
    BScrollView * scrollview;

    // input box
    box = new BBox( BRect( 10,10,Bounds().Width()-15,Bounds().Height()-115 ),
                    NULL, B_FOLLOW_NONE );
    box->SetLabel( "Input" ); 

    fVolumesPopUp = new BPopUpMenu( "Pick one :" );
    fVolumesField = new BMenuField( BRect( 10,15,box->Bounds().Width()-15,35 ),
                                    NULL, "Volume :", fVolumesPopUp, true );
    box->AddChild( fVolumesField );
    fListView = new BListView( BRect( 15,45,
                                      box->Bounds().Width()-15-B_V_SCROLL_BAR_WIDTH,
                                      box->Bounds().Height()-45 ),
                               NULL, B_MULTIPLE_SELECTION_LIST );
    scrollview = new BScrollView( NULL, fListView, B_FOLLOW_ALL,
                                  B_WILL_DRAW, false, true );
    box->AddChild( scrollview );
    fAllButton = new BButton( BRect( box->Bounds().Width()-90,
                                     box->Bounds().Height()-35,
                                     box->Bounds().Width()-10,
                                     box->Bounds().Height()-10 ),
                              NULL, "Select All", new BMessage( SELECT_ALL ) );
    box->AddChild( fAllButton );
    fNoneButton = new BButton( BRect( box->Bounds().Width()-180,
                                      box->Bounds().Height()-35,
                                      box->Bounds().Width()-100,
                                      box->Bounds().Height()-10 ),
                               NULL, "Select None", new BMessage( SELECT_NONE ) );
    box->AddChild( fNoneButton );

    AddChild( box );

    // output box
    box = new BBox( BRect( 10,Bounds().Height()-105,
                           Bounds().Width()-15,Bounds().Height()-40 ),
                     NULL, B_FOLLOW_ALL );
    box->SetLabel( "Output" );
    
    string = new BStringView( BRect( 10,15,box->Bounds().Width()-15,30 ),
                              NULL, "Destination folder :" );
    box->AddChild( string );
    fFolderControl = new BTextControl( BRect( 30,35,box->Bounds().Width()-90,55 ),
                                       NULL, "", "/boot/home/Desktop",
                                       new BMessage() );
    fFolderControl->SetDivider( 0 );
    fFolderControl->SetEnabled( false );
    box->AddChild( fFolderControl );
    fBrowseButton = new BButton( BRect( box->Bounds().Width()-80,30,
                                        box->Bounds().Width()-15,55 ),
                                 NULL, "Browse...", new BMessage( BROWSE_FOLDER ) );
    box->AddChild( fBrowseButton );

    AddChild( box );
    
    // hidden panel
    fPanel = new BFilePanel( B_OPEN_PANEL, NULL,
                             NULL, B_DIRECTORY_NODE );
}

void DVDRipFilesView::MessageReceived( BMessage * message )
{
    switch( message->what )
    {
        case SELECT_NONE:
            fListView->DeselectAll();
            break;
        
        case SELECT_ALL:
            fListView->Select( 0, fListView->CountItems() - 1 );
            break;
        
        case BROWSE_FOLDER:
            if( !fPanel->IsShowing() )
                fPanel->Show();
            break;
        
        case B_REFS_RECEIVED:
        {
            entry_ref ref;
            if( message->FindRef( "refs", 0, &ref ) == B_OK )
            {
                BPath * path = new BPath( &ref );
                if( fFolderControl->LockLooper() )
                {
                    fFolderControl->SetText( path->Path() );
                    fFolderControl->UnlockLooper();
                }
            }
            break;
        }
        
        default:
            BView::MessageReceived( message );
            break;
    }
}

void DVDRipFilesView::Refresh( bool force )
{
    // clean up
    BMenuItem * item;
    while( ( item = fVolumesPopUp->ItemAt( 0 ) ) )
        fVolumesPopUp->RemoveItem( (int32)0 );

    BList * volumes = fWrapper->GetVolumes( force );
    if( volumes )
        for( int i = 0; i < volumes->CountItems(); i++ )
            fVolumesPopUp->AddItem( (BMenuItem*)volumes->ItemAt( i ) );

    fListView->MakeEmpty();
    if( !fVolumesPopUp->FindMarked() )
            return;
    BList * files = fWrapper->GetFiles( (char*)fVolumesPopUp->FindMarked()->Label() );
    if( files )
        fListView->AddList( files );
}

void DVDRipFilesView::UpdateWrapper()
{
    if( fVolumesPopUp->FindMarked() )
        fWrapper->fSelectedVolume = strdup( fVolumesPopUp->FindMarked()->Label() );
    else
        fWrapper->fSelectedVolume = NULL;
    
    if( fWrapper->fSelectedList )
        delete fWrapper->fSelectedList;
    fWrapper->fSelectedList = new BList( MAX_FILES_NUMBER );
    int32 selected;
    int i = 0;
    while ( ( selected = fListView->CurrentSelection( i )) >= 0 )
    {
        BStringItem * item =
            new BStringItem( ((BStringItem*)fListView->ItemAt( selected ))->Text() );
        fWrapper->fSelectedList->AddItem( item );
        i++;
    }
    
    fWrapper->fDestinationFolder = strdup( fFolderControl->Text() );
}

void DVDRipFilesView::SetEnabled( bool enabled )
{
    if( fVolumesField->LockLooper() )
    {
        fVolumesField->SetEnabled( enabled );
        fVolumesField->UnlockLooper();
    }
    if( fNoneButton->LockLooper() )
    {
        fNoneButton->SetEnabled( enabled );
        fNoneButton->UnlockLooper();
    }
    if( fAllButton->LockLooper() )
    {
        fAllButton->SetEnabled( enabled );
        fAllButton->UnlockLooper();
    }
    if( fBrowseButton->LockLooper() )
    {
        fBrowseButton->SetEnabled( enabled );
        fBrowseButton->UnlockLooper();
    }
}
