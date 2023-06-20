#ifndef _DVDRIPAPP_H
#define _DVDRIPAPP_H

#include <AppKit.h>

#include "DVDRipWrapper.h"
#include "DVDRipWin.h"

#define APP_NAME      "DVDRip 1.0"

class DVDRipApp : public BApplication
{
    public:
        DVDRipApp( int, char ** );
        
        void            MessageReceived( BMessage * message );
        void            RefsReceived( BMessage * message );

    private:
        DVDRipWrapper * fWrapper;
        DVDRipWin *     fWindow;
};

#endif
