/*****************************************************************************
 * Header for Common SVCD and VCD subtitle routines.
 *****************************************************************************
 * Copyright (C) 2003 VideoLAN
 * $Id: common.h,v 1.2 2003/12/30 04:43:52 rocky Exp $
 *
 * Author: Rocky Bernstein
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

void           VCDSubClose  ( vlc_object_t * );
void           VCDSubInitSubtitleBlock( decoder_sys_t * p_sys );
void           VCDSubInitSubtitleData(decoder_sys_t *p_sys);
void           VCDSubAppendData( decoder_t *p_dec, uint8_t *buffer, 
				 uint32_t buf_len );
vout_thread_t *VCDSubFindVout( decoder_t *p_dec );

void           VCDSubScaleX( decoder_t *p_dec, subpicture_t *p_spu, 
			     unsigned int i_scale_x, unsigned int i_scale_y );
void           VCDSubDestroySPU( subpicture_t *p_spu );
int            VCDSubCropCallback( vlc_object_t *p_object, char const *psz_var,
				   vlc_value_t oldval, vlc_value_t newval, 
				   void *p_data );
void           VCDSubUpdateSPU( subpicture_t *p_spu, vlc_object_t *p_object );
void           VCDInlinePalette ( /*inout*/ uint8_t *p_dest, 
				  decoder_sys_t *p_sys, unsigned int i_height, 
				  unsigned int i_width );



