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

#ifndef __GRAPHICAL_SEQUENCE_H__
#define __GRAPHICAL_SEQUENCE_H__

#include "GUI/GraphicalTrack.h"
#include "GUI/MeasureBar.h"
#include "ptr_vector.h"

namespace AriaMaestosa
{
    class Sequence;
    class MainPane;
    
    class GraphicalSequence
    {
        OwnerPtr<Sequence> m_sequence;
        OwnerPtr<MeasureBar>  m_measure_bar;

        // dock
        ptr_vector<GraphicalTrack, REF> dock;
        int dockHeight;
        
        bool maximize_track_mode;
        
        float m_zoom; int m_zoom_percent;
        
        float m_x_scroll_in_pixels;
        int y_scroll;
        
        /** used when reordering tracks, to hold the new position of the track being moved */
        int reordering_newPosition;
        
        /** while reordering tracks, contains the vertical scrolling amount */
        int reorderYScroll;
        
        // FIXME(DESIGN): remove friendship
        friend class GraphicalTrack;
        friend class MainPane;
        
    public:
        
        /** 
         * will store the horizontal scrolling when copying, and upon pasting behaviour will depend if
         * x_scroll has changed since copy
         */
        int x_scroll_upon_copying; // TODO: make private
        
        GraphicalSequence(Sequence* sequence);
        
        Sequence* getModel() { return m_sequence; }
        const Sequence* getModel() const { return m_sequence.raw_ptr; }

        int   getZoomInPercent() const { return m_zoom_percent; }
        float getZoom         () const { return m_zoom;         }
        void  setZoom(int percent);
        
        void  setXScrollInMidiTicks(int value);
        void  setXScrollInPixels(int value);
        int   getXScrollInMidiTicks() const;
        int   getXScrollInPixels() const { return round(m_x_scroll_in_pixels); }
        
        void  setYScroll(int value);
        int   getYScroll() const { return y_scroll; }
        
        /**
         * @return the number of pixels it takes to draw all tracks, vertically.
         * @note   This is used mostly by the code managing the vertical scrollbar.
         */    
        int   getTotalHeight() const;
        
        void renderTracks(int currentTick, RelativeXCoord mousex, int mousey, int mousey_initial, int from_y);
        
        /** @brief called repeatedly when mouse is held down */
        void mouseHeldDown(RelativeXCoord mousex_current, int mousey_current,
                           RelativeXCoord mousex_initial, int mousey_initial);
        
        /** @return do we need to start a timer that will frequently send mouse held down events? */
        bool areMouseHeldDownEventsNeeded();
        
        /** @brief Hide a track by sending it to the 'dock' */
        void addToDock(GraphicalTrack* track);
        
        void removeFromDock(GraphicalTrack* track);
        
        void pushYScroll(int delta) { y_scroll += delta; }
        
        /**
         * @brief called when mouse is released after having dragged a track.
         *
         * Called when a user has finished dragging the track to reorder it.
         * Where the track ends was calculated while drawing the preview - all this methods needs to do is
         * remove the track from its curren location and move it to its new location.
         */    
        void reorderTracks();
        
        MeasureBar* getMeasureBar() { return m_measure_bar; }
        
        Track* addTrack();
        
        /** @brief Called before loading, prepares empty tracks */
        void prepareEmptyTracksForLoading(int amount);
        
        void copy();
        
        void saveToFile(wxFileOutputStream& fileout);
        bool readFromFile(irr::io::IrrXMLReader* xml);
    };
    
}


#endif