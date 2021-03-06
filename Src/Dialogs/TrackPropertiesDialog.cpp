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

#include "AriaCore.h"
#include "Dialogs/TrackPropertiesDialog.h"
#include "Editors/Editor.h"
#include "GUI/GraphicalSequence.h"
#include "GUI/GraphicalTrack.h"
#include "GUI/MainFrame.h"
#include "Midi/DrumChoice.h"
#include "Midi/InstrumentChoice.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"

#include <wx/tokenzr.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/utils.h>
#include <wx/slider.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <iostream>

namespace AriaMaestosa
{
    class BackgroundChoicePanel : public wxPanel
    {
    public:
        LEAK_CHECK();
        
        wxBoxSizer* sizer;
        wxCheckBox* active;
        
        BackgroundChoicePanel(wxWindow* parent, const int trackID, Track* track, const bool activated=false, const bool enabled = false) : wxPanel(parent)
        {
            sizer = new wxBoxSizer(wxHORIZONTAL);
            
            const bool isDrum = track->isNotationTypeEnabled(DRUM);
            
            // checkbox
            active = new wxCheckBox(this, 200, wxT(" "));
            sizer->Add(active, 0, wxALL, 5);
            
            if (not enabled or isDrum)  active->Enable(false);
            else if (activated)         active->SetValue(true);
            
            wxString instrumentname;
            if (isDrum) instrumentname = DrumChoice::getDrumkitName( track->getDrumKit() );
            else        instrumentname = InstrumentChoice::getInstrumentName( track->getInstrument() );
            
            sizer->Add( new wxStaticText(this, wxID_ANY, to_wxString(trackID) + wxT(" : ") + track->getName() +
                                         wxT(" (") + instrumentname + wxT(")")) , 1, wxALL, 5);
            
            SetSizer(sizer);
            sizer->Layout();
            sizer->SetSizeHints(this); // resize to take ideal space
        }
        
        bool isChecked()
        {
            return active->GetValue();
        }
        
        void setChecked(bool checked)
        {
            if (checked)
            {
                // May only be checked if enabled
                active->SetValue(active->IsEnabled());
            }
            else
            {
                active->SetValue(false);
            }
            
        }
        
    };
    
    /**
      * @ingroup dialogs
      * @brief The dialog where per-track properties may be edited
      * @note this is a private class, it won't be instanciated directly
      * @see TrackProperties::show
      */
    class TrackPropertiesDialog : public wxDialog
    {
        wxButton* m_ok_btn;
        wxButton* m_cancel_btn;
        
        wxButton* m_checkall_btn;
        wxButton* m_uncheckall_btn;
        
        wxBoxSizer* m_sizer;
        
        wxTextCtrl* m_volume_text;
        wxSlider* m_volume_slider;
        
        ptr_vector<BackgroundChoicePanel> m_choice_panels;
        
        GraphicalTrack* m_parent;
        int m_modalid;
        bool m_ignore_events;
        
    
    public:
        LEAK_CHECK();
        
        ~TrackPropertiesDialog()
        {
        }
        
        TrackPropertiesDialog(GraphicalTrack* parent) :
        wxDialog(getMainFrame(), wxID_ANY, wxString::Format(_("Track '%s' Properties"),
                                                   (const char*)parent->getTrack()->getName().c_str()),
                 wxPoint(100,100), wxSize(700,300), wxCAPTION | wxCLOSE_BOX | wxSTAY_ON_TOP )
        {
            bool enableBackgroundTracks;
        
            m_ignore_events = true;
            m_parent = parent;
            
            Track* parent_t = parent->getTrack();
            Sequence* seq = parent->getSequence()->getModel();
            m_modalid = -1;
            m_sizer = new wxBoxSizer(wxVERTICAL);
            wxPanel* properties_panel = new wxPanel(this);
            
            enableBackgroundTracks = parent_t->isNotationTypeEnabled(KEYBOARD) || parent_t->isNotationTypeEnabled(SCORE);
            
            m_sizer->Add(properties_panel, 1, wxEXPAND | wxALL, 5);

            // TODO: adapt with new multi-editor paradigm
            Editor* editor = parent->getFocusedEditor();
            
            wxBoxSizer* props_sizer = new wxBoxSizer(wxHORIZONTAL);
            
            // ------ track background -----
            if (enableBackgroundTracks)
            {
                wxStaticBoxSizer* bg_subsizer = new wxStaticBoxSizer(wxVERTICAL, properties_panel, _("Track Background"));
                
                const int trackAmount = seq->getTrackAmount();
                int enabledTrackCount = 0;
                for (int n=0; n<trackAmount; n++)
                {
                    Track* track = seq->getTrack(n);
                    bool enabled = true;
                    if (track == parent_t) enabled = false; // can't be background of itself
                    
                    if (enabled && !track->isNotationTypeEnabled(DRUM)) enabledTrackCount++;
                    
                    bool activated = false;
                    if (editor->hasAsBackground(track)) activated = true;
                    
                    BackgroundChoicePanel* bcp = new BackgroundChoicePanel(properties_panel, n, track, activated, enabled);
                    bg_subsizer->Add(bcp, 0, wxALL, 5);
                    m_choice_panels.push_back(bcp);
                }
                
                // Adds selection buttons if necessary
                if (enabledTrackCount>0)
                {
                    wxPanel* checkPanel = new wxPanel(properties_panel);
                    wxBoxSizer* check_sizer = new wxBoxSizer(wxHORIZONTAL);
                    m_checkall_btn = new wxButton(checkPanel, wxID_ANY, _("Check all"));
                    m_uncheckall_btn = new wxButton(checkPanel, wxID_ANY, _("Uncheck all"));
                    check_sizer->Add(m_checkall_btn, 1, wxEXPAND |wxALL, 5);
                    check_sizer->Add(m_uncheckall_btn, 1, wxEXPAND |wxALL, 5);
                    
                    m_checkall_btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &TrackPropertiesDialog::ckeckAllButton, this);
                    m_uncheckall_btn->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &TrackPropertiesDialog::unckeckAllButton, this);
    
                    check_sizer->Layout();
                    check_sizer->SetSizeHints(this);
                    check_sizer->Fit(checkPanel);
                    checkPanel->SetSizer(check_sizer);
                    bg_subsizer->Add(checkPanel, 1, wxALL|wxEXPAND, 5);
                }
                
                props_sizer->Add(bg_subsizer, 0, wxALL, 5);
                bg_subsizer->Layout();
            }
            
            // ------ other properties ------
            wxBoxSizer* right_subsizer = new wxBoxSizer(wxVERTICAL);
            props_sizer->Add(right_subsizer, 0, wxALL, 5);
            
            wxStaticBoxSizer* default_volume_subsizer = new wxStaticBoxSizer(wxHORIZONTAL, properties_panel, _("Default volume for new notes"));
            
            m_volume_slider = new wxSlider(properties_panel, 300 /* ID */, 80 /* current */, 0 /* min */, 127 /* max */);
            default_volume_subsizer->Add(m_volume_slider, 0, wxALL, 5);
#ifdef __WXGTK__
            m_volume_slider->SetMinSize( wxSize(127,-1) );
#endif
            
            wxSize smallsize = wxDefaultSize;
            smallsize.x = 50;
            m_volume_text = new wxTextCtrl(properties_panel, 301, wxT("80"), wxDefaultPosition, smallsize);
            default_volume_subsizer->Add(m_volume_text, 0, wxALL, 5);
            
            right_subsizer->Add(default_volume_subsizer, 0, wxALL, 0);
            
            //right_subsizer->Add( new wxButton(properties_panel, wxID_ANY, wxT("Editor-specific options")), 1, wxALL, 5 );
            
            properties_panel->SetSizer(props_sizer);
            
            default_volume_subsizer->Layout();
            right_subsizer->Layout();
            props_sizer->Layout();
            props_sizer->SetSizeHints(properties_panel);
            
            // ------ bottom OK/cancel buttons ----
            m_ok_btn = new wxButton(this, wxID_OK, _("OK"));
            m_ok_btn->SetDefault();
            
            m_cancel_btn = new wxButton(this, wxID_CANCEL,  _("Cancel"));

            wxStdDialogButtonSizer* stdDialogButtonSizer = new wxStdDialogButtonSizer();
            stdDialogButtonSizer->AddButton(m_ok_btn);
            stdDialogButtonSizer->AddButton(m_cancel_btn);
            stdDialogButtonSizer->Realize();

