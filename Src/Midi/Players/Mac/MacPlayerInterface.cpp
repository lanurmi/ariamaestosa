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


#ifdef _MAC_QUICKTIME_COREAUDIO


#include <iostream>

#include <wx/string.h>

#include "Utils.h"
#include "AriaCore.h"
#include "Dialogs/WaitWindow.h"
#include "GUI/MainFrame.h"
#include "IO/MidiToMemoryStream.h"
#include "IO/IOUtils.h"
#include "Midi/CommonMidiUtils.h"
#include "Midi/MeasureData.h"
#include "Midi/Players/Mac/AudioUnitOutput.h"
#include "Midi/Players/Mac/CoreMIDIOutput.h"
#include "Midi/Players/Mac/OutputBase.h"
#include "Midi/Players/Mac/QuickTimeExport.h"
#include "Midi/Players/PlatformMidiManager.h"
#include "Midi/Players/Sequencer.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"
#include "PreferencesData.h"

#include "jdksmidi/world.h"
#include "jdksmidi/track.h"
#include "jdksmidi/multitrack.h"
#include "jdksmidi/filereadmultitrack.h"
#include "jdksmidi/fileread.h"
#include "jdksmidi/fileshow.h"
#include "jdksmidi/filewritemultitrack.h"
#include "jdksmidi/msg.h"
#include "jdksmidi/sysex.h"
#include "jdksmidi/sequencer.h"
#include "jdksmidi/driver.h"
#include "jdksmidi/process.h"

#include <wx/msgdlg.h>


namespace AriaMaestosa
{
    bool g_playing;
    
    int g_current_tick;
    int g_current_accurate_tick;
    
    bool g_thread_should_continue = true;
    
    OutputBase* output = NULL;
    
    enum AudioExportBackend
    {
        AUDIO_EXPORT_BACKEND_QUICKTIME,
        AUDIO_EXPORT_BACKEND_AUDIOUNIT
    };
    
    /**
      * @ingroup midi.players
      *
      * Helper thread class for AIFF export on OSX
      */
    class AudioExport : public wxThread
    {
        wxString m_filepath;
        Sequence* m_sequence;
        char* m_data;
        int m_length;
        int m_firstMeasureValue;
        AudioExportBackend m_backend;
        
    public:
        
        AudioExport(Sequence* sequence, wxString filepath, AudioExportBackend backend)
        {
            m_backend = backend;
            m_filepath = filepath;
            m_sequence = sequence;
            
            if (Create() != wxTHREAD_NO_ERROR)
            {
                wxMessageBox(wxT("Failed to create Audio Export thread!"));
                return;
            }
            
            MeasureData* md = m_sequence->getMeasureData();

            // when we're saving, we always want song to start at first measure, so temporarly set
            // firstMeasure to 0, and set it back in the end
            m_firstMeasureValue = md->getFirstMeasure();
            md->setFirstMeasure(0);
            
            int startTick = -1, songLength = -1;
            allocAsMidiBytes(m_sequence, false, &songLength, &startTick, &m_data, &m_length, true);
            
            if (backend == AUDIO_EXPORT_BACKEND_QUICKTIME)
            {
                if (not QuickTimeExport::qtkit_setData(m_data, m_length))
                {
                    wxMessageBox( _("Sorry, an internal error occurred during export") );
                    
                    // send hide progress window event
                    MAKE_HIDE_PROGRESSBAR_EVENT(event);
                    getMainFrame()->GetEventHandler()->AddPendingEvent(event);
                    
                    return;
                }
            }
            Run();
        }
        
