#include "DVDRipWrapper.h"

#include <InterfaceKit.h>       // BMenuItem
#include <KernelKit.h>          // fs_info
#include <StorageKit.h>         // BVolumeRoster
#include <drivers/Drivers.h>    // device_geometry
#include <dvdread/dvd_reader.h> // libdvdread
#include <stdlib.h>             // atoi()
#include <sys/time.h>

#include "DVDRipApp.h"          // APP_NAME

int32 RipThread( void * info );
void RipTitles( DVDRipWrapper *, dvd_reader_t * );
void RipFiles( DVDRipWrapper *, dvd_reader_t * );

DVDRipWrapper::DVDRipWrapper()
{
    fVolumesList = NULL;
    fSelectedList = NULL;
    fStatus = -1;
    fLimit = 0;
}

BList * DVDRipWrapper::GetVolumes( bool force )
{
    if( force )
    {
        // clean up
        if( fVolumesList )
        {
            void * item;
            for( int i = 0; ( item = fVolumesList->ItemAt( i ) ); i++ )
                delete (DVDRipVolume*)item;
            delete fVolumesList;
        }
        fVolumesList = new BList( MAX_VOLUMES_NUMBER );

        // parse mounted volumes
        BVolumeRoster * roster = new BVolumeRoster();
        BVolume * volume = new BVolume();
        char volume_name[B_FILE_NAME_LENGTH];
        dev_t device;
        fs_info info;
        int i_device;
        device_geometry geometry;
        DVDRipVolume * dvdripvolume;

        while( roster->GetNextVolume( volume ) == B_NO_ERROR )
        {
            // only keep read-only ones
            if( volume->GetName( volume_name ) != B_OK ||
                !volume->IsReadOnly() )
                continue;

            // open() and ioctl() for more informations
            device = volume->Device();
            fs_stat_dev( device, &info );
            i_device = open( info.device_name, O_RDONLY );

            if( i_device < 0 )
                continue;

            if( ioctl( i_device, B_GET_GEOMETRY, &geometry,
                       sizeof( geometry ) ) < 0 )
                continue;

            if( geometry.device_type != B_CD )
                continue;

            // check for a non-empty VIDEO_TS folder (not case sensitive)
            BString path;
            path << "/" << volume_name;
            BDirectory directory( path.String() );
            BEntry entry;
            char video_folder[B_FILE_NAME_LENGTH];
            bool folder_found = false;
            while( directory.GetNextEntry( &entry ) == B_OK )
            {
                entry.GetName( video_folder );
                if( !strncasecmp( video_folder, "video_ts", 8 ) )
                {
                    folder_found = true;
                    break;
                }
            }

            if( !folder_found )
                continue;

            // this is a DVD, yoohoo !
            dvdripvolume = new DVDRipVolume( (char*)volume_name,
                                             (char*)info.device_name,
                                             video_folder );
            fVolumesList->AddItem( dvdripvolume );

            // update titles and files information
            GetTitles( dvdripvolume->volume_name );
            GetFiles( dvdripvolume->volume_name );
        }
    }

    // return a list of BMenuItems for pop-ups
    BList * list = new BList( MAX_VOLUMES_NUMBER );
    DVDRipVolume * dvdripvolume;
    for( int i = 0;
         ( dvdripvolume = (DVDRipVolume*)fVolumesList->ItemAt( i ) );
         i++ )
    {
        list->AddItem( new BMenuItem( dvdripvolume->volume_name,
                       new BMessage( UPDATE_TITLES ) ) );
    }
    // select the first item
    if( list->CountItems() )
        ((BMenuItem*)list->ItemAt( 0 ))->SetMarked( true );

    return list;
}

