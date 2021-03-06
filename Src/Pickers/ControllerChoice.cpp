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


#include "Pickers/ControllerChoice.h"

#include "AriaCore.h"
#include "GUI/GraphicalTrack.h"
#include "GUI/MainFrame.h"
#include "GUI/MainPane.h"
#include "IO/IOUtils.h"
#include "Midi/Sequence.h"
#include "PreferencesData.h"
#include "Singleton.h"
#include "Utils.h"

#include <iostream>

namespace AriaMaestosa
{

static const wxString g_controller_names[] =
{
    wxT("Bank select"), // 0
    wxT("Modulation"), // 1, fine 33
    wxT("Breath"), // 2
    wxT(""), // 3
    wxT("Foot"), // 4
    wxT("Portamento Time"), // 5, fine 37
    wxT(""), // 6
    wxT("Volume"), // 7, fine 39
    wxT("Balance"), // 8
    wxT(""), // 9
    wxT("Pan"), // 10, fine 42
    wxT("Expression"), // 11
    wxT("Effect 1"), // 12
    wxT("Effect 2"), // 13
    wxT(""), // 14
    wxT(""), // 15
    wxT("General 1"), // 16
    wxT("General 2"), // 17
    wxT("General 3"), // 18
    wxT("General 4"), // 19
    wxT(""), // 20
    wxT(""), // 21
    wxT(""), // 22
    wxT(""), // 23
    wxT(""), // 24
    wxT(""), // 25
    wxT(""), // 26
    wxT(""), // 27
    wxT(""), // 28
    wxT(""), // 29
    // these strings aren't actually controllers, but it'll be quicker to re-use empty spots
    // so they can join the same openGL string list, and thus render faster
    wxT("Pitch Bend"), // 30
    wxT("Tempo (global)"), // 31
    wxT("Lyrics (global)"), // 32
    wxT("Instrument"), // 33
    wxT(""), // 34
    wxT(""), // 35
    wxT(""), // 36
    wxT(""), // 37
    wxT(""), // 38
    wxT(""), // 39
    wxT(""), // 40
    wxT(""), // 41
    wxT(""), // 42
    wxT(""), // 43
    wxT(""), // 44
    wxT(""), // 45
    wxT(""), // 46
    wxT(""), // 47
    wxT(""), // 48
    wxT(""), // 49
    wxT(""), // 50
    wxT(""), // 51
    wxT(""), // 52
    wxT(""), // 53
    wxT(""), // 54
    wxT(""), // 55
    wxT(""), // 56
    wxT(""), // 57
    wxT(""), // 58
    wxT(""), // 59
    wxT(""), // 60
    wxT(""), // 61
    wxT(""), // 62
    wxT(""), // 63
    wxT("Sustain"), // 64
    wxT("Portamento"), // 65
    wxT("Sostenuto"), // 66
    wxT("Soft Pedal"), // 67
    wxT("Legato footswitch"), // 68
    wxT("Hold"), // 69
    wxT("Timber Variation"), // 70
    wxT("Timber/Harmonic"), // 71
    wxT("Release Time"), // 72
    wxT("Attack Time"), // 73
    wxT("Brightness"), // 74
    wxT("Decay Time"), // 75
    wxT("Vibrato Rate"), // 76
    wxT("Vibrato Depth"), // 77
    wxT("Vibrato Delay"), // 78
    wxT(""), // 79
    wxT("General 5"), // 80
    wxT("General 6"), // 81
    wxT("General 7"), // 82
    wxT("General 8"), // 83
    wxT("Portamento Control"), // 84
    wxT(""), // 85
    wxT(""), // 86
    wxT(""), // 87
    wxT(""), // 88
    wxT(""), // 89
    wxT(""), // 90
    wxT("Reverb"), // 91
    wxT("Tremolo"), // 92
    wxT("Chorus"), // 93
    wxT("Celeste"), // 94
    wxT("Phaser"), // 95
    wxT(""), // 96
    wxT(""), // 97
    wxT(""), // 98
    wxT(""), // 99
    wxT(""), // 100
    wxT(""), // 101
    wxT(""), // 102
    wxT(""), // 103
    wxT(""), // 104
    wxT(""), // 105
    wxT(""), // 106
    wxT(""), // 107
    wxT(""), // 108
    wxT(""), // 109
    wxT(""), // 110
    wxT(""), // 111
    wxT(""), // 112
    wxT(""), // 113
    wxT(""), // 114
    wxT(""), // 115
    wxT(""), // 116
    wxT(""), // 117
    wxT(""), // 118
    wxT(""), // 119
    wxT("All sound off"), // 120
    wxT("Re set control"), // 121
    wxT(""), // 122
    wxT("All notes off"), // 123
    wxT(""), // 124
    wxT(""), // 125
    wxT(""), // 126
    wxT(""), // 127
};
    // 32-63 (0x20-0x3F)     LSB for controllers 0-31

class LabelSingleton : public AriaRenderArray, public Singleton<LabelSingleton>
{
public:
    LabelSingleton() : AriaRenderArray()
    {
    }
    