        virtual ExitCode Entry()
        {
            printf("Export thread started...\n");
            MeasureData* md = m_sequence->getMeasureData();
            
            wxArrayString errorMessages;
            
            bool success = false;
            if (m_backend == AUDIO_EXPORT_BACKEND_QUICKTIME)
            {
                success = QuickTimeExport::qtkit_exportToAiff( m_filepath.mb_str() );
            }
            else if (m_backend == AUDIO_EXPORT_BACKEND_AUDIOUNIT)
            {
                success = ((AudioUnitOutput*)output)->outputToDisk(m_filepath.mb_str(),
                                                                    m_data, m_length,
                                                                    errorMessages);
            }

            // send hide progress window event
            MAKE_HIDE_PROGRESSBAR_EVENT(event);
            getMainFrame()->GetEventHandler()->AddPendingEvent(event);
            
            
            // send hide progress window event
            if (not success)
            {
                wxString msg = _("Sorry, exporting to audio could not complete successfully. Please include the following error code when reporting the issue :");
                
                if (errorMessages.size() == 0)
                {
                    msg << wxT("\n(unknown error)");
                }
                
                for (unsigned int n=0; n<errorMessages.size(); n++)
                {
                    msg << wxT("\n");
                    msg << errorMessages[n];
                }
                MAKE_ASYNC_ERR_MESSAGE_EVENT(errorNotification);
                errorNotification.SetString(msg);
                getMainFrame()->GetEventHandler()->AddPendingEvent(errorNotification);
            }
            
            
            
            free(m_data);
            md->setFirstMeasure(m_firstMeasureValue);
            
            return 0;
        }
    };
    
    void cleanup_after_playback()
    {
        g_playing = false;
        output->reset_all_controllers();
    }
    
    /**
      * @ingroup midi.players
      *
      * Helper thread class for playback on OSX
      */
    class SequencerThread : public wxThread
    {
        jdksmidi::MIDIMultiTrack* jdkmidiseq;
        jdksmidi::MIDISequencer* jdksequencer;
        int songLengthInTicks;
        bool m_selection_only;
        int m_start_tick;
        Sequence* m_sequence;
                
    public:
        
        SequencerThread(Sequence* seq, const bool selectionOnly)
        {
            m_sequence = seq;
            jdkmidiseq = NULL;
            jdksequencer = NULL;
            m_selection_only = selectionOnly;
        }
        ~SequencerThread()
        {
            if (jdksequencer != NULL) delete jdksequencer;
            if (jdkmidiseq != NULL) delete jdkmidiseq;
        }
        
        void prepareSequencer()
        {
            jdkmidiseq = new jdksmidi::MIDIMultiTrack();
            songLengthInTicks = -1;
            int trackAmount = -1;
            m_start_tick = 0;
            makeJDKMidiSequence(m_sequence, *jdkmidiseq, m_selection_only, &songLengthInTicks,
                                &m_start_tick, &trackAmount, true /* for playback */);
            
            //std::cout << "trackAmount=" << trackAmount << " start_tick=" << m_start_tick<<
            //        " songLengthInTicks=" << songLengthInTicks << std::endl;
            
            jdksequencer = new jdksmidi::MIDISequencer(jdkmidiseq);
            
            g_current_tick = m_start_tick;
            g_current_accurate_tick = m_start_tick;
        }
        
        void go(int* startTick /* out */)
        {
            g_playing = true;
            g_thread_should_continue = true;
            
            if (Create() != wxTHREAD_NO_ERROR)
            {
                std::cerr << "[OSX Player] ERROR: failed to create thread" << std::endl;
                return;
            }
            SetPriority(85 /* 0 = min, 100 = max */);
            
            prepareSequencer();
            *startTick = m_start_tick;
            
            Run();
        }
        
        ExitCode Entry()
        {
            AriaSequenceTimer timer(m_sequence);
            timer.run(jdksequencer, songLengthInTicks);
            
            //must_stop = true;
            cleanup_after_playback();
            
            return 0;
        }
    };
    
    void playMidiBytes(char* bytes, int length);

    /**
      * @ingroup midi.players
      *
      * Main interface for playback on OSX
      */
    class MacMidiManager : public PlatformMidiManager
    {
        int stored_songLength;
        
        Sequence* m_sequence;
        
    public:
        
        MacMidiManager()
        {
            g_playing   = false;
            stored_songLength = 0;
        }
        
        virtual ~MacMidiManager() { }
        
        
        virtual void initMidiPlayer()
        {
            wxString driver = PreferencesData::getInstance()->getValue(SETTING_ID_MIDI_OUTPUT);
            if (driver == DEFAULT_PORT || driver == _("OSX Software Synthesizer"))
            {
                if (driver == DEFAULT_PORT)
                {
                    PreferencesData::getInstance()->setValue(SETTING_ID_MIDI_OUTPUT, _("OSX Software Synthesizer"));
                }
                
                wxString soundfont = PreferencesData::getInstance()->getValue(SETTING_ID_SOUNDBANK);
                output = new AudioUnitOutput(soundfont == SYSTEM_BANK ?
                                             NULL : (const char*)soundfont.utf8_str());
            }
            else
            {
                output = new CoreMidiOutput(driver);
            }
        }
        