BList * DVDRipWrapper::GetTitles(const char* volume_name)
{
    // show a tiny alert window to inform the user we are scanning
    // the drives (this can take some time)
    BWindow * scanwin;
    scanwin = new BWindow( BRect( 0,0,200,35 ), APP_NAME,
                           B_FLOATING_WINDOW_LOOK,
                           B_MODAL_APP_WINDOW_FEEL,
                           B_NOT_CLOSABLE | B_NOT_MOVABLE |
                           B_NOT_ZOOMABLE | B_NOT_RESIZABLE );
    BView * scanview;
    scanview = new BView( scanwin->Bounds(), NULL,
                          B_FOLLOW_NONE, B_WILL_DRAW );
    scanview->SetViewColor( ui_color( B_PANEL_BACKGROUND_COLOR ) );
    BStringView * scanstring;
    scanstring = new BStringView( BRect( 0,10,200,25 ), NULL,
                                  "Scanning drives, please wait..." );
    scanstring->SetAlignment( B_ALIGN_CENTER );
    scanview->AddChild( scanstring );
    scanwin->AddChild( scanview );

    // center it
    BScreen screen;
    scanwin->MoveTo( ( screen.Frame().Width() -
                         scanwin->Bounds().Width() ) / 2,
                     ( screen.Frame().Height() -
                         scanwin->Bounds().Height() ) / 2 );
    scanwin->Show();

    // find the right DVDRipVolume
    BList * list = NULL;
    DVDRipVolume * dvdripvolume;
    for( int i = 0;
         ( dvdripvolume = (DVDRipVolume*)fVolumesList->ItemAt( i ) );
         i++ )
    {
        if( !strcmp( dvdripvolume->volume_name, volume_name ) )
            // we found it
        {
            // scan it only if we haven't done it already
            if( !dvdripvolume->fTitlesList )
            {
                dvdripvolume->fTitlesList = new BList( MAX_TITLES_NUMBER );

                // parse the titles
                dvd_reader_t * dvd_reader;
                dvd_file_t * dvd_file;
                DVDRipTitle * dvdriptitle;
                dvd_reader = DVDOpen( dvdripvolume->device_name );
                int index = 1;
                while( ( dvd_file = DVDOpenFile( dvd_reader, index, DVD_READ_TITLE_VOBS ) ) )
                {
                    dvdriptitle = new DVDRipTitle( index,
                                                   DVDFileSize( dvd_file ) );
                    dvdripvolume->fTitlesList->AddItem( dvdriptitle );

                    DVDCloseFile( dvd_file );
                    index++;
                }
                DVDClose( dvd_reader );
            }

            // return a list of BMenuItems for pop-ups
            list = new BList( MAX_TITLES_NUMBER );
            DVDRipTitle * dvdriptitle;
            for( int i = 0;
                 ( dvdriptitle = (DVDRipTitle*)dvdripvolume->fTitlesList->ItemAt( i ) );
                 i++ )
            {
                char string[1024];
                memset( string, 0, 1024 );
                if( (long long)dvdriptitle->length * DVD_VIDEO_LB_LEN > 1024 * 1024 )
                    sprintf( string, "%d (%lld MB)", dvdriptitle->index,
                             (long long)dvdriptitle->length * DVD_VIDEO_LB_LEN /
                                 ( 1024 * 1024 ) );
                else if( (long long)dvdriptitle->length * DVD_VIDEO_LB_LEN > 1024 )
                    sprintf( string, "%d (%lld KB)", dvdriptitle->index,
                             (long long)dvdriptitle->length * DVD_VIDEO_LB_LEN / 1024 );
                else
                    sprintf( string, "%d (%lld bytes)", dvdriptitle->index,
                             (long long)dvdriptitle->length * DVD_VIDEO_LB_LEN );
                list->AddItem( new BMenuItem( strdup( string ), new BMessage() ) );
            }
            // select the first item
            if( list->CountItems() )
            ((BMenuItem*)list->ItemAt( 0 ))->SetMarked( true );

            break;
        }
    }

    // remove the scan alert
    scanwin->Lock();
    scanwin->Quit();

    return list;
}