    virtual ~LabelSingleton()
    {
    }
};
    DEFINE_SINGLETON( LabelSingleton );
     
}
using namespace AriaMaestosa;

// -----------------------------------------------------------------------------------------------------------

ControllerChoice::ControllerChoice() : wxMenu(), m_controller_label(new Model<wxString>(wxT("")), true)
{
    LabelSingleton* label_renderer = LabelSingleton::getInstance();
    if (label_renderer->getStringAmount() == 0)
    {
        //I18N: - in controller, when a controller can only be on/off
        label_renderer->addString( _("On") );
        //I18N: - in controller, when a controller can only be on/off
        label_renderer->addString(_("Off"));
        //I18N: - in controller, for a controller with min/max range
        label_renderer->addString(_("Min"));
        //I18N: - in controller, for a controller with min/max range
        label_renderer->addString(_("Max"));
        //I18N: - when setting pan in controller
        label_renderer->addString(_("Left"));
        //I18N: - when setting pan in controller
        label_renderer->addString(_("Right"));

        
        label_renderer->addString(wxT("+2"));
        label_renderer->addString(wxT("-2"));
    }
    
    label_renderer->setFont( getControllerFont() );
    m_controller_label.setFont( getControllerFont() );

    m_controller_id = 7;
    m_controller_label.setMaxWidth(75, true);
    m_controller_label.getModel()->setValue(g_controller_names[m_controller_id]);
    
    Append( 7 ,  g_controller_names[7  ] ); // Volume // fine:39
    Append( 10 , g_controller_names[10 ] ); // Pan // fine:42
    Append( 1 ,  g_controller_names[1  ] ); // Modulation // fine:33
    Append( 91 , g_controller_names[91 ] ); // Reverb
    Append( 64 , g_controller_names[64 ] ); // Sustain

    Append( PSEUDO_CONTROLLER_INSTRUMENT_CHANGE, wxT("Instrument") );
    
    // In the midi specs, pitch bend is not a controller. However, i found it just made sense to place it
    // among controllers, so i assigned it arbitrary ID 200.
    Append( PSEUDO_CONTROLLER_PITCH_BEND , wxT("Pitch Bend") );
    Append( PSEUDO_CONTROLLER_TEMPO , wxT("Tempo (global)") );
    Append( PSEUDO_CONTROLLER_LYRICS , wxT("Lyrics (global)") );
    
    AppendSeparator();

    Append( 0  , g_controller_names[0  ] ); // Bank select
    Append( 2  , g_controller_names[2  ] ); // Breath
    Append( 4  , g_controller_names[4  ] ); // Foot
    Append( 8  , g_controller_names[8  ] ); // Balance
    Append( 11 , g_controller_names[11 ] ); // Expression
    Append( 92 , g_controller_names[92 ] ); // Tremolo
    Append( 93 , g_controller_names[93 ] ); // Chorus
    Append( 94 , g_controller_names[94 ] ); // Celeste
    Append( 95 , g_controller_names[95 ] ); // Phaser


    Append( 70 , g_controller_names[70 ] ); // Timber Variation
    Append( 71 , g_controller_names[71 ] ); // Timber/Harmonic
    Append( 72 , g_controller_names[72 ] ); // Release Time
    Append( 73 , g_controller_names[73 ] ); // Attack Time
    Append( 74 , g_controller_names[74 ] ); // Brightness
    Append( 75 , g_controller_names[75 ] ); // Decay Time

    AppendSeparator();

    wxMenu* misc_menu = new wxMenu();
    Append(wxID_ANY,wxT("Misc"), misc_menu);

    misc_menu->Append( 66 , g_controller_names[66 ] ); // Sostenuto
    misc_menu->Append( 67 , g_controller_names[67 ] ); // Soft Pedal
    misc_menu->Append( 68 , g_controller_names[68 ] ); // Legato footswitch
    misc_menu->Append( 69 , g_controller_names[69 ] ); // Hold

    misc_menu->AppendSeparator();
    misc_menu->Append( 65 , g_controller_names[65 ] ); // Portamento
    misc_menu->Append( 5  , g_controller_names[5  ] ); // Portamento Time // fine:37
    misc_menu->Append( 84 , g_controller_names[84 ] ); // Portamento Control
    misc_menu->AppendSeparator();
    misc_menu->Append( 76 , g_controller_names[76 ] ); // Vibrato Rate
    misc_menu->Append( 77 , g_controller_names[77 ] ); // Vibrato Depth
    misc_menu->Append( 78 , g_controller_names[78 ] ); // Vibrato Delay

    misc_menu->AppendSeparator();
    misc_menu->Append( 120 , g_controller_names[120 ] ); // All sound off
    misc_menu->Append( 121 , g_controller_names[121 ] ); // Reset control
    misc_menu->Append( 123 , g_controller_names[123 ] ); // All notes off
    /*
    misc_menu->AppendSeparator();

    misc_menu->Append( 12 , g_controller_names[12 ] ); // Effect 1
    misc_menu->Append( 13 , g_controller_names[13 ] ); // Effect 2

    misc_menu->Append( 16 , g_controller_names[16 ] ); // General Purpose 1
    misc_menu->Append( 17 , g_controller_names[17 ] ); // General Purpose 2
    misc_menu->Append( 18 , g_controller_names[18 ] ); // General Purpose 3
    misc_menu->Append( 19 , g_controller_names[19 ] ); // General Purpose 4
    misc_menu->Append( 80 , g_controller_names[80 ] ); // General Purpose 5
    misc_menu->Append( 81 , g_controller_names[81 ] ); // General Purpose 6
    misc_menu->Append( 82 , g_controller_names[82 ] ); // General Purpose 7
    misc_menu->Append( 83 , g_controller_names[83 ] ); // General Purpose 8
     */

    Connect(0,204, wxEVT_COMMAND_MENU_SELECTED,
            wxCommandEventHandler(ControllerChoice::menuSelected));
    misc_menu->Connect(0,204, wxEVT_COMMAND_MENU_SELECTED,
                       wxCommandEventHandler(ControllerChoice::menuSelected), NULL, this);
}