        virtual void freeMidiPlayer()
        {
            delete output;
            output = NULL;
        }
        
        
        virtual void exportAudioFile(Sequence* sequence, wxString filepath)
        {
            new AudioExport(sequence, filepath, AUDIO_EXPORT_BACKEND_QUICKTIME);
        }
        
        virtual const wxString getAudioExtension()
        {
            return wxT(".aiff");
        }
        
        virtual const wxString getAudioWildcard()
        {
            return  wxString( _("AIFF file")) + wxT("|*.aiff");
        }
        
        virtual int getAccurateTick()
        {
            return g_current_accurate_tick;
        }
        
        virtual int getCurrentTick()
        {
            return g_current_tick;
        }
        
        virtual wxArrayString getOutputChoices()
        {
            const std::vector<CoreMidiOutput::Destination>& destinations = CoreMidiOutput::getDestinations();
            wxArrayString out;
            out.Add(_("OSX Software Synthesizer"));
            for (unsigned int n=0; n<destinations.size(); n++)
            {
                out.Add(wxString(destinations[n].m_name.c_str(), wxConvUTF8));
            }
            return out;
        }
        
        virtual bool isPlaying()
        {
            return g_playing;
        }        
        
        
        virtual void playNote(int noteNum, int volume, int duration, int channel, int instrument)
        {
            if (g_playing) return;
            output->playNote( noteNum, volume, duration, channel, instrument );
        }
        
        virtual bool playSelected(Sequence* sequence, /*out*/ int* startTick)
        {
            if (g_playing) return false; //already playing
            output->stopNote();
            
            m_sequence = sequence;
            
            SequencerThread* seqthread = new SequencerThread(sequence, true /* selection only */);
            seqthread->go(startTick);
            
            m_start_tick = *startTick;
            
            return true;
        }
        
        virtual bool playSequence(Sequence* sequence, /*out*/ int* startTick)
        {
            if (g_playing) return false; //already playing
            output->stopNote();
            
            m_sequence = sequence;
            
            SequencerThread* seqthread = new SequencerThread(sequence, false /* selection only */);
            seqthread->go(startTick);
            
            m_start_tick = *startTick;
            return true;
        }

        void seq_controlchange(const int controller, const int value, const int channel)
        {
            output->controlchange(controller, value, channel);
        }
        
        /**
         * @brief will be called by the generic sequencer to determine whether it should continue
         * @return false to stop it, true to continue
         */
        bool seq_must_continue()
        {
            return g_thread_should_continue;
        }
        
        void seq_note_on(const int note, const int volume, const int channel)
        {
            output->note_on(note, volume, channel);
        }
        
        void seq_note_off(const int note, const int channel)
        {
            output->note_off(note, channel);
        }
        
        /**
         * @brief called repeatedly by the generic sequencer to tell the midi player what is the current
         *        progression. the sequencer will call this with -1 as argument to indicate it exits.
         */
        virtual void seq_notify_current_tick(const int tick)
        {
            g_current_tick = tick;
            
            if (tick == -1) g_playing = false;
        }
        
        virtual void seq_notify_accurate_current_tick(const int tick)
        {
            g_current_accurate_tick = tick;
        }
        
        void seq_pitch_bend(const int value, const int channel)
        {
            output->pitch_bend(value + 8192, channel);
        }
        
        void seq_prog_change(const int instrument, const int channel)
        {
            output->prog_change(instrument, channel);
        }
        
        virtual void stopNote()
        {
            output->stopNote();
        }
        
        virtual void stop()
        {
            g_thread_should_continue = false;
        }
        
        virtual int trackPlaybackProgression()
        {
            return g_current_tick;
        }
        
    }; // end class
    
    class MacMidiManagerFactory : public PlatformMidiManagerFactory
    {
    public:
        MacMidiManagerFactory() : PlatformMidiManagerFactory(wxT("QuickTime/AudioToolkit"))
        {
        }
        virtual ~MacMidiManagerFactory()
        {
        }
        
        virtual PlatformMidiManager* newInstance()
        {
            return new MacMidiManager();
        }
    };
    MacMidiManagerFactory g_mac_midi_manager_factory;
    
} // end namespace

#endif
