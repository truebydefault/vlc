/*****************************************************************************
 * sgimb.c: a meta demux to parse sgimb referrer files
 *****************************************************************************
 * Copyright (C) 2004 VideoLAN
 * $Id: $
 *
 * Authors: Derk-Jan Hartman <hartman at videolan dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111, USA.
 *****************************************************************************/

/*****************************************************************************
 * This is a metademux for the Kasenna MediaBase metafile format.
 * Kasenna MediaBase first returns this file when you are trying to access
 * their MPEG streams (MIME: application/x-sgimb). Very few applications
 * understand this format and the format is not really documented on the net.
 * Following a typical MediaBase file. Notice the sgi prefix of all the elements.
 * This stems from the fact that the MediaBase server were first introduced by SGI?????.
 *
 * sgiNameServerHost=host.name.tld
 * Stream="xdma://host.name.tld/demo/a_very_cool.mpg"
 * sgiMovieName=/demo/a_very_cool.mpg
 * sgiAuxState=1
 * sgiFormatName=PARTNER_41_MPEG-4
 * sgiBitrate=1630208
 * sgiDuration=378345000
 * sgiQTFileBegin
 * rtsptext
 * rtsp://host.name.tld/demo/a_very_cool.mpg
 * sgiQTFileEnd
 * sgiApplicationName=MediaBaseURL
 * sgiElapsedTime=0
 * sgiServerVersion=6.1.2
 * sgiRtspPort=554
 * AutoStart=True
 * sgiUserAccount=pid=1724&time=1078527309&displayText=You%20are%20logged%20as%20guest&
 * sgiUserPassword=
 *
 *****************************************************************************/


/*****************************************************************************
 * Preamble
 *****************************************************************************/
#include <stdlib.h>                                      /* malloc(), free() */

#include <vlc/vlc.h>
#include <vlc/input.h>
#include <vlc_playlist.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Activate  ( vlc_object_t * );
static void Deactivate( vlc_object_t * );

vlc_module_begin();
    set_description( _("Kasenna MediaBase metademux") );
    set_capability( "demux2", 170 );
    set_callbacks( Activate, Deactivate );
    add_shortcut( "sgimb" );
vlc_module_end();

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
#define MAX_LINE 1024

struct demux_sys_t
{
    char        *psz_uri;       /* Stream= or sgiQTFileBegin rtsp link */
    char        *psz_server;    /* sgiNameServerHost= */
    char        *psz_location;  /* sgiMovieName= */
    char        *psz_name;      /* sgiShowingName= */
    char        *psz_user;      /* sgiUserAccount= */
    char        *psz_password;  /* sgiUserPassword= */
    mtime_t     i_duration;     /* sgiDuration= */
    int         i_port;         /* sgiRtspPort= */
    int         i_sid;          /* sgiSid= */
};

static int Demux ( demux_t *p_demux );
static int Control( demux_t *p_demux, int i_query, va_list args );

/*****************************************************************************
 * Activate: initializes m3u demux structures
 *****************************************************************************/
static int Activate( vlc_object_t * p_this )
{
    demux_t *p_demux = (demux_t *)p_this;
    demux_sys_t *p_sys;

    p_demux->pf_demux = Demux;
    p_demux->pf_control = Control;
    p_demux->p_sys = p_sys = malloc( sizeof( demux_sys_t ) );
    p_sys->psz_uri = NULL;
    p_sys->psz_server = NULL;
    p_sys->psz_location = NULL;
    p_sys->psz_name = NULL;
    p_sys->psz_user = NULL;
    p_sys->psz_password = NULL;
    p_sys->i_duration = (mtime_t)0;
    p_sys->i_port = 0;


    /* Lets check the content to see if this is a sgi mediabase file */
    byte_t *p_peek;
    int i_size = stream_Peek( p_demux->s, &p_peek, MAX_LINE );
    i_size -= sizeof("sgiNameServerHost=") - 1;
    if ( i_size > 0 ) {
        while ( i_size
            && strncasecmp( p_peek, "sgiNameServerHost=", sizeof("sgiNameServerHost=") - 1 ) )
        {
            p_peek++;
            i_size--;
        }
        if ( !strncasecmp( p_peek, "sgiNameServerHost=", sizeof("sgiNameServerHost=") -1 ) )
        {
            return VLC_SUCCESS;
        }
    }
    return VLC_EGENERIC;
}

/*****************************************************************************
 * Deactivate: frees unused data
 *****************************************************************************/
static void Deactivate( vlc_object_t *p_this )
{
    demux_t *p_demux = (demux_t*)p_this;
    demux_sys_t *p_sys = p_demux->p_sys;
    if( p_sys->psz_uri )
        free( p_sys->psz_uri );
    if( p_sys->psz_server )
        free( p_sys->psz_server );
    if( p_sys->psz_location )
        free( p_sys->psz_location );
    if( p_sys->psz_name )
        free( p_sys->psz_name );
    if( p_sys->psz_user )
        free( p_sys->psz_user );
    if( p_sys->psz_password )
        free( p_sys->psz_password );
    free( p_demux->p_sys );
    return;
}

