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

#ifndef __MAIN_FRAME_H__
#define __MAIN_FRAME_H__

/**
 * @defgroup gui
 * @brief This module contains the classes that manage the main frame and its contents.
 * This includes the management of the main pane and its various areas, including the track
 * area (even though the contents of each track is managed by module Editors)
 */

#include <wx/frame.h>
#include <wx/toolbar.h>
#include <wx/panel.h>

#include "AriaCore.h"
#include "GUI/GraphicalSequence.h"
#include "Midi/Sequence.h"
#include "ptr_vector.h"
#include "Utils.h"

class wxButton;
class wxScrollBar;
class wxSpinCtrl;
class wxSpinEvent;
class wxStaticText;
class wxCommandEvent;
class wxFlexGridSizer;
class wxTextCtrl;
class wxBoxSizer;

namespace AriaMaestosa
{
    class CustomNoteSelectDialog;
    class Sequence;
    class PreferencesDialog;
    class AboutDialog;
    class InstrumentChoice;
    class DrumChoice;
    class VolumeSlider;
    class TuningPicker;
    class KeyPicker;

    // events useful if you need to show a
    // progress bar from another thread
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_SHOW_WAIT_WINDOW,   -1)
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_UPDATE_WAIT_WINDOW, -1)
    DECLARE_LOCAL_EVENT_TYPE(wxEVT_HIDE_WAIT_WINDOW,   -1)

    const int SHOW_WAIT_WINDOW_EVENT_ID = 100001;
    const int UPDT_WAIT_WINDOW_EVENT_ID = 100002;
    const int HIDE_WAIT_WINDOW_EVENT_ID = 100003;


#define MAKE_SHOW_PROGRESSBAR_EVENT(eventname, message, time_known) wxCommandEvent eventname( wxEVT_SHOW_WAIT_WINDOW, SHOW_WAIT_WINDOW_EVENT_ID ); eventname.SetString(message); eventname.SetInt(time_known)
#define MAKE_UPDATE_PROGRESSBAR_EVENT(eventname, progress) wxCommandEvent eventname( wxEVT_UPDATE_WAIT_WINDOW, UPDT_WAIT_WINDOW_EVENT_ID ); eventname.SetInt(progress)
#define MAKE_HIDE_PROGRESSBAR_EVENT(eventname) wxCommandEvent eventname( wxEVT_HIDE_WAIT_WINDOW, HIDE_WAIT_WINDOW_EVENT_ID )

#ifndef __WXMAC__
#define NO_WX_TOOLBAR
#endif