BList * DVDRipWrapper::GetFiles(const char* volume_name)
{
    // find the right DVDRipVolume
    BList * list = NULL;
    DVDRipVolume * dvdripvolume;
    for( int i = 0;
         ( dvdripvolume = (DVDRipVolume*)fVolumesList->ItemAt( i ) );
         i++ )
    {
        if( !strcmp( dvdripvolume->volume_name, volume_name ) )
            // we found it
        {
            // scan it only if we haven't done it already
            if( !dvdripvolume->fFilesList )
            {
                dvdripvolume->fFilesList = new BList( MAX_FILES_NUMBER );

                DVDRipFile * dvdripfile;
                BString path;
                path << "/" << dvdripvolume->volume_name
                    << "/" << dvdripvolume->video_folder;
                BDirectory directory( path.String() );
                BEntry entry;

                while( directory.GetNextEntry( &entry ) == B_OK )
                {
                    dvdripfile = new DVDRipFile();
                    entry.GetName( dvdripfile->name );
                    entry.GetSize( &dvdripfile->size );
                    dvdripvolume->fFilesList->AddItem( dvdripfile );
                }
            }

            // return a list of BStringItems for the BListView
            list = new BList( MAX_FILES_NUMBER );
            DVDRipFile * dvdripfile;
            for( int i = 0;
                 ( dvdripfile = (DVDRipFile*)dvdripvolume->fFilesList->ItemAt( i ) );
                 i++ )
            {
                char string[1024];
                memset( string, 0, 1024 );
                if( dvdripfile->size > 1024 * 1024 )
                    sprintf( string, "%s (%ld MB)", dvdripfile->name,
                             dvdripfile->size / ( 1024 * 1024 ) );
                else if( dvdripfile->size > 1024 )
                    sprintf( string, "%s (%ld KB)", dvdripfile->name,
                             dvdripfile->size / 1024 );
                else
                    sprintf( string, "%s (%ld bytes)", dvdripfile->name,
                             dvdripfile->size );
                list->AddItem( new BStringItem( strdup( string ) ) );
            }

            break;
        }
    }

    return list;
}

bool DVDRipWrapper::Rip()
{
    // sanity tests : is there something to rip ?
    if( !fSelectedVolume )
    {
        BAlert * alert;
        alert = new BAlert( APP_NAME, "No DVD found !", "Duh ?" );
        alert->Go();
        return false;
    }
    if( fFilesInsteadOfTitles &&
        ( !fSelectedList || !fSelectedList->CountItems() ) )
    {
        BAlert * alert;
        alert = new BAlert( APP_NAME, "Select at least one file !", "Duh ?" );
        alert->Go();
        return false;
    }

    // now, do the job
    thread_id id = spawn_thread( RipThread, "rip thread",
                                 B_LOW_PRIORITY, this );
    resume_thread( id );

    return true;
}

int32 RipThread( void * info )
{
    DVDRipWrapper * w = (DVDRipWrapper*)info;

    w->fStatus = 0;
    w->fCanceled = false;

    // show "Opening device..."
    BMessage * message = NULL;
    message = new BMessage( UPDATE_INTERFACE );
    be_app->PostMessage( message );
    if( message ) delete message;

    // find and open the device
    dvd_reader_t * dvd_reader = NULL;
    DVDRipVolume * dvdripvolume = NULL;
    for( int i = 0;
         ( dvdripvolume = (DVDRipVolume*)w->fVolumesList->ItemAt( i ) );
         i++ )
    {
        if( !strcmp( dvdripvolume->volume_name, w->fSelectedVolume ) )
        {
            dvd_reader = DVDOpen( dvdripvolume->device_name );
            break;
        }
    }
    if( !dvd_reader )
    {
        BAlert * alert;
        alert = new BAlert( APP_NAME, "Cannot open device", "Argh !" );
        alert->Go();
        w->fStatus = -1;
        return -1;
    }

    // do the job
    if( w->fFilesInsteadOfTitles )
        RipFiles( w, dvd_reader );
    else
        RipTitles( w, dvd_reader );

    if( dvd_reader ) { DVDClose( dvd_reader ); dvd_reader = NULL; }

    if( w->fCanceled )
        message = new BMessage( RIP_CANCELED );
    else
        message = new BMessage( RIP_DONE );

    be_app->PostMessage( message );
    if( message ) { delete message; message = NULL; }

    w->fStatus = -1;
    w->fCanceled = false;

    return 0;
}

