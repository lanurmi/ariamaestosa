/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef __VOLUME_SLIDER_H__
#define __VOLUME_SLIDER_H__

#include <wx/event.h>

#include "Utils.h"

namespace AriaMaestosa
{
    
    class Note; // forward
    class Track;
    
    
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_DESTROY_VOLUME_SLIDER, -1)
    const int DESTROY_SLIDER_EVENT_ID = 100000;
    
     bool isVolumeSliderShown();
    
    /**
      * @ingroup pickers
      * @brief show small frame used to pick a note volume (velocity)
      */
    void showVolumeSlider(int x, int y, int noteID, Track* track);
    
    
    /**
      * @ingroup pickers
      * @brief show small frame used to pick a track volume
      */
    void showVolumeSlider(int x, int y, Track* track);
    
    
    /**
      * @ingroup pickers
      * @brief hide and delete small frame used to pick a note volume (velocity)
      */
    void freeVolumeSlider();
    
}

#endif
