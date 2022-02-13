//
// Created by paul on 2/10/22.
//
#define _XOPEN_SOURCE 500

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ftw.h>
#include <magic.h>
#include <string.h>

#include "fixup-x.h"

typedef unsigned long tStringHash;

typedef struct sFileType {
    struct sFileType *  next;
    tStringHash         hash;
    const char *        description;
    unsigned long       count;
    bool                executable;
    bool                hasDebugInfo;
    bool                notStripped;
    enum {
        eNotApplicable = 0, i386, x86_64, arm, arm64
    } instructionSetArchitecture;
    enum {
        unknown = 0, statically, dynamically
    } linked;
} tFileType;

static struct {
    struct {
        magic_t         cookie;
    } magic;
    struct {
        unsigned long   files;
        unsigned long   directories;
        unsigned long   directoriesNotReadable;
        unsigned long   symlinks;
        unsigned long   unknown;
    } count;
    struct {
        tFileType *     fileTypes;
    } list;
} global;


#define kPosixShellScript       0x8c31cd705219c078
#define kBashScript             0xf1ed5dcbda7290eb
#define kASCIItextExecutable    0x37d5693b400b5e92
#define kMakefile               0x9b30d0a1938941ed
#define kM4MacroProcessor       0x047491e3fe91ab92

#define kASCIItext              0x007793288a3d4940
#define kUTF8text               0x14363e1b1decc94a

#define kELF32LSBexecutable     0x0d378a936c98dd77
#define kELF32LSBrelocatable    0x3c18c8e848a2b921
#define kELF32LSBsharedObject   0x5cf99f6b75c5ab31
#define kELF64LSBexecutable     0x2787caf6cff77570
#define kELF64LSBrelocatable    0xa7939999f9863ff4
#define kELF64LSBsharedObject   0xa706f2d2111e752c

#define kIntel80386             0x16ae39e49542b30b
#define kx86_64                 0x000000042728a7ad
#define kARM                    0x000000000001e38c
#define kARMaarch64             0x14144f04b8a53c29

#define kGIFimageData           0x2865dca351520983
#define kJPEGimageData          0x8778c2d6ef005bbf
#define kPNGimageData           0x5d6522604bcab33e

#define kAssemblerSource        0xa1f98a0f20fb2f03
#define kCsource                0x000010c366926420

#define kCRLFlineEndings        0xfbcb3a91b4eed651
#define kLongLines              0x88c4f3f4395ec30f
#define kWithDebugInfo          0xae9062447a72f508
#define kNotStripped            0xacb9afc024c117a4
#define kDynamicallyLinked      0x277b2b4786b618f2

tStringHash hashString( const char * string )
{
    tStringHash hash;
    for ( hash = 0; *string != '\0'; ++string )
    {
        hash = (hash * 43) + (unsigned char)*string;
    }
    return hash;
}

#if 0
    S_IRUSR	/* Read by owner     */
    S_IWUSR	/* Write by owner    */
    S_IXUSR	/* Execute by owner  */

    S_IRGRP	/* Read by group     */
    S_IWGRP	/* Write by group    */
    S_IXGRP	/* Execute by group  */

    S_IROTH	/* Read by others    */
    S_IWOTH /* Write by others   */
    S_IXOTH	/* Execute by others */
#endif

