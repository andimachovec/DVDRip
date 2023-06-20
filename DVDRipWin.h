#ifndef _DVDRIPWIN_H
#define _DVDRIPWIN_H

#include <InterfaceKit.h>

#include "DVDRipWrapper.h"
#include "DVDRipView.h"

#define WINDOW_RECT BRect( 0,0,350,450 )

class DVDRipWin : public BWindow
{
    public:
        DVDRipWin( const char * name, DVDRipWrapper * );

        void            MessageReceived( BMessage * message );
        bool            QuitRequested();

    private:
        DVDRipWrapper * fWrapper;
        DVDRipView *    fView;
};

#endif