            m_sizer->Add(stdDialogButtonSizer, 0, wxALL | wxEXPAND, 5);

            SetSizer(m_sizer);
            m_sizer->Layout();
            m_sizer->SetSizeHints(this); // resize window to take ideal space
                                       // FIXME - if too many tracks for current screen space, may cause problems
            
            m_ignore_events = false;
        }
        
        void show()
        {
            Center();
            
            Track* t = m_parent->getTrack();
            m_volume_text->SetValue( to_wxString(t->getDefaultVolume()) );
            m_volume_slider->SetValue( t->getDefaultVolume() );
            
            m_modalid = ShowModal();
        }
        
        void ckeckAllButton(wxCommandEvent& evt)
        {
            int size = m_choice_panels.size();
            
            for (int i=0 ; i<size ; i++)
            {
                m_choice_panels[i].setChecked(true);
            }
        }
        
        void unckeckAllButton(wxCommandEvent& evt)
        {
            int size = m_choice_panels.size();
            
            for (int i=0 ; i<size ; i++)
            {
                m_choice_panels[i].setChecked(false);
            }
        }
        
        /** when Cancel button of the tuning picker is pressed */
        void cancelButton(wxCommandEvent& evt)
        {
            wxDialog::EndModal(m_modalid);
        }
        
        /** when OK button of the tuning picker is pressed */
        void okButton(wxCommandEvent& evt)
        {
            const int value = atoi_u(m_volume_text->GetValue());
            if (value >=0 and value < 128)
            {
                m_parent->getTrack()->setDefaultVolume( value );
            }
            else
            {
                wxBell();
            }
            
            const int amount = m_choice_panels.size();
            Sequence* seq = m_parent->getSequence()->getModel();
            
            // TODO: adapt with new multi-editor paradigm
            Editor* editor = m_parent->getFocusedEditor();
            editor->clearBackgroundTracks();
            
            for (int n=0; n<amount; n++)
            {
                if (m_choice_panels[n].isChecked())
                {
                    editor->addBackgroundTrack( seq->getTrack(n) );
                    std::cout << "Adding track " << n << " as background to track " << std::endl;
                }
            }
            
            wxDialog::EndModal(m_modalid);
            Display::render();
        }
        
        void volumeSlideChanging(wxScrollEvent& evt)
        {
            // FIXME: an apparent wxGTK bug sends events before the constructor even returned
            if (m_ignore_events) return;
            
            const int value = m_volume_slider->GetValue();
            if (value >=0 and value < 128) m_volume_text->SetValue( to_wxString(value) );
        }
        
        void volumeTextChanged(wxCommandEvent& evt)
        {
            // FIXME: an apparent wxGTK/wxMSW bug(?) sends events before the constructor even returned
            if (m_ignore_events) return;
            
            const int value = atoi_u(m_volume_text->GetValue());
            m_volume_slider->SetValue( value );
        }
        DECLARE_EVENT_TABLE()
    };
    
    BEGIN_EVENT_TABLE(TrackPropertiesDialog, wxDialog)
    EVT_BUTTON(wxID_OK, TrackPropertiesDialog::okButton)
    EVT_BUTTON(wxID_CANCEL, TrackPropertiesDialog::cancelButton)
    
    EVT_COMMAND_SCROLL_THUMBTRACK(300, TrackPropertiesDialog::volumeSlideChanging)
    EVT_COMMAND_SCROLL_THUMBRELEASE(300, TrackPropertiesDialog::volumeSlideChanging)
    EVT_COMMAND_SCROLL_LINEUP(300, TrackPropertiesDialog::volumeSlideChanging)
    EVT_COMMAND_SCROLL_LINEDOWN(300, TrackPropertiesDialog::volumeSlideChanging)
    EVT_COMMAND_SCROLL_PAGEUP(300, TrackPropertiesDialog::volumeSlideChanging)
    EVT_COMMAND_SCROLL_PAGEDOWN(300, TrackPropertiesDialog::volumeSlideChanging)
    
    EVT_TEXT(301, TrackPropertiesDialog::volumeTextChanged)
    
    END_EVENT_TABLE()
    
    
    
    namespace TrackProperties
    {
        
        
        void show(GraphicalTrack* parent)
        {
            TrackPropertiesDialog frame(parent);
            frame.show();
        }
        
    }
    
    
}
