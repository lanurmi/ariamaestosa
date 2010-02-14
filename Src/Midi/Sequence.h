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

#ifndef _sequence_
#define _sequence_

#include "wx/wx.h"
#include "wx/wfstream.h"

#include "irrXML/irrXML.h"

#include "Config.h"
#include "ptr_vector.h"
#include "GUI/GraphicalTrack.h"
#include "Midi/Track.h"
#include "Editors/RelativeXCoord.h"
#include "Renderers/RenderAPI.h"

#include "Config.h"
#include "AriaCore.h"

#include <string>


namespace AriaMaestosa {

class ControllerEvent;
class MeasureBar;

class Sequence {

    int tempo;
    int beatResolution;

    float zoom; int zoom_percent;

    int x_scroll_in_pixels, y_scroll;
    //int measureWidth;

    int reordering_newPosition; // used when reordering tracks, to hold the new position of the track being moved
    int reorderYScroll; // while reordering tracks, contains the vertical scrolling amount

    wxString copyright;
    wxString internal_sequenceName;

    int currentTrack;

    ptr_vector<Track> tracks;

    ChannelManagementType channelManagement;

    ptr_vector<Action::EditAction> undoStack;

 public:

    LEAK_CHECK();

    //FIXME: remove read-write public members!
    /** this object is to be modified by MainFrame, to remember where to save this sequence */
    wxString filepath;

    // ---- these variables are to be modified by tracks
    
    /** will store the horizontal scrolling when copying, and upon pasting behaviour will depend if
      * x_scroll has changed since copy
      */
    int x_scroll_upon_copying;
    
    /** if no scrolling is done, this value will be used to determine where to place notes */
    int notes_shift_when_no_scrolling;
    
    bool maximize_track_mode;
    
    // ------------ read-only -------------
    /** set to true when importing - indicates the sequence will have frequent changes and not compute
      * too much until it's over
      */
    bool importing;

    // set this flag true to follow playback
    bool follow_playback;

    // dock
    int dockSize;
    ptr_vector<GraphicalTrack, REF> dock;
    int dockHeight;

    AriaRenderString sequenceFileName;
    OwnerPtr<MeasureData>  measureData;

    ptr_vector<ControllerEvent> tempoEvents;
    // ------------------------------------

    /*
     * @brief perform an action that affects multiple tracks
     *
     * This is the method called for performing any action that can be undone.
     * A EditAction object is used to describe the task, and it also knows how to revert it.
     * The EditAction objects are kept in a stack in Sequence in order to offer multiple undo levels.
     *
     * Sequence::action does actions that affect all tracks. Also see Track::action.
     */
    void action( Action::MultiTrackAction* action );

    /** you do not need to call this yourself, Track::action and Sequence::action do. */
    void addToUndoStack( Action::EditAction* action );
    
    void undo();
    
    /** forbid undo, drop all undo information kept in memory. */
    void clearUndoStack();
    
    /** @return is there something to undo? */
    bool somethingToUndo();

    Sequence();
    ~Sequence();

    wxString suggestFileName();
    wxString suggestTitle();

    /** Hide a track by sending it to the 'dock' */
    void addToDock(GraphicalTrack* track);
    void removeFromDock(GraphicalTrack* track);

    void pushYScroll(int delta) { y_scroll += delta; }
    
    void spacePressed();
    void renderTracks(int currentTick, RelativeXCoord mousex, int mousey, int mousey_initial, int from_y);

    /**
     * @brief called when mouse is released after having dragged a track.
     *
     * Called when a user has finished dragging the track to reorder it.
     * Where the track ends was calculated while drawing the preview - all this methods needs to do is
     * remove the track from its curren location and move it to its new location.
     */    
    void reorderTracks();

    /** called repeatedly when mouse is held down */
    void mouseHeldDown(RelativeXCoord mousex_current, int mousey_current,
                       RelativeXCoord mousex_initial, int mousey_initial);

    /** do we need to start a timer that will frequently send mouse held down events? */
    bool areMouseHeldDownEventsNeeded();

    // tracks
    int getTrackAmount() const;
    int getCurrentTrackID() const;
    Track* getTrack(int ID);
    Track* getCurrentTrack();
    void setCurrentTrackID(int ID);
    void setCurrentTrack(Track* track);
    void addTrack();
    void deleteTrack();
    void deleteTrack(int ID);
    
    /** Called before loading, prepares empty tracks */    
    void prepareEmptyTracksForLoading(int amount);

    
    /**
     * @return the number of pixels it takes to draw all tracks, vertically.
     *         This is used mostly by the code managing the vertical scrollbar.
     */    
    int   getTotalHeight() const;
    
    void  setTempo(int tempo);
    int   getTempo() const;

    int   getZoomInPercent() const;
    float getZoom() const;
    void  setZoom(int percent);

    void  setXScrollInMidiTicks(int value);
    void  setXScrollInPixels(int value);
    int   getXScrollInMidiTicks();
    int   getXScrollInPixels();

    void  setYScroll(int value);
    int   getYScroll();

    void  addTempoEvent( ControllerEvent* evt );
    
    /** adds a tempo event. used during importing - then we know events are in time order
      * so no time is wasted verifying that
      */
    void  addTempoEvent_import( ControllerEvent* evt );

    void  setChannelManagementType(ChannelManagementType m);
    ChannelManagementType getChannelManagementType() const;

    /** called reacting to the user selecting "snap notes to grid" from the edit menu */
    void snapNotesToGrid();

    void setCopyright( wxString copyright );
    wxString getCopyright();
    void setInternalName(wxString name);
    wxString getInternalName();

    void scale(
               float factor,
               bool rel_first_note, bool rel_begin, // relative to what (only one must be true)
               bool affect_selection, bool affect_track, bool affect_song // scale what (only one must be true)
               );

    void copy();
    void paste();
    void pasteAtMouse();
    void selectAll();
    void selectNone();

    /**
     * @return Ticks per beat (the number of time units in a quarter note.)
     */
    int ticksPerBeat() const;
    
    /**
     * @param res Ticks per beat (the number of time units in a quarter note.)
     */
    void setTicksPerBeat(int res);

    // ---- serialization
    
    /** Called when saving <Sequence> ... </Sequence> in .aria file */
    void saveToFile(wxFileOutputStream& fileout);
    
    /** Called when reading <sequence> ... </sequence> in .aria file */
    bool readFromFile(irr::io::IrrXMLReader* xml);

};

}
#endif