// -----------------------------------------------------------------------------------------------------------

ControllerChoice::~ControllerChoice()
{
}

// -----------------------------------------------------------------------------------------------------------

void ControllerChoice::menuSelected(wxCommandEvent& evt)
{
    m_controller_id = evt.GetId();
    updateLabel();
}

// -----------------------------------------------------------------------------------------------------------

void ControllerChoice::updateLabel()
{
    // special cases (non-controllers)
    if      (m_controller_id == PSEUDO_CONTROLLER_PITCH_BEND)
    {
        m_controller_label.getModel()->setValue(g_controller_names[30]);
    }
    else if (m_controller_id == PSEUDO_CONTROLLER_TEMPO)
    {
        m_controller_label.getModel()->setValue(g_controller_names[31]);
    }
    else if (m_controller_id == PSEUDO_CONTROLLER_LYRICS)
    {
        m_controller_label.getModel()->setValue(g_controller_names[32]);
    }
    else if (m_controller_id == PSEUDO_CONTROLLER_INSTRUMENT_CHANGE)
    {
        m_controller_label.getModel()->setValue(g_controller_names[33]);
    }
    else
    {
        m_controller_label.getModel()->setValue(g_controller_names[m_controller_id]);
    }
    
    ASSERT_E(m_controller_id,<,204);
    ASSERT_E(m_controller_id,>=,0);

    Display::render();
    getMainFrame()->getMainPane()->SetFocus();
}

