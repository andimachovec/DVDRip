#ifndef _DVDRIPVIEW_H
#define _DVDRIPVIEW_H

#include <InterfaceKit.h>

#include "DVDRipWrapper.h"
#include "DVDRipFilesView.h"
#include "DVDRipTitlesView.h"

#define RIP_BUTTON       'ribu'
#define REFRESH_VOLUMES  'revo'

class DVDRipView : public BView
{
    public:
        DVDRipView( BRect frame, DVDRipWrapper * );

        void               MessageReceived( BMessage * message );
        void               UpdateWrapper();
        void               SetEnabled( bool );
        void               Canceling();

    private:
        DVDRipWrapper *    fWrapper;
        BTabView *         fTabView;
        BTab *             fTitlesTab;
        BTab *             fFilesTab;
        DVDRipTitlesView * fTitlesView;
        DVDRipFilesView *  fFilesView;
        BButton *          fRipButton;
        BButton *          fRefreshButton;
        BButton *          fAboutButton;
        BStatusBar *       fStatusBar;
};

#endif
