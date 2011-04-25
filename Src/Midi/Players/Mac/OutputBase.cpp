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

#include <wx/timer.h>
#include "Midi/Players/Mac/OutputBase.h"
#include "Midi/Players/Mac/AUNotePlayer.h"

class StopNoteTimer : public wxTimer
{
    
    OutputBase* m_o;
public:
    
    StopNoteTimer(OutputBase* o) : wxTimer()
    {
        m_o = o;
    }
    
    void Notify()
    {
        m_o->stopNote();
        wxTimer::Stop();
    }
    
    void start(int duration)
    {
        Start(duration);
    }
};

bool playing = false;

StopNoteTimer* stopNoteTimer = NULL;


OutputBase::OutputBase()
{
    MIDIClientCreate(CFSTR("MidiOutput"), NULL, NULL, &m_client);
    
    stopNoteTimer = new StopNoteTimer(this);

}

OutputBase::~OutputBase()
{
    if (stopNoteTimer != NULL)
    {
        delete stopNoteTimer;
        stopNoteTimer = NULL;
    }
}

int lastNote;
int lastChannel;

void OutputBase::playNote(int pitchID, int volume, int duration, int channel, int instrument)
{
    if (playing) stopNote();
    
    lastNote = pitchID;
    lastChannel = channel;
    
    note_on(lastNote, volume, lastChannel);
    stopNoteTimer->start(duration);
    playing = true;

    return;
}

// ------------------------------------------------------------------------------------------------------

void OutputBase::stopNote()
{
    if (not playing) return;

    note_off(lastNote, lastChannel);
    playing = false;

    return;
}