#ifdef NO_WX_TOOLBAR
#define TOOLBAR_BASE wxPanel
#else
#define TOOLBAR_BASE wxToolBar
#endif

    /**
      * @ingroup gui
      * @brief   class used to abstract differences between wxMac and wxGTK, providing a non-native
      *          toolbar where the native one is not suitable for Aria
      */
    class CustomToolBar : public TOOLBAR_BASE
    {
    public:

#ifdef NO_WX_TOOLBAR
        std::vector<wxString> labels;
        wxFlexGridSizer* toolbarSizer;
#endif

    public:

#ifdef NO_WX_TOOLBAR
        void AddSeparator() {}
        void AddTool(const int id, wxString label, wxBitmap& bmp);
        void SetToolNormalBitmap(const int id, wxBitmap& bmp);
        void EnableTool(const int id, const bool enabled);
        void AddStretchableSpace();
#endif
        CustomToolBar(wxWindow* parent);
        void add(wxControl* ctrl, wxString label=wxEmptyString);
        void realize();
    };

    /**
      * @ingroup gui
      * @brief manages the main frame of Aria Maestosa
      */
    class MainFrame : public wxFrame, public IPlaybackModeListener, public IActionStackListener,
        public ISequenceDataListener, public ICurrentSequenceProvider
    {
        WxOwnerPtr<CustomNoteSelectDialog>  m_custom_note_select_dialog;

        wxPanel* m_main_panel;
        wxFlexGridSizer* m_border_sizer;
        CustomToolBar* m_toolbar;

        wxScrollBar* m_horizontal_scrollbar;
        wxScrollBar* m_vertical_scrollbar;

        wxButton*   m_time_sig;
        wxTextCtrl* m_first_measure;
        wxSpinCtrl* m_song_length;
        wxSpinCtrl* m_display_zoom;

        wxMenu* m_file_menu;
        wxMenu* m_edit_menu;
        wxMenu* m_settings_menu;
        wxMenu* m_track_menu;
        wxMenu* m_help_menu;

        wxBitmap m_play_bitmap;
        wxBitmap m_pause_bitmap;
        wxBitmap m_tool1_bitmap;
        wxBitmap m_tool2_bitmap;

        wxMenuItem* m_follow_playback_menu_item;

        wxMenuItem* m_play_during_edits_always;
        wxMenuItem* m_play_during_edits_onchange;
        wxMenuItem* m_play_during_edits_never;

        wxMenuItem* m_channel_management_automatic;
        wxMenuItem* m_channel_management_manual;
        wxMenuItem* m_expanded_measures_menu_item;
        
        wxMenuItem* m_metronome;

        wxPanel*      m_notification_panel;
        wxStaticText* m_notification_text;
        
        wxBoxSizer* m_root_sizer;
        
        int m_current_sequence;
        
        /** Contains all open sequences */
        ptr_vector<GraphicalSequence> m_sequences;

    public:
        LEAK_CHECK();

        // READ AND WRITE
        bool changingValues; // set this to true when modifying the controls in the top bar, this allows to ignore all events thrown by their modification.
        wxTextCtrl* m_tempo_ctrl;

        // ------- read-only -------
        int m_play_during_edit; // what is the user's preference for note preview during edits
        bool m_playback_mode;
        MainPane*                     m_main_pane;
        WxOwnerPtr<PreferencesDialog> m_preferences;
        OwnerPtr<InstrumentPicker>    m_instrument_picker;
        OwnerPtr<DrumPicker>          m_drumKit_picker;
        OwnerPtr<TuningPicker>        m_tuning_picker;
        OwnerPtr<KeyPicker>           m_key_picker;
        // ----------------------

        MainFrame();
        void init();
        void initMenuBar();
        ~MainFrame();

        // top bar controls updated
        void tempoChanged(wxCommandEvent& evt);

        void updateUndoMenuLabel();

        /** Called whenever the user edits the text field containing song length. */
        void songLengthChanged(wxSpinEvent& evt);
        void zoomChanged(wxSpinEvent& evt);
        void songLengthTextChanged(wxCommandEvent& evt);
        void zoomTextChanged(wxCommandEvent& evt);
        void timeSigClicked(wxCommandEvent& evt);
        void toolButtonClicked(wxCommandEvent& evt);
        //void measureDenomChanged(wxCommandEvent& evt);
        void firstMeasureChanged(wxCommandEvent& evt);
        void changeMeasureAmount(int i, bool throwEvent=true);
        void disableMenus(const bool disable);
        void onHideNotifBar(wxCommandEvent& evt);

        void enterPressedInTopBar(wxCommandEvent& evt);

        // wait window events
        void evt_showWaitWindow(wxCommandEvent& evt);
        void evt_updateWaitWindow(wxCommandEvent& evt);
        void evt_hideWaitWindow(wxCommandEvent& evt);

        // menus
        void on_close(wxCloseEvent& evt);
        void menuEvent_new(wxCommandEvent& evt);
        void menuEvent_close(wxCommandEvent& evt);
        void menuEvent_exportNotation(wxCommandEvent& evt);
        void menuEvent_open(wxCommandEvent& evt);
        void menuEvent_copy(wxCommandEvent& evt);
        void menuEvent_undo(wxCommandEvent& evt);
        void menuEvent_paste(wxCommandEvent& evt);
        void menuEvent_pasteAtMouse(wxCommandEvent& evt);
        void menuEvent_save(wxCommandEvent& evt);
        bool doSave();
        void menuEvent_saveas(wxCommandEvent& evt);
        bool doSaveAs();
        void menuEvent_selectNone(wxCommandEvent& evt);
        void menuEvent_selectAll(wxCommandEvent& evt);
        void menuEvent_addTrack(wxCommandEvent& evt);
        void menuEvent_deleteTrack(wxCommandEvent& evt);
        void menuEvent_trackBackground(wxCommandEvent& evt);
        void menuEvent_importmidi(wxCommandEvent& evt);
        void menuEvent_exportmidi(wxCommandEvent& evt);
        void menuEvent_exportSampledAudio(wxCommandEvent& evt);
        void menuEvent_customNoteSelect(wxCommandEvent& evt);
        void menuEvent_snapToGrid(wxCommandEvent& evt);
        void menuEvent_scale(wxCommandEvent& evt);
        void menuEvent_copyright(wxCommandEvent& evt);
        void menuEvent_preferences(wxCommandEvent& evt);
        void menuEvent_followPlayback(wxCommandEvent& evt);
        void menuEvent_removeOverlapping(wxCommandEvent& evt);
        void menuEvent_playAlways(wxCommandEvent& evt);
        void menuEvent_playOnChange(wxCommandEvent& evt);
        void menuEvent_playNever(wxCommandEvent& evt);
        void menuEvent_quit(wxCommandEvent& evt);
        void menuEvent_about(wxCommandEvent& evt);
        void menuEvent_manual(wxCommandEvent& evt);
        void menuEvent_automaticChannelModeSelected(wxCommandEvent& evt);
        void menuEvent_manualChannelModeSelected(wxCommandEvent& evt);
        void menuEvent_expandedMeasuresSelected(wxCommandEvent& evt);
        void menuEvent_metronome(wxCommandEvent& evt);
        
        void updateMenuBarToSequence();

        // ---- playback
        void songHasFinishedPlaying();
        void toolsEnterPlaybackMode();
        void toolsExitPlaybackMode();
        void playClicked(wxCommandEvent& evt);
        void stopClicked(wxCommandEvent& evt);

        // ---- I/O
        void updateTopBarAndScrollbarsForSequence(const GraphicalSequence* seq);

        /** Opens the .aria file in filepath, reads it and prepares the editor to display and edit it. */
        void loadAriaFile(wxString path);

        /** Opens the .mid file in filepath, reads it and prepares the editor to display and edit it. */
        void loadMidiFile(wxString path);

#ifdef __WXMSW__
        void onDropFile(wxDropFilesEvent& event);
#endif

        void onMouseWheel(wxMouseEvent& event);

        // ---- scrollbars
        void updateVerticalScrollbar();

        /** Called to update the horizontal scrollbar, usually because song length has changed. */
        void updateHorizontalScrollbar(int thumbPos=-1);

        /**
         * User scrolled horizontally by dragging.
         * Just make sure to update the display to the new values.
         */
        void horizontalScrolling(wxScrollEvent& evt);

        /**
         * User scrolled vertically by dragging.
         * Just make sure to update the display to the new values.
         */
        void verticalScrolling(wxScrollEvent& evt);

        /**
         * User scrolled horizontally by clicking oin the arrows.
         * We need to ensure it scrolls of one whole measure (cause scrolling pixel by pixel horizontally would be
         * too slow) We by the way need to make sure it doesn't get out of bounds, in which case we need to put the
         * scrollbar back into correct position.
         */
        void horizontalScrolling_arrows(wxScrollEvent& evt);

        /**
         * User scrolled vertically by clicking on the arrows.
         * Just make sure to update the display to the new values.
         */
        void verticalScrolling_arrows(wxScrollEvent& evt);

        // ---- sequences

        /** Add a new sequence. There can be multiple sequences if user opens or creates multiple files. */
        void addSequence();

        /** Returns the amount of open sequences (files). */
        int getSequenceAmount() const { return m_sequences.size(); }

        /**
         * Close an open sequence.
         *
         * @param id  ID of the sequence to close, from 0 to sequence amount -1 (or -1 to mean "current sequence")
         */
        bool closeSequence(int id = -1);

        /** Returns the sequence (file) currently being active. */
        virtual Sequence*  getCurrentSequence();
        Sequence*          getSequence(int n);
        GraphicalSequence* getCurrentGraphicalSequence();
        GraphicalSequence* getGraphicalSequence(int n);
        
        int  getCurrentSequenceID() const { return m_current_sequence; }
        void setCurrentSequence(int n);

        void changeShownTimeSig(int num, int denom);

        void evt_freeVolumeSlider( wxCommandEvent& evt );
        void evt_freeTimeSigPicker( wxCommandEvent& evt );

        void addIconItem(wxMenu* menu, int menuID, const wxString& label, const wxString& stockIconId);

        /** @brief Implement callback from IPlaybackModeListener */
        virtual void onEnterPlaybackMode();

        /** @brief Implement callback from IPlaybackModeListener */
        virtual void onLeavePlaybackMode();

        /** @brief Implement callback from IActionStackListener */
        virtual void onActionStackChanged();

        /** @brief Implement callback from ISequenceDataListener */
        virtual void onSequenceDataChanged();

        DECLARE_EVENT_TABLE();
    };

}

#endif
