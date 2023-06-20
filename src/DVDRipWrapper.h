#ifndef _DVDRIPWRAPPER_H
#define _DVDRIPWRAPPER_H

#include <SupportKit.h>           // BList

#define MAX_VOLUMES_NUMBER 10     // hopefully nobody has more than
                                  // 10 DVD drives ;)
#define MAX_TITLES_NUMBER  200
#define MAX_FILES_NUMBER   200
#define BLOCKS_READ_ONCE   64     // 128 ko by loop
#define UPDATE_INTERFACE   'upin'
#define UPDATE_TITLES      'upti'
#define RIP_CANCELED       'rica'
#define RIP_DONE           'rido'

class DVDRipTitle
{
    public:
        DVDRipTitle( int index, ssize_t length )
            { this->index = index;
              this->length = length; }

        int     index;
        ssize_t length;
};

class DVDRipFile
{
    public:
        DVDRipFile() { memset( name, 0, B_FILE_NAME_LENGTH ); }

        char name[B_FILE_NAME_LENGTH];
        off_t size;
};

class DVDRipVolume
{
    public:
        DVDRipVolume( char * volume_name, char * device_name,
                      char * video_folder )
            { this->volume_name = strdup( volume_name );
              this->device_name = strdup( device_name );
              this->video_folder = strdup( video_folder );
              fTitlesList = NULL;
              fFilesList = NULL; }

        const char*  volume_name;
        const char*  device_name;
        const char*  video_folder;
        BList * fTitlesList;
        BList * fFilesList;
};

class DVDRipWrapper
{
    public:
        DVDRipWrapper();

        bool               Rip();
        BList *            GetVolumes( bool force );
        BList *            GetTitles(const char* volume_name);
        BList *            GetFiles(const char* volume_name);

        BList *            fVolumesList;

        // infos from the interface
        bool               fFilesInsteadOfTitles;
        const char*        fSelectedVolume;
        int                fSelectedTitle;
        bool               fWriteToFile;    // true if file, false if stdout
        const char*   	   fDestinationFile;
        bool               fEncodeToMpeg4;
        BList *            fSelectedList;
        const char*        fDestinationFolder;
        volatile float     fStatus;
        volatile float     fSpeed;
        volatile long long fTotal;          // bytes
        volatile bool      fCanceled;
        int                fLimit;          // MB !
};

#endif
