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


#include "Dialogs/Preferences.h"

#include <iostream>
#include <wx/config.h>
#include "AriaCore.h"

#include "GUI/MainFrame.h"

namespace AriaMaestosa {
	
BEGIN_EVENT_TABLE(Preferences, wxDialog)

EVT_CHOICE(1, Preferences::languageSelected)
EVT_BUTTON(2, Preferences::okClicked)
EVT_CHOICE(3, Preferences::playSelected)

END_EVENT_TABLE()

Preferences::Preferences(MainFrame* parent) : wxDialog(NULL, wxID_ANY,  _("Preferences"), wxPoint(100,100), wxSize(500,300), wxCAPTION )
{
	
	INIT_LEAK_CHECK();
	
	Preferences::parent = parent;
	
	vert_sizer = new wxBoxSizer(wxVERTICAL);
	
	// language
	{
	wxStaticText* lang_label = new wxStaticText(this, wxID_ANY,  _("Language"));
	vert_sizer->Add( lang_label, 0, wxALL, 10 );
	
	wxString choices[3] = { wxT("System"), wxT("English"), wxT("Fran\u00E7ais")};
		
	lang_combo = new wxChoice(this, 1, wxDefaultPosition, wxDefaultSize, 3, choices );
	vert_sizer->Add( lang_combo, 0, wxALL, 10 );
	}
	
	// play settings
	{
	wxStaticText* play_label = new wxStaticText(this, wxID_ANY,  _("Play during edit (default value)"));
	vert_sizer->Add( play_label, 0, wxALL, 10 );
	
	wxString choices[3] = { _("Always"),  _("On note change"),  _("Never")};
	
	play_combo = new wxChoice(this, 3, wxDefaultPosition, wxDefaultSize, 3, choices );
	vert_sizer->Add( play_combo, 0, wxALL, 10 );
	}
	

	wxConfig* prefs;
	prefs = (wxConfig*) wxConfig::Get();
	
	// --- read language from prefs -----
	long language;
	if(prefs->Read( wxT("lang"), &language) )
	{
		if(language == wxLANGUAGE_DEFAULT)
		{
			lang_combo->Select(0);
		}
		else if(language == wxLANGUAGE_ENGLISH)
		{
			lang_combo->Select(1);
		}
		else if(language == wxLANGUAGE_FRENCH)
		{
			lang_combo->Select(2);
		}
	}
	// --- read play settings from prefs -----
	long play_v;
	if(prefs->Read( wxT("playDuringEdit"), &play_v) )
	{
		if(play_v == PLAY_ALWAYS)
		{
			play_combo->Select(0);
			parent->play_during_edit = 0;
			
			wxCommandEvent useless;
			parent->menuEvent_playAlways(useless);
		}
		else if(play_v == PLAY_ON_CHANGE)
		{
			play_combo->Select(1);
			parent->play_during_edit = 1;
			
			wxCommandEvent useless;
			parent->menuEvent_playOnChange(useless);
		}
		else if(play_v == PLAY_NEVER)
		{
			play_combo->Select(2);
			parent->play_during_edit = 2;
			
			wxCommandEvent useless;
			parent->menuEvent_playNever(useless);
		}
	}
	else
	{
		play_combo->Select(1);
	}
	// -----------------------------------
	
	wxStaticText* effect_label = new wxStaticText(this, wxID_ANY,  _("Changes will take effect next time you open the app."));
	vert_sizer->Add( effect_label, 0, wxALL, 10 );
	
	ok_btn = new wxButton(this, 2, wxT("OK"));
	vert_sizer->Add( ok_btn, 0, wxALL, 10 );
	
	ok_btn -> SetDefault();
	
	SetSizer( vert_sizer );
	vert_sizer->Layout();
	vert_sizer->SetSizeHints( this );
}

void Preferences::show()
{
	Center();
	modalCode = ShowModal();
	//Show();
}

void Preferences::languageSelected(wxCommandEvent& evt)
{

	wxConfig* prefs = (wxConfig*) wxConfig::Get();
	
	// wxLANGUAGE_DEFAULT
	// wxLANGUAGE_ENGLISH
	// wxLANGUAGE_FRENCH
	
	if( lang_combo->GetStringSelection() == wxT("System") )
	{
		prefs->Write( wxT("lang"), wxLANGUAGE_DEFAULT );
	}
	else if( lang_combo->GetStringSelection() == wxT("English") )
	{
		prefs->Write( wxT("lang"), wxLANGUAGE_ENGLISH );
	}
	else if( lang_combo->GetStringSelection() == wxT("Fran\u00E7ais") )
	{
		prefs->Write( wxT("lang"), wxLANGUAGE_FRENCH );
	}
	
	prefs->Flush();
	
}

void Preferences::playSelected(wxCommandEvent& evt)
{	
	wxConfig* prefs = (wxConfig*) wxConfig::Get();
	prefs->Write( wxT("playDuringEdit"), play_combo->GetSelection() );
	prefs->Flush();
}


void Preferences::okClicked(wxCommandEvent& evt)
{
	wxDialog::EndModal(modalCode);
	//Hide();
}

}