void RipTitles( DVDRipWrapper * w, dvd_reader_t * dvd_reader )
{
    // open the DVD title
    dvd_file_t * dvd_file = NULL;
    dvd_file = DVDOpenFile( dvd_reader, w->fSelectedTitle, DVD_READ_TITLE_VOBS );
    if( !dvd_file )
    {
        BAlert * alert;
        alert = new BAlert( APP_NAME, "Cannot open DVD title", "Argh !" );
        alert->Go();
        w->fStatus = -1;
        return;
    }

    int length = DVDFileSize( dvd_file );
    w->fTotal = (long long)length * DVD_VIDEO_LB_LEN;
    if( w->fLimit > 0 && w->fTotal > w->fLimit * 1024 * 1024 )
        length = w->fLimit * 1024 * 1024 / DVD_VIDEO_LB_LEN;

    BFile * file = NULL;
    if( w->fWriteToFile )
    {
        // open the destination file
        file = new BFile( w->fDestinationFile, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );
        if( file->InitCheck() != B_OK )
        {
            BAlert * alert;
            alert = new BAlert( APP_NAME, "Cannot open file", "Argh !" );
            alert->Go();
            if( dvd_file ) { DVDCloseFile( dvd_file ); dvd_file = NULL; }
            w->fStatus = -1;
            return;
        }
    }

    // needed for speed evaluation
    struct timeval tv;
    gettimeofday( &tv, NULL );
    float oldtime = (float)(tv.tv_sec * 1000000 + tv.tv_usec );
    float time;

    // rip loop
    unsigned char buffer[BLOCKS_READ_ONCE*DVD_VIDEO_LB_LEN];
    int oldoffset = 0;
    int offset = 0;
    BMessage * message = NULL;
    while( !w->fCanceled && offset < length )
    {
        int blocks = DVDFileSize( dvd_file ) - offset < BLOCKS_READ_ONCE ?
                         DVDFileSize( dvd_file ) - offset : BLOCKS_READ_ONCE;

        // read
        if( DVDReadBlocks( dvd_file, offset, blocks, buffer ) < 0 )
        {
            BAlert * alert;
            alert = new BAlert( APP_NAME, "Cannot read block", "Argh !" );
            alert->Go();
            if( dvd_file ) { DVDCloseFile( dvd_file ); dvd_file = NULL; }
            if( file ) { delete file; file = NULL; }
            w->fStatus = -1;
            return;
        }

        // write
        if( w->fWriteToFile )
        {
            off_t size;
            file->GetSize( &size );
            if( file->SetSize( size + blocks*DVD_VIDEO_LB_LEN ) == B_DEVICE_FULL )
            {
                BAlert * alert;
                alert = new BAlert( APP_NAME, "Device full !", "Argh !" );
                alert->Go();
                DVDCloseFile( dvd_file );
                delete file;
                w->fStatus = -1;
                return;
            }
            if( file->WriteAt( size, buffer, blocks*DVD_VIDEO_LB_LEN ) < 0 )
            {
                BAlert * alert;
                alert = new BAlert( APP_NAME, "Cannot write to file", "Argh !" );
                alert->Go();
                if( dvd_file ) { DVDCloseFile( dvd_file ); dvd_file = NULL; }
                if( file ) { delete file; file = NULL; }
                w->fStatus = -1;
                return;
            }
        }
        else
            fwrite( buffer, DVD_VIDEO_LB_LEN, blocks, stdout );

        offset += blocks;

        // update the slider and speed
        w->fStatus = (float)offset / (float)length;
        gettimeofday( &tv, NULL );
        time = (float)(tv.tv_sec * 1000000 + tv.tv_usec );
        if( time - oldtime > 500000 )
        {
            // refresh the speed every 0.5 sec
            w->fSpeed =  (float)( offset - oldoffset ) * DVD_VIDEO_LB_LEN /
                             ( time - oldtime );
            oldoffset = offset;
            oldtime = time;
        }
        message = new BMessage( UPDATE_INTERFACE );
        be_app->PostMessage( message );
        if( message ) { delete message; message = NULL; }
    }

    // close
    if( dvd_file ) { DVDCloseFile( dvd_file ); dvd_file = NULL; }
    if( file ) { delete file; file = NULL; }
}