int incFileType(  const char * fpath, const char * description )
{
    bool found = false;
    tStringHash hash;

    char * desc = strdup( description );
    tFileType * newType = calloc( 1, sizeof( tFileType ));

    if ( desc != NULL && newType != NULL)
    {
        char * s = desc;
        char * d = desc;
        char * rewind;
        while ( *s != '\0' )
        {
            hash = 0;
            rewind = d;
            while ( *s != ',' && *s != '\0' )
            {
                hash = (hash * 43) + (unsigned char)*s;
                *d++ = *s++;
            }
            if ( *s == ',' )
            {
                *d++ = *s++;
            }
            while ( *s == ' ' )
            {
                *d++ = *s++;
            }

#if 0
            printf( "segment: \"" );
            fwrite( rewind, s - rewind, 1, stdout );
            printf( "\", hash: 0x%016lx\n", hash );
#endif

            switch ( hash )
            {
            case kIntel80386:
//                printf( "i386\n" );
                newType->executable = true;
                newType->instructionSetArchitecture = i386;
                printf("%s\n    %s\n", desc, fpath );
                break;

            case kx86_64:
//                printf( "x86-64\n" );
                newType->executable = true;
                newType->instructionSetArchitecture = x86_64;
//                printf("%s\n    %s\n", desc, fpath );
                break;

#if 0
            case kASCIItext: printf( "ASCII text\n" ); break;
            case kCsource:   printf( "C source\n" );   break;
            case kLongLines:
                printf( "long lines\n" );
                d = rewind;
                break;

            case kWithDebugInfo:
                newType->executable   = true;
                newType->hasDebugInfo = true;
                d = rewind;
                printf( "with debug info\n" );
                break;

            case kNotStripped:
                newType->executable  = true;
                newType->notStripped = true;
                d = rewind;
                printf( "not stripped\n" );
                break;

            case kDynamicallyLinked:
                newType->executable = true;
                newType->linked = dynamically;
                d = rewind;
                printf( "dynamically linked\n" );
                break;

#endif
            default:
                if ( strncmp( "BuildID", rewind, 7 ) == 0 )
                {
                    d = rewind;
//                    printf( "Build ID\n" );
                }
                break;
            }
        }
        while ( d[-1] == ' ' || d[-1] == ',' ) { --d; }
        *d = '\0';

        hash = hashString( desc );
//        printf( "type: %s, hash: 0x%016lx\n", desc, hash );

        tFileType * type;
        for ( type = global.list.fileTypes; type != NULL && found != true; type = type->next )
        {
            if ( type->hash == hash )
            {
                ++type->count;
//                printf( "exists: %s\n", type->description );
                found = true;
                free( newType );
            }
        }
        if ( !found )
        {
            newType->hash = hash;
            newType->description = strdup( desc );
            newType->count = 1;
            newType->next = global.list.fileTypes;
            global.list.fileTypes = newType;
        }
        free( desc );
    }
    return 0;
}

int countFile( const char *fpath, const struct stat * sb )
{
    int result = 0;
    const char * description = NULL;

    // printf("\npath: %s\n", fpath );
    description = magic_file( global.magic.cookie, fpath );
    if ( description != NULL )
    {
        result = incFileType( fpath, description );
    } else {
        fprintf( stderr, "### error: %s\n", magic_error( global.magic.cookie ) );
    }

    return result;
}

int fn( const char *fpath, const struct stat * sb, int typeflag, struct FTW *ftwbuf )
{
    int result = 0;
    const char * description = NULL;


    switch ( typeflag )
    {
    case FTW_F: /* regular file */
        result = countFile( fpath, sb );
        ++global.count.files;
        break;

    case FTW_D: /* directory */
        ++global.count.directories;
        break;

    case FTW_DNR: /* directory, not readable */
        ++global.count.directoriesNotReadable;
        break;

    case FTW_SL: /* symbolic link */
    case FTW_NS:
        ++global.count.symlinks;
        break;

    default:
        ++global.count.unknown;
        break;
    }

    return result;
}

void displayResults( void )
{
    printf( "           file count: %lu\n", global.count.files);
    tFileType * type;
#if 0
    for ( type = global.list.fileTypes; type != NULL; type = type->next )
    {
        printf("%5lu\t%s\n", type->count, type->description );
    }
#endif
    printf( "        symlink count: %lu\n", global.count.symlinks);
    printf( "      directory count: %lu\n", global.count.directories);
    printf( " unreadable dir count: %lu\n", global.count.directoriesNotReadable);
    printf( "        unknown count: %lu\n", global.count.unknown);
}

int main( int argc, char *argv[] )
{
    global.magic.cookie = magic_open( 0 );
    if ( global.magic.cookie != NULL )
    {
        magic_load( global.magic.cookie, NULL );

        nftw( ".", fn, 10, FTW_PHYS );

        displayResults();

        magic_close( global.magic.cookie );
        global.magic.cookie = NULL;
    }
}