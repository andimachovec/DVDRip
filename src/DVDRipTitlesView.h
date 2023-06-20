#ifndef _DVDRIPTITLESVIEW_H
#define _DVDRIPTITLESVIEW_H

#include <InterfaceKit.h>
#include <StorageKit.h>     // BFilePanel

#include "DVDRipWrapper.h"

#define RADIO_STDOUT 'rast'
#define RADIO_FILE   'rafi'
#define BROWSE_FILE  'brfi'

class DVDRipTitlesView : public BView
{
    public:
        DVDRipTitlesView( BRect frame, DVDRipWrapper * );

        void            MessageReceived( BMessage * message );
        void            Refresh( bool force );
        void            UpdateWrapper();
        void            SetEnabled( bool );
    
    private:
        DVDRipWrapper * fWrapper;
        BButton *       fBrowseButton;
        BCheckBox *     fMpeg4CheckBox;
        BTextControl *  fWidthControl;
        BTextControl *  fHeightControl;
        BTextControl *  fBitrateControl;
        BPopUpMenu *    fVolumesPopUp;
        BPopUpMenu *    fTitlesPopUp;
        BRadioButton *  fStdOutRadio;
        BRadioButton *  fFileRadio;
        BTextControl *  fFileControl;
        BMenuField *    fVolumesField;
        BMenuField *    fTitlesField;
        BFilePanel *    fPanel;
};

#endif