void RipFiles( DVDRipWrapper * w, dvd_reader_t * dvd_reader )
{
    dvd_read_domain_t domain;
    int title;
    dvd_file_t * dvd_file = NULL;
    BFile * file;

    // find current volume
    DVDRipVolume * dvdripvolume;
    for( int i = 0;
         ( dvdripvolume = (DVDRipVolume*)w->fVolumesList->ItemAt( i ) );
         i++ )
    {
        if( !strcmp( dvdripvolume->volume_name, w->fSelectedVolume ) )
            break;
    }

    // calculate total size
    int index = 0;
    BStringItem * item;
    w->fTotal = 0;
    while( ( item = (BStringItem*)w->fSelectedList->ItemAt( index ) ) )
    {
        char * name = strdup( item->Text() );
        /* kludge */
        char * tmp;
        tmp = name;
        while( *tmp && *tmp != ' ' )
            tmp++;
        *tmp = '\0';

        DVDRipFile * dvdripfile;
        for( int i = 0;
             ( dvdripfile = (DVDRipFile*)dvdripvolume->fFilesList->ItemAt( i ) );
             i++ )
        {
            if( !strcmp( dvdripfile->name, name ) )
                w->fTotal += dvdripfile->size;
        }
        index++;
    }

    // now rip the files
    index = 0;
    while( ( item = (BStringItem*)w->fSelectedList->ItemAt( index ) ) )
    {
        char name[B_FILE_NAME_LENGTH] = "";
        sprintf( name, item->Text() );
        /* kludge */
        char * tmp;
        tmp = name;
        while( *tmp && *tmp != ' ' )
            tmp++;
        *tmp = '\0';

        // get file type
        if( !strcasecmp( name + 8, ".BUP" ) )
            domain = DVD_READ_INFO_BACKUP_FILE;
        else if( !strcasecmp( name + 8, ".IFO" ) )
            domain = DVD_READ_INFO_FILE;
        else if( name[7] != '0' && name[7] != 'S' )
            domain = DVD_READ_TITLE_VOBS;
        else
            domain = DVD_READ_MENU_VOBS;

        // get title
        if( !strncasecmp( name, "VIDEO_TS", 8 ) )
            title = 0;
        else
        {
            char titlestring[3] = "";
            memset( titlestring, 0, 3 );
            memcpy( titlestring, name + 4, 2 );
            title = atoi( titlestring );
        }

        // get offset and size (UGLY !)
        int offset = 0;
        int length = 0;

        DVDRipFile * dvdripfile;
        for( int i = 0;
             ( dvdripfile = (DVDRipFile*)dvdripvolume->fFilesList->ItemAt( i ) );
             i++ )
        {
            if( !strcmp( dvdripfile->name, name ) )
                length = dvdripfile->size / DVD_VIDEO_LB_LEN;

            if( domain == DVD_READ_TITLE_VOBS && title > 0 )
            {
                char * tmpname = dvdripfile->name;
                dvd_read_domain_t tmpdomain;
                int tmptitle;

                // get file type
                if( !strcasecmp( tmpname + 8, ".BUP" ) )
                    tmpdomain = DVD_READ_INFO_BACKUP_FILE;
                else if( !strcasecmp( tmpname + 8, ".IFO" ) )
                    tmpdomain = DVD_READ_INFO_FILE;
                else if( tmpname[7] != '0' && tmpname[7] != 'S' )
                    tmpdomain = DVD_READ_TITLE_VOBS;
                else
                    tmpdomain = DVD_READ_MENU_VOBS;

                // get title
                if( !strncasecmp( tmpname, "VIDEO_TS", 8 ) )
                    tmptitle = 0;
                else
                {
                    char titlestring[3] = "";
                    memset( titlestring, 0, 3 );
                    memcpy( titlestring, tmpname + 4, 2 );
                    tmptitle = atoi( titlestring );
                }

                if( tmpdomain != domain || tmptitle != title )
                    continue;

                if( tmpname[7] < name[7] )
                    offset += dvdripfile->size / DVD_VIDEO_LB_LEN;
            }
        }

        // open the destination file
        BString path;
        path << w->fDestinationFolder << "/" << name;
        file = new BFile( path.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE );
        if( file->InitCheck() != B_OK )
        {
            BAlert * alert;
            alert = new BAlert( APP_NAME, "Cannot open file", "Argh !" );
            alert->Go();
            DVDCloseFile( dvd_file );
            w->fStatus = -1;
            return;
        }

        // in case we have 0-byte files (e.g. the Matrix DVD)
        if( !length )
        {
            delete file;
            index++;
            continue;
        }

        dvd_file = DVDOpenFile( dvd_reader, title, domain );
        if( !dvd_file )
        {
            BAlert * alert;
            alert = new BAlert( APP_NAME, "Cannot open DVD file", "Argh !" );
            alert->Go();
            w->fStatus = -1;
            return;
        }

        // needed for speed evaluation
        struct timeval tv;
        gettimeofday( &tv, NULL );
        float oldtime = (float)(tv.tv_sec * 1000000 + tv.tv_usec );
        float time;

        // rip loop
        unsigned char buffer[BLOCKS_READ_ONCE*DVD_VIDEO_LB_LEN];
        int oldoffset = offset;
        int end = offset + length;
        BMessage * message;
        while( !w->fCanceled && offset < end )
        {
            int blocks = end - offset < BLOCKS_READ_ONCE ?
                             end - offset : BLOCKS_READ_ONCE;

            // read
            int result;
            if( domain == DVD_READ_INFO_FILE || domain == DVD_READ_INFO_BACKUP_FILE )
                result = DVDReadBytes( dvd_file, buffer, blocks*DVD_VIDEO_LB_LEN );
            else
                result = DVDReadBlocks( dvd_file, offset, blocks, buffer );

            if( result < 0 )
            {
                BAlert * alert;
                alert = new BAlert( APP_NAME, "Cannot read block", "Argh !" );
                alert->Go();
                DVDCloseFile( dvd_file );
                delete file;
                w->fStatus = -1;
                return;
            }

            // write
            off_t size;
            file->GetSize( &size );
            if( file->SetSize( size + blocks*DVD_VIDEO_LB_LEN ) == B_DEVICE_FULL )
            {
                BAlert * alert;
                alert = new BAlert( APP_NAME, "Device full !", "Argh !" );
                alert->Go();
                DVDCloseFile( dvd_file );
                delete file;
                w->fStatus = -1;
                return;
            }
            if( file->WriteAt( size, buffer, blocks*DVD_VIDEO_LB_LEN ) < 0 )
            {
                BAlert * alert;
                alert = new BAlert( APP_NAME, "Cannot write to file", "Argh !" );
                alert->Go();
                DVDCloseFile( dvd_file );
                delete file;
                w->fStatus = -1;
                return;
            }

            offset += blocks;

            // update the slider and speed
            w->fStatus += DVD_VIDEO_LB_LEN * (float)blocks / (float)w->fTotal;
            gettimeofday( &tv, NULL );
            time = (float)(tv.tv_sec * 1000000 + tv.tv_usec );
            if( time - oldtime > 500000 )
            {
                // refresh the speed every 0.5 sec
                w->fSpeed =  (float)( offset - oldoffset ) * DVD_VIDEO_LB_LEN /
                                 ( time - oldtime );
                oldoffset = offset;
                oldtime = time;
            }
            message = new BMessage( UPDATE_INTERFACE );
            be_app->PostMessage( message );
            delete message;
        }

        // close
        if( dvd_file )
            DVDCloseFile( dvd_file );
        if( file )
            delete file;

        index++;
    }
}
