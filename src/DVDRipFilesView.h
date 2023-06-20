#ifndef _DVDRIPFILESVIEW_H
#define _DVDRIPFILESVIEW_H

#include <InterfaceKit.h>
#include <StorageKit.h>    // BFilePanel

#include "DVDRipWrapper.h"

#define SELECT_NONE   'seno'
#define SELECT_ALL    'seal'
#define BROWSE_FOLDER 'brfo'

class DVDRipFilesView : public BView
{
    public:
        DVDRipFilesView( BRect frame, DVDRipWrapper * );
        
        void            MessageReceived( BMessage * message );
        void            Refresh( bool force );
        void            UpdateWrapper();
        void            SetEnabled( bool );
    
    private:
        DVDRipWrapper * fWrapper;
        BPopUpMenu *    fVolumesPopUp;
        BMenuField *    fVolumesField;
        BButton *       fNoneButton;
        BButton *       fAllButton;
        BButton *       fBrowseButton;
        BListView *     fListView;
        BTextControl *  fFolderControl;
        BFilePanel *    fPanel;
};

#endif
