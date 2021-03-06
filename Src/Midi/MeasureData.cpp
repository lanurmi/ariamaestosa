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

#include "Utils.h"

#include "Midi/MeasureData.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"
#include "Midi/TimeSigChange.h"

#include <iostream>
#include "irrXML/irrXML.h"

using namespace AriaMaestosa;

// ----------------------------------------------------------------------------------------------------------

MeasureData::MeasureData(Sequence* seq, int measureAmount)
{
    m_something_selected = false;
    m_selected_time_sig  = 0;
    m_measure_amount     = measureAmount;
    m_first_measure      = 0;
    m_loop_end_measure   = 0;
    m_expanded_mode      = false;
    m_sequence           = seq;
    
    m_time_sig_changes.push_back( new TimeSigChange(0,0,4,4) );
    
    updateVector(measureAmount);
}

// ----------------------------------------------------------------------------------------------------------

MeasureData::~MeasureData()
{
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#endif

void MeasureData::setExpandedMode(bool arg_expanded)
{
    //when turning it off, ask for a confirmation because all events will be lost
    if (this->m_expanded_mode and not arg_expanded)
    {
        // remove all added events
        m_selected_time_sig = 0;
        m_time_sig_changes.clearAndDeleteAll();
        m_time_sig_changes.push_back( new TimeSigChange(0, 0, 4, 4) );
    }

    m_expanded_mode = arg_expanded;
}


// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Measures
#endif

void MeasureData::setMeasureAmount(int measureAmount)
{
    m_measure_amount = measureAmount;
    updateVector(measureAmount);
    
    ASSERT_E(m_measure_amount, ==, (int)m_measure_info.size());
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::setFirstMeasure(int firstMeasureID)
{
    m_first_measure = firstMeasureID;
}

// ----------------------------------------------------------------------------------------------------------

int MeasureData::measureLengthInTicks(int measure) const
{
    if (measure == -1) measure = 0; // no parameter passed, use measure 0 settings

    const int num   = getTimeSigNumerator(measure);
    const int denom = getTimeSigDenominator(measure);
    return getMeasureLengthInTicks(num, denom);
}

// ----------------------------------------------------------------------------------------------------------

// right now, it's just the first measure that is considered "default". This may need to be reviewed
// (i'm not sure if this is used at all or very much)
int MeasureData::defaultMeasureLengthInTicks()
{
    const int num   = getTimeSigNumerator(0);
    const int denom = getTimeSigDenominator(0);
    return getMeasureLengthInTicks(num, denom);
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::selectOnly(const int measureID)
{
    const int measure_amount = m_measure_info.size();
    for (int n=0; n<measure_amount; n++)
    {
        m_measure_info[n].selected = false;
    }
    
    m_measure_info[measureID].selected = true;
    m_something_selected = true;
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::selectNothing()
{
    m_something_selected = false;
    
    const int measureAmount = m_measure_info.size();
    for (int n=0; n<measureAmount; n++)
    {
        m_measure_info[n].selected = false;
    }
    
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::selectNotesInSelectedMeasures()
{
    
    // iterate through notes and select those that are within the selection range just found
    const int trackAmount = m_sequence->getTrackAmount();
    for (int trackID=0; trackID<trackAmount; trackID++)
    {
        Track* track = m_sequence->getTrack(trackID);
        
        const int noteAmount = track->getNoteAmount();
        for (int n=0; n<noteAmount; n++)
        {
            const int note_tick = track->getNoteStartInMidiTicks(n);
            
            // note is within selection range? if so, select it, else unselect it.
            track->selectNote(n, m_measure_info[measureAtTick(note_tick)].selected , true);
        }
    }
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::checkUpToDate()
{
    // if measure amount changed and MeasureBar is out of sync with its current number of measures, fix it
    if ((int)m_measure_info.size() != m_measure_amount)
    {
        updateVector(m_measure_amount);
    }
}

// ----------------------------------------------------------------------------------------------------------

int MeasureData::getTimeSigChangeCount() const
{
    return m_time_sig_changes.size();
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::selectMeasure(int mid)
{
    m_measure_info[mid].selected = true;
    m_something_selected = true;
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::extendToTick(const int tick)
{
    const int last_measure_tick = lastTickInMeasure( getMeasureAmount() - 1 );
    if (tick <= last_measure_tick) return; // nothing to do
    
    ScopedMeasureTransaction tr(startTransaction());
    
    const int last_measure_length = getMeasureLength( getMeasureAmount() - 1 );
    
    float add_measures = float(tick - last_measure_tick) / float(last_measure_length);
    if (add_measures > 0.0f)
    {
        tr->setMeasureAmount(getMeasureAmount() + ceil(add_measures));
    }
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Find Measure From Location
#endif

int MeasureData::measureAtTick(int tick) const
{
    if (isMeasureLengthConstant())
    {
        const float step = measureLengthInTicks();

        const int answer = (int)( tick / step );
        
        if (not m_sequence->isImportMode())
        {
            // verify that we're within song bounds. except if importing, since the song length
            // might not have been set yet.
            ASSERT_E(answer, <=, m_measure_amount + 10); // allow some bounds at end to let ring, etc.
        }
        
        return answer;

    }
    else
    {
        if (tick < 0) tick = 0;
        if (not m_sequence->isImportMode())
        {
            // verify that we're within song bounds. except if importing, since the song length
            // might not have been set yet.
            const int lastTick = lastTickInMeasure(m_measure_amount-1);
            if (tick > lastTick)
            {
                fprintf(stderr, "[MeasureData] WARNING: tick %i is beyond song end %i!\n", tick, lastTick);
            }
        }
        
        // iterate through measures till we find the one at the given tick
        const int amount = m_measure_info.size();
        for (int n=0; n<amount-1; n++)
        {
            if ( m_measure_info[n].tick <= tick and m_measure_info[n+1].tick > tick ) return n;
        }

        // did not find this tick in our current measure set
        if (m_sequence->isImportMode())
        {
            // if we're currently importing, extrapolate beyond the current song end since we
            // might be trying to determine needed song length (FIXME: this should not read the
            // importing bool from the sequence, whether to extrapolate should be a parameter)
            const int last_id = m_time_sig_changes.size()-1;
            tick -= m_time_sig_changes[ last_id ].getTickCache();
            
            const int answer =  (int)(
                                      m_time_sig_changes[ last_id ].getMeasure() +
                                      tick / getMeasureLengthInTicks(
                                            m_time_sig_changes[ last_id ].getNum(),
                                            m_time_sig_changes[ last_id ].getDenom())
                                      );
            return answer;
        }
        else
        {
            // didnt find any... current song length is not long enough
            return m_measure_amount-1;
        }
    }

}

// ----------------------------------------------------------------------------------------------------------

int MeasureData::firstTickInMeasure(int id) const
{
    ASSERT_E(m_measure_amount, ==, (int)m_measure_info.size());
    
    if (isMeasureLengthConstant())
    {
        if (id >= m_measure_amount)
        {
            // allow going a little past 'official' song end to account for rounding errors, etc. (FIXME?)
            ASSERT_E(id, <, m_measure_amount + 50); // but not too far, to detect corrupt values
            return m_measure_amount * measureLengthInTicks();
        }
        return id * measureLengthInTicks();
    }
    else
    {
        // allow going a little past 'official' song end to account for rounding errors, etc. (FIXME?)
        if (id >= (int)m_measure_info.size())
        {
            if (id > (int)m_measure_info.size()+500){ ASSERT(false); } // but not too far, to detect corrupt values
            
            return m_measure_info[m_measure_info.size()-1].endTick;
        }
        
        
        return m_measure_info[id].tick;
    }
}

// ----------------------------------------------------------------------------------------------------------

int MeasureData::lastTickInMeasure(int id) const
{
    ASSERT_E(m_measure_amount, ==, (int)m_measure_info.size());
    
    if (isMeasureLengthConstant())
    {
        return (id+1) * measureLengthInTicks();
    }
    else
    {
        // allow going a little past 'official' song end
        if (id >= (int)m_measure_info.size())
        {
            if (id > (int)m_measure_info.size()+50){ ASSERT(false); } // but not too far, to detect corrupt values
            
            return m_measure_info[m_measure_info.size()-1].endTick;
        }
        
        ASSERT_E(id,<,(int)m_measure_info.size());
        return m_measure_info[id].endTick;
    }
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Time Signature Management
#endif

int MeasureData::getTimeSigNumerator(int measure) const
{
    if (measure != -1)
    {
        measure += 1;
        const int timeSigChangeAmount = m_time_sig_changes.size();
        for (int n=0; n<timeSigChangeAmount; n++)
        {
            if (m_time_sig_changes[n].getMeasure() >= measure) return m_time_sig_changes[n-1].getNum();
        }
        return m_time_sig_changes[timeSigChangeAmount-1].getNum();
    }
    else return m_time_sig_changes[m_selected_time_sig].getNum();
}

// ----------------------------------------------------------------------------------------------------------

int MeasureData::getTimeSigDenominator(int measure) const
{

    if (measure != -1)
    {
        measure+=1;

        const int timeSigChangeAmount = m_time_sig_changes.size();
        for (int n=0; n<timeSigChangeAmount; n++)
        {
            if (m_time_sig_changes[n].getMeasure() >= measure) return m_time_sig_changes[n-1].getDenom();
        }
        return m_time_sig_changes[timeSigChangeAmount-1].getDenom();
    }
    else
    {
        return m_time_sig_changes[m_selected_time_sig].getDenom();
    }
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::setTimeSig(int top, int bottom)
{
    ASSERT_E(m_measure_amount, ==, (int)m_measure_info.size());
    
    m_time_sig_changes[m_selected_time_sig].setNum( top );
    m_time_sig_changes[m_selected_time_sig].setDenom( bottom );
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::eraseTimeSig(int id)
{
    m_time_sig_changes.erase( id );
    if (m_selected_time_sig == id)
    {
        m_selected_time_sig = 0;
    }
}

// ----------------------------------------------------------------------------------------------------------

int MeasureData::addTimeSigChange(int measure, int num, int denom) // -1 means "same as previous event"
{
    const int timeSig_amount_minus_one = m_time_sig_changes.size()-1;

    // if there are no events, just add it. otherwise, add in time order.
    if (m_time_sig_changes.size() == 0)
    {
        m_time_sig_changes.push_back( new TimeSigChange(measure, -1, num, denom) );
        return m_time_sig_changes.size() - 1;
    }
    else
    {
        // iterate through time sig events, until an iteration does something with the event
        for (int n=0; n<(int)m_time_sig_changes.size(); n++) 
        {
            if (m_time_sig_changes[n].getMeasure() == measure)
            {
                // a time sig event already exists at this location
                // if we're not importing, select it
                if (not m_sequence->isImportMode())
                {
                    m_selected_time_sig = n;
                    return n;
                }
                // if we're importing, replace it with new value
                else
                {
                    m_time_sig_changes[m_selected_time_sig].setNum(num);
                    m_time_sig_changes[m_selected_time_sig].setDenom(denom);
                    return m_selected_time_sig;
                }
            }
            // we checked enough events so that we are past the point where the click occured.
            // we know there is no event already existing at the clicked measure.
            if (m_time_sig_changes[n].getMeasure() > measure)
            {
                if (num == -1 or denom == -1)
                {
                    m_time_sig_changes.add( new TimeSigChange(measure, -1, m_time_sig_changes[n].getNum(),
                                                              m_time_sig_changes[n].getDenom()), n );
                }
                else
                {
                    m_time_sig_changes.add( new TimeSigChange(measure, -1, num, denom), n );
                }

                m_selected_time_sig = n;
                return n;
            }
            else if (n == timeSig_amount_minus_one)
            {
                // Append a new event at the end
                if (num == -1 or denom == -1)
                {
                    m_time_sig_changes.add( new TimeSigChange(measure, -1, m_time_sig_changes[n].getNum(),
                                                              m_time_sig_changes[n].getDenom()), n+1 );
                }
                else
                {
                    m_time_sig_changes.add( new TimeSigChange(measure, -1, num, denom), n + 1 );
                }
                
                return m_time_sig_changes.size() - 1;
            }
        }//next

    }
    ASSERT(false);
    return -1;
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::setTimesigMeasure(const int id, const int newMeasure)
{
    ASSERT_E(id,>=,0);
    ASSERT_E(id,<,m_time_sig_changes.size());
    m_time_sig_changes[id].setMeasure(newMeasure);
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Refresh Info
#endif

void MeasureData::updateVector(int newSize)
{
    while ((int)m_measure_info.size() < newSize) m_measure_info.push_back( MeasureInfo() );
    while ((int)m_measure_info.size() > newSize) m_measure_info.erase( m_measure_info.begin()+m_measure_info.size()-1 );

    ASSERT_E(m_measure_amount, ==, (int)m_measure_info.size());
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::updateMeasureInfo()
{
    const int amount = m_measure_info.size();
    //const float zoom = sequence->getZoom();
    
    const int ticksPerQuarterNote = m_sequence->ticksPerQuarterNote();
    float tick = 0;
    int timg_sig_event = 0;

    //ASSERT_E(timg_sig_event,<,m_time_sig_changes.size());
    //m_time_sig_changes[timg_sig_event].setTickCache(0);
    //m_time_sig_changes[timg_sig_event].pixel = 0;

    for (int n=0; n<amount; n++)
    {
        // check if time sig changes on this measure
        if (timg_sig_event != (int)m_time_sig_changes.size()-1 and
            m_time_sig_changes[timg_sig_event+1].getMeasure() == n)
        {
            timg_sig_event++;
            //m_time_sig_changes[timg_sig_event].setTickCache((int)round( tick ));
            //m_time_sig_changes[timg_sig_event].pixel = (int)round( tick * zoom );
        }

        // set end location of previous measure
        if (n > 0)
        {
            m_measure_info[n-1].endTick = (int)round( tick );
            //m_measure_info[n-1].endPixel = (int)round( tick * zoom );
            m_measure_info[n-1].widthInTicks = m_measure_info[n-1].endTick - m_measure_info[n-1].tick;
            //m_measure_info[n-1].widthInPixels = (int)( m_measure_info[n-1].widthInTicks * zoom );
        }

        // set the location of measure in both ticks and pixels so that it can be used later in
        // calculations and drawing
        m_measure_info[n].tick = (int)round( tick );
        //m_measure_info[n].pixel = (int)round( tick * zoom );
        tick += getMeasureLengthInTicks(m_time_sig_changes[timg_sig_event].getNum(),
                                        m_time_sig_changes[timg_sig_event].getDenom());
    }

    // fill length and end of last measure
    m_measure_info[amount-1].endTick = (int)tick;
    //m_measure_info[amount-1].endPixel = (int)( tick * zoom );
    m_measure_info[amount-1].widthInTicks = m_measure_info[amount-1].endTick - m_measure_info[amount-1].tick;
    //m_measure_info[amount-1].widthInPixels = (int)( m_measure_info[amount-1].widthInTicks * zoom );

    totalNeededLengthInTicks = (int)tick;
    //totalNeededLengthInPixels = (int)( tick * zoom );

    
    const int sequenceLength = m_sequence->getLastTickInSequence();
    const int lastTickMeasure = getTotalTickAmount();
    if (sequenceLength > lastTickMeasure)
    {
        const int missingTicks = (sequenceLength - lastTickMeasure);
        const int missingMeasures = ceil( float(missingTicks) / float(getMeasureLength(getMeasureAmount() - 1)) );
        setMeasureAmount( m_measure_amount + missingMeasures );
        updateMeasureInfo();
        return;
    }
    
    for (unsigned int n=0; n<m_listeners.size(); n++)
    {
        m_listeners[n]->onMeasureDataChange(0);
    }
    
    for (int n = 0; n < m_time_sig_changes.size(); n++)
    {
        int measure = m_time_sig_changes[n].getMeasure();
        m_time_sig_changes[n].setTickCache(firstTickInMeasure(measure));
    }
    
    ASSERT_E(m_measure_amount, ==, (int)m_measure_info.size());
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark I/O
#endif

//FIXME: remove globals
// when we import time sig changes, their time is given in ticks. however Aria needs them in measure ID.
// this variable is used to convert ticks to measures (we can't yet use measureAt method and friends because
// measure information is still incomplete at this point ).
int last_event_tick;
int measuresPassed;

// ----------------------------------------------------------------------------------------------------------

void MeasureData::beforeImporting()
{
    m_time_sig_changes.clearAndDeleteAll();
    m_time_sig_changes.push_back(new TimeSigChange(0,0,4,4));

    last_event_tick = 0;
    measuresPassed = 0;
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::addTimeSigChange_import(int tick, int num, int denom)
{
    int measure;

    if (tick == 0)
    {
        measure = 0;

        if (m_time_sig_changes.size() == 0)
        {
            m_time_sig_changes.push_back( new TimeSigChange(0, 0, 4, 4) );
        }

        // when an event already exists at the beginning, don't add another one, just modify it
        if (m_time_sig_changes[0].getTickCache() == 0)
        {
            m_time_sig_changes[0].setNum(num);
            m_time_sig_changes[0].setDenom(denom);
            m_time_sig_changes[0].setTickCache(0);
            m_time_sig_changes[0].setMeasure(0);
            last_event_tick = 0;
            return;
        }
        else
        {
            std::cerr << "Unexpected!! tick is " << m_time_sig_changes[0].getTickCache() << std::endl;
        }
    }
    else
    {
        const int last_id = m_time_sig_changes.size() - 1;
        measure = (int)(
                        measuresPassed + (tick - last_event_tick)  /
                        getMeasureLengthInTicks(m_time_sig_changes[last_id].getNum(),
                                        m_time_sig_changes[last_id].getDenom())
                        );
    }

    m_time_sig_changes.push_back( new TimeSigChange(measure, tick, num, denom) );

    measuresPassed = measure;

    last_event_tick = tick;
}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::afterImporting()
{
    if (m_time_sig_changes.size() > 1)
    {
        m_expanded_mode = true;
    }
    else
    {
        m_expanded_mode = false;
    }
    if (not isMeasureLengthConstant()) updateMeasureInfo();
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#endif

int MeasureData::getTotalTickAmount() const
{
    if (isMeasureLengthConstant())
    {
        return (int)( m_measure_amount * measureLengthInTicks() );
    }
    else
    {
        return totalNeededLengthInTicks;
    }
}

// ----------------------------------------------------------------------------------------------------------
// ----------------------------------------------------------------------------------------------------------
#if 0
#pragma mark -
#pragma mark Serialisation
#endif

bool MeasureData::readFromFile(irr::io::IrrXMLReader* xml)
{

    // ---------- measure ------
    if (strcmp("measure", xml->getNodeName()) == 0)
    {

        const char* firstMeasure_c = xml->getAttributeValue("firstMeasure");
        if ( firstMeasure_c != NULL )
        {
            setFirstMeasure( atoi( firstMeasure_c ) );
        }
        else
        {
            std::cerr << "Missing info from file: first measure" << std::endl;
            setFirstMeasure( 0 );
        }

        if (getFirstMeasure() < 0)
        {
            std::cerr << "Wrong first measure: " << getFirstMeasure() << std::endl;
            setFirstMeasure( 0 );
        }

        const char* denom = xml->getAttributeValue("denom");
        if (denom != NULL)
        {
            // add an event if one is not already there
            if (m_time_sig_changes.size() == 0)
            {
                m_time_sig_changes.push_back(new TimeSigChange(0,0,4,4) );
            }
            m_time_sig_changes[0].setDenom( atoi( denom ) );

            if (m_time_sig_changes[0].getDenom() < 0 or m_time_sig_changes[0].getDenom() > 64)
            {
                std::cerr << "Wrong measureBar->getTimeSigDenominator(): "
                          << m_time_sig_changes[0].getDenom() << std::endl;
                
                m_time_sig_changes[0].setDenom(4);
            }
        }

        const char* num = xml->getAttributeValue("num");
        if (num != NULL)
        {
            if (m_time_sig_changes.size() == 0)
            {
                // add an event if one is not already there
                m_time_sig_changes.push_back(new TimeSigChange(0,0,4,4));
            }
            m_time_sig_changes[0].setNum( atoi( num ) );

            if (m_time_sig_changes[0].getNum() < 0 or m_time_sig_changes[0].getNum() > 64)
            {
                std::cerr << "Wrong m_time_sig_changes[0].getNum(): " << m_time_sig_changes[0].getNum() << std::endl;
                m_time_sig_changes[0].setNum( 4 );
            }
        }


    }
    else if (strcmp("timesig", xml->getNodeName()) == 0)
    {
        std::cout << "importing event" << std::endl;
        int num=-1, denom=-1, meas=-1;

        const char* num_c = xml->getAttributeValue("num");
        if (num_c != NULL)
        {
            num = atoi( num_c );
        }
        else
        {
            std::cerr << "Missing info from file: measure numerator" << std::endl;
            return true; // dont halt program but just ignore event
        }
        const char* denom_c = xml->getAttributeValue("denom");
        if (denom_c != NULL)
        {
            denom = atoi( denom_c );
        }
        else
        {
            std::cerr << "Missing info from file: measure denominator" << std::endl;
            return true;  // dont halt program but just ignore event
        }
        const char* meas_c = xml->getAttributeValue("measure");
        if (meas_c != NULL)
        {
            meas = atoi( meas_c );
        }
        else
        {
            std::cerr << "Missing info from file: time sig location" << std::endl;
            return true;  // dont halt program but just ignore event
        }

        addTimeSigChange(meas, num, denom);
        if (m_time_sig_changes.size() > 1)
        {
            m_expanded_mode = true;
        }
    }

    return true;

}

// ----------------------------------------------------------------------------------------------------------

void MeasureData::saveToFile(wxFileOutputStream& fileout)
{
    writeData(wxT("<measure ") +
              wxString( wxT(" firstMeasure=\"") ) + to_wxString(getFirstMeasure()),
              fileout);

    if (isMeasureLengthConstant())
    {
        writeData(wxT("\" denom=\"") + to_wxString(getTimeSigDenominator()) +
                  wxT("\" num=\"")   + to_wxString(getTimeSigNumerator()) +
                  wxT("\"/>\n\n"),
                  fileout );
    }
    else
    {
        writeData(wxT("\">\n"), fileout );
        const int timeSigAmount = m_time_sig_changes.size();
        for (int n=0; n<timeSigAmount; n++)
        {
            writeData(wxT("<timesig num=\"") + to_wxString(m_time_sig_changes[n].getNum())     +
                      wxT("\" denom=\"")     + to_wxString(m_time_sig_changes[n].getDenom())   +
                      wxT("\" measure=\"")   + to_wxString(m_time_sig_changes[n].getMeasure()) + wxT("\"/>\n"),
                      fileout );
        }//next
        writeData(wxT("</measure>\n\n"), fileout );
    }
}


// ----------------------------------------------------------------------------------------------------------

float MeasureData::getBeatSize(int measure) const
{
    if (measure == -1) measure = 0; // no parameter passed, use measure 0 settings

    const int num   = getTimeSigNumerator(measure);
    const int denom = getTimeSigDenominator(measure);
    return getBeatSize(num, denom);
}


// ----------------------------------------------------------------------------------------------------------

int MeasureData::getBeatCount(int measure) const
{
    if (measure == -1) measure = 0; // no parameter passed, use measure 0 settings

    const int num   = getTimeSigNumerator(measure);
    const int denom = getTimeSigDenominator(measure);
    return getBeatCount(num, denom);
}


// ----------------------------------------------------------------------------------------------------------

int MeasureData::getMeasureLengthInTicks(int num, int denom) const
{
    return (int)((float)m_sequence->ticksPerQuarterNote() * 
                 (float)getBeatCount(num, denom) * getBeatSize(num, denom));
}

// ----------------------------------------------------------------------------------------------------------

// Returns the number of beat in a bar given the time signature
// This is simply given in theory by numerator
// But, practically, the result might be different for certain 
// ternary time signatures (e. g. : 3/8, 6/8, 3/4)
// cf. http://www2.siba.fi/muste1/index.php?id=98&la=en
// This is why we also consider denominator for possible evolutions
int MeasureData::getBeatCount(int numerator, int denominator) const
{
    return numerator;
}


// ----------------------------------------------------------------------------------------------------------

// Returns the beat size expressed in quarter note
// May be less than 1.0 (ex : 0.5 => eighth note)
// This is simply given in theory by denominator
// But, practically, the result might be different for certain 
// ternary time signatures (e. g. : 3/8, 6/8, 3/4)
// cf. http://www2.siba.fi/muste1/index.php?id=98&la=en
// This is why we also consider numerator for possible evolutions
float MeasureData::getBeatSize(int numerator, int denominator) const
{
    return 4.0/(float)denominator;
}