// -----------------------------------------------------------------------------------------------------------

// FIXME: it's dubious that rendering belongs here
void ControllerChoice::renderControllerName(const int x, const int y)
{
    m_controller_label.bind();
    m_controller_label.render(x, y);
}

// -----------------------------------------------------------------------------------------------------------

/** Returns whether a specific controller is of on/off type, rather than taking values ranging from min to max */
bool ControllerChoice::isOnOffController(const int id) const
{
    if (m_controller_id == 66 or m_controller_id == 67 or
        m_controller_id == 68 or m_controller_id == 69 or
        m_controller_id == 64 or m_controller_id == 65 )
    {
        return true;
    }

    return false;
}

// -----------------------------------------------------------------------------------------------------------

void ControllerChoice::setControllerID(int id)
{
    m_controller_id = id;
    updateLabel();
}

// -----------------------------------------------------------------------------------------------------------

/**
 * Which label should appear at the top of controller editor for current Controller?
 */
void ControllerChoice::renderTopLabel(const int x, const int y)
{
    LabelSingleton* label_renderer = LabelSingleton::getInstance();
    label_renderer->bind();

    // pan
    if (m_controller_id == 10 or m_controller_id == 42)       label_renderer->get(5).render(x, y);

    // pitch bend
    else if (m_controller_id == PSEUDO_CONTROLLER_PITCH_BEND) label_renderer->get(6).render(x, y);

    // on/offs
    else if ( isOnOffController(m_controller_id) )            label_renderer->get(0).render(x, y);

    // tempo
    else if (m_controller_id == PSEUDO_CONTROLLER_TEMPO)      AriaRender::renderNumber("400", x, y);
    
    // lyrics
    else if (m_controller_id == PSEUDO_CONTROLLER_LYRICS)     ;

    // bank select
    else if (m_controller_id == 0)     ;

    // instrument
    else if (m_controller_id == PSEUDO_CONTROLLER_INSTRUMENT_CHANGE);
    
    else                                                      label_renderer->get(3).render(x, y);
}

// -----------------------------------------------------------------------------------------------------------

/**
 * Which label should appear at the bottom of controller editor for current Controller?
 */
void ControllerChoice::renderBottomLabel(const int x, const int y)
{
    LabelSingleton* label_renderer = LabelSingleton::getInstance();
    label_renderer->bind();
    
    // pan
    if (m_controller_id == 10 or m_controller_id == 42)       label_renderer->get(4).render(x, y);

    // pitch bend
    else if (m_controller_id == PSEUDO_CONTROLLER_PITCH_BEND) label_renderer->get(7).render(x, y);

    // on/offs
    else if (isOnOffController(m_controller_id))              label_renderer->get(1).render(x, y);

    // tempo
    else if (m_controller_id == PSEUDO_CONTROLLER_TEMPO)      AriaRender::renderNumber( "20", x, y );
    
    // lyrics
    else if (m_controller_id == PSEUDO_CONTROLLER_LYRICS)     ;
    
    // bank select
    else if (m_controller_id == 0)     ;
    
    // instrument
    else if (m_controller_id == PSEUDO_CONTROLLER_INSTRUMENT_CHANGE);
    
    else                                                      label_renderer->get(2).render(x, y);
}