static int ParseLine ( demux_t *p_demux, char *psz_line )
{
    char        *psz_bol;
    demux_sys_t *p_sys = p_demux->p_sys;

    psz_bol = psz_line;

    /* Remove unnecessary tabs or spaces at the beginning of line */
    while( *psz_bol == ' ' || *psz_bol == '\t' ||
           *psz_bol == '\n' || *psz_bol == '\r' )
    {
        psz_bol++;
    }

    if( !strncasecmp( psz_bol, "rtsp://", sizeof("rtsp://") - 1 ) )
    {
        /* We found the link, it was inside a sgiQTFileBegin */
        p_sys->psz_uri = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "Stream=\"", sizeof("Stream=\"") - 1 ) )
    {
        psz_bol += sizeof("Stream=\"") - 1;
        if ( !psz_bol )
            return 0;
        strrchr( psz_bol, '"' )[0] = '\0';
        /* We cheat around xdma. for some reason xdma links work different then rtsp */
        if( !strncasecmp( psz_bol, "xdma://", sizeof("xdma://") - 1 ) )
        {
            psz_bol[0] = 'r';
            psz_bol[1] = 't';
            psz_bol[2] = 's';
            psz_bol[3] = 'p';
        }
        p_sys->psz_uri = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "sgiNameServerHost=", sizeof("sgiNameServerHost=") - 1 ) )
    {
        psz_bol += sizeof("sgiNameServerHost=") - 1;
        p_sys->psz_server = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "sgiMovieName=", sizeof("sgiMovieName=") - 1 ) )
    {
        psz_bol += sizeof("sgiMovieName=") - 1;
        p_sys->psz_location = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "sgiUserAccount=", sizeof("sgiUserAccount=") - 1 ) )
    {
        psz_bol += sizeof("sgiUserAccount=") - 1;
        p_sys->psz_user = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "sgiUserPassword=", sizeof("sgiUserPassword=") - 1 ) )
    {
        psz_bol += sizeof("sgiUserPassword=") - 1;
        p_sys->psz_password = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "sgiShowingName=", sizeof("sgiShowingName=") - 1 ) )
    {
        psz_bol += sizeof("sgiShowingName=") - 1;
        p_sys->psz_name = strdup( psz_bol );
    }
    else if( !strncasecmp( psz_bol, "sgiDuration=", sizeof("sgiDuration=") - 1 ) )
    {
        psz_bol += sizeof("sgiDuration=") - 1;
        p_sys->i_duration = (mtime_t) strtol( psz_bol, NULL, 0 );
    }
    else if( !strncasecmp( psz_bol, "sgiRtspPort=", sizeof("sgiRtspPort=") - 1 ) )
    {
        psz_bol += sizeof("sgiRtspPort=") - 1;
        p_sys->i_port = (int) strtol( psz_bol, NULL, 0 );
    }else
    {
        /* This line isn't really important */
        return 0;
    }
    return VLC_SUCCESS;
}

/*****************************************************************************
 * Demux: reads and demuxes data packets
 *****************************************************************************
 * Returns -1 in case of error, 0 in case of EOF, 1 otherwise
 *****************************************************************************/
static int Demux ( demux_t *p_demux )
{
    demux_sys_t     *p_sys = p_demux->p_sys;
    playlist_t      *p_playlist;
    
    char            *psz_line;
    int             i_position;

    p_playlist = (playlist_t *) vlc_object_find( p_demux, VLC_OBJECT_PLAYLIST,
                                                 FIND_ANYWHERE );
    if( !p_playlist )
    {
        msg_Err( p_demux, "can't find playlist" );
        return -1;
    }

    p_playlist->pp_items[p_playlist->i_index]->b_autodeletion = VLC_TRUE;
    i_position = p_playlist->i_index + 1;

    while( ( psz_line = stream_ReadLine( p_demux->s ) ) )
    {
        ParseLine( p_demux, psz_line );
        if( psz_line ) free( psz_line );
    }
    
    if( p_sys->psz_uri == NULL )
    {
        if( p_sys->psz_server && p_sys->psz_location )
        {
            char *temp;
            
            temp = (char *)malloc( sizeof("rtsp/live://" ":" "123456789") +
                                       strlen( p_sys->psz_server ) + strlen( p_sys->psz_location ) );
            sprintf( temp, "rtsp/live://" "%s:%i%s",
                     p_sys->psz_server, p_sys->i_port > 0 ? p_sys->i_port : 554, p_sys->psz_location );
            
            p_sys->psz_uri = strdup( temp );
            free( temp );
        }
    }
    
    playlist_AddExt( p_playlist, p_sys->psz_uri,
        p_sys->psz_name ? p_sys->psz_name : p_sys->psz_uri,
        PLAYLIST_INSERT, i_position, p_sys->i_duration, NULL, 0 );

    vlc_object_release( p_playlist );
    return VLC_SUCCESS;
}

static int Control( demux_t *p_demux, int i_query, va_list args )
{
    return VLC_EGENERIC;
}

