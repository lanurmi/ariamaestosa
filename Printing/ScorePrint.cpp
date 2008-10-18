
#include <wx/wx.h>
#include <wx/sizer.h>
#include <wx/file.h>

#include "Midi/Sequence.h"
#include "AriaCore.h"

#include "Midi/MeasureData.h"
#include "Editors/ScoreEditor.h"
#include "Editors/ScoreAnalyser.h"
#include "IO/IOUtils.h"
#include "Printing/ScorePrint.h"
#include "Printing/PrintingBase.h"
#include "Printing/PrintLayout.h"

namespace AriaMaestosa
{
    
    
ScorePrintable::ScorePrintable(Track* track) : EditorPrintable() { }
ScorePrintable::~ScorePrintable() { }

/*
 * An instance of this will be given to the score analyser. Having a separate x-coord-converter 
 * allows it to do conversions between units without becoming context-dependent.
 */
class PrintXConverter : public TickToXConverter
{
    ScorePrintable* parent;
public:
    LEAK_CHECK(PrintXConverter);
    
    PrintXConverter(ScorePrintable* parent)
    {
        PrintXConverter::parent = parent;
    }
    ~PrintXConverter(){}
    int tickToX(const int tick)
    {
        return parent->tickToX(tick);
    }
};

/*
 * A few drawing routines
 */
void renderSharp(wxDC& dc, const int x, const int y)
{
    dc.SetPen(  wxPen( wxColour(0,0,0), 1 ) );
    
    // horizontal lines
    dc.DrawLine( x-5, y, x+5, y-2 );
    dc.DrawLine( x-5, y+4, x+5, y+2 );
    
    // vertical lines
    dc.DrawLine( x-2, y-3, x-2, y+6 );
    dc.DrawLine( x+2, y-4, x+2, y+5 );
}
void renderFlat(wxDC& dc, const int x, const int y)
{
    dc.SetPen(  wxPen( wxColour(0,0,0), 1 ) );
    
    wxPoint points[] = 
    {
        wxPoint(x,     y-3),
        wxPoint(x,     y+12),
        wxPoint(x+1,   y+12),
        wxPoint(x+5,   y+6),
        wxPoint(x+3,   y+4),
        wxPoint(x,     y+6)
    };
    dc.DrawSpline(6, points);
}
void renderNatural(wxDC& dc, const int x, const int y)
{
    dc.SetPen(  wxPen( wxColour(0,0,0), 1 ) );
    
    // horizontal lines
    dc.DrawLine( x-3, y,   x+3, y-2 );
    dc.DrawLine( x-3, y+4, x+3, y+2 );
    
    // vertical lines
    dc.DrawLine( x-3, y+4, x-3, y-6 );
    dc.DrawLine( x+3, y-2, x+3, y+8 );
}
void renderGClef(wxDC& dc, const int x, const int y)
{
    /*
    dc.SetPen(  wxPen( wxColour(0,0,0), 1 ) );
    
    const float scale = 10;
    
#define POINT(MX,MY) wxPoint( (int)round(x + MX*scale), (int)round(y - MY*scale) )
    
    wxPoint points[] = 
    {
        POINT(4.531, -8.744), // bottom tail
        POINT(3.181, -9.748),
        POINT(4.946, -11.236),
        POINT(7.191, -10.123),
        POINT(7.577, -6.909),
        POINT(6.642, -1.336),
        POINT(4.941, 4.612),
        POINT(3.852, 10.668),
        POINT(4.527, 14.740),
        POINT(6.063, 16.144), // 10 - top
        POINT(7.227, 15.416),
        POINT(7.485, 11.511),
        POINT(5.365, 8.513),
        POINT(0.796, 3.155),
        POINT(1.062, -1.551), // 15
        POINT(5.739, -3.776), // main circle bottom
        POINT(9.401, 0.390),
        POINT(6.059, 2.953), // main circle top
        POINT(3.358, 1.260),
        POINT(3.908, -1.258), // 20
        POINT(4.503, -1.487) // G end
    };
    
#undef POINT
    
    dc.DrawSpline(21, points);
     */
}

PrintXConverter* x_converter = NULL;

// leave a pointer to the dc for the callback
// FIXME find cleaner way
wxDC* global_dc = NULL;
void renderSilenceCallback(const int tick, const int tick_length, const int silences_y)
{
    assert( global_dc != NULL);
    
    // FIXME - merge common parts with the one in ScoreEditor
    const int beat = getMeasureData()->beatLengthInTicks();
    
    if(tick_length<2) return;
    
    // check if silence spawns over more than one measure
    const int end_measure = getMeasureData()->measureAtTick(tick+tick_length-1);
    if(getMeasureData()->measureAtTick(tick) != end_measure)
    {
        // we need to plit it in two
        const int split_tick = getMeasureData()->firstTickInMeasure(end_measure);
        
        // Check split is valid before attempting.
        if(split_tick-tick>0 and tick_length-(split_tick-tick)>0)
        {
            renderSilenceCallback(tick, split_tick-tick, silences_y);
            renderSilenceCallback(split_tick, tick_length-(split_tick-tick), silences_y);
            return;
        }
    }
    
    if(tick < 0) return; // FIXME - find why it happens
    assertExpr(tick,>,-1);

	const int x = x_converter->tickToX(tick);
	bool dotted = false, triplet = false;
	int type = -1;
	
	int dot_delta_x = 0, dot_delta_y = 0;
	
	const float relativeLength = tick_length / (float)(getMeasureData()->beatLengthInTicks()*4);
	
	const int tick_in_measure_start = (tick) - getMeasureData()->firstTickInMeasure( getMeasureData()->measureAtTick(tick) );
    const int remaining = beat - (tick_in_measure_start % beat);
    const bool starts_on_beat = aboutEqual(remaining,0) or aboutEqual(remaining,beat);
    
	if( aboutEqual(relativeLength, 1.0) ) type = 1;
	else if (aboutEqual(relativeLength, 3.0/2.0) and starts_on_beat){ type = 1; dotted = true; dot_delta_x = 5; dot_delta_y = 2;}
	else if (aboutEqual(relativeLength, 1.0/2.0)) type = 2;
	else if (aboutEqual(relativeLength, 3.0/4.0) and starts_on_beat){ type = 2; dotted = true; dot_delta_x = 5; dot_delta_y = 2;}
    else if (aboutEqual(relativeLength, 1.0/4.0)) type = 4;
    else if (aboutEqual(relativeLength, 1.0/3.0)){ type = 2; triplet = true; }
	else if (aboutEqual(relativeLength, 3.0/8.0) and starts_on_beat){ type = 4; dotted = true; dot_delta_x = -3; dot_delta_y = 10; }
    else if (aboutEqual(relativeLength, 1.0/8.0)) type = 8;
    else if (aboutEqual(relativeLength, 1.0/6.0)){ type = 4; triplet = true; }
	else if (aboutEqual(relativeLength, 3.0/16.0) and starts_on_beat){ type = 8; dotted = true; }
	else if (aboutEqual(relativeLength, 1.0/16.0)) type = 16;
    else if (aboutEqual(relativeLength, 1.0/12.0)){ triplet = true; type = 8; }
	else if(relativeLength < 1.0/16.0){ return; }
	else
	{
		// silence is of unknown duration. split it in a serie of silences.
		
		// start by reaching the next beat if not already done
		if(!starts_on_beat and !aboutEqual(remaining,tick_length))
		{
            renderSilenceCallback(tick, remaining, silences_y);
			renderSilenceCallback(tick+remaining, tick_length - remaining, silences_y);
			return;
		}
		
		// split in two smaller halves. render using a simple recursion.
		float closestShorterDuration = 1;
		while(closestShorterDuration >= relativeLength) closestShorterDuration /= 2.0;
		
		const int firstLength = closestShorterDuration*(float)(getMeasureData()->beatLengthInTicks()*4);
        
		renderSilenceCallback(tick, firstLength, silences_y);
		renderSilenceCallback(tick+firstLength, tick_length - firstLength, silences_y);
		return;
	}
	
    global_dc->SetBrush( *wxBLACK_BRUSH );
    global_dc->SetPen(  wxPen( wxColour(0,0,0), 2 ) );
    
	if( type == 1 )
	{
        global_dc->DrawRectangle(x+4, silences_y-5, 10, 5);
	}
	else if( type == 2 )
	{
        global_dc->DrawRectangle(x+4, silences_y, 10, 5);
	}
	else if( type == 4 )
	{
        const int mx = x + 10;
        const int y = silences_y - 5;
        wxPoint points[] = 
        {
            wxPoint(mx,     y),
            wxPoint(mx+4,   y+4),
            wxPoint(mx+5,   y+5),
            wxPoint(mx+4,   y+6),
            wxPoint(mx+1,   y+9),
            wxPoint(mx,     y+10),
            wxPoint(mx+1,   y+11),
            wxPoint(mx+4,   y+14),
            wxPoint(mx+5,   y+15),
            wxPoint(mx,     y+15),
            wxPoint(mx+1,   y+17),
            wxPoint(mx+5,   y+20),
        };
        global_dc->DrawSpline(12, points);
	}
	else if( type == 8 )
	{
        const int mx = x + 10;
        const int y = silences_y;
        wxPoint points[] = 
        {
        wxPoint(mx,     y+15),
        wxPoint(mx+7,   y),
        wxPoint(mx,     y),
        };
        global_dc->DrawSpline(3, points);
        
        global_dc->DrawCircle(mx, y, 2);
	}
	else if( type == 16 )
	{
        const int mx = x + 10;
        const int y = silences_y + 5;
        wxPoint points[] = 
        {
        wxPoint(mx,     y+10),
        wxPoint(mx+5,   y),
        wxPoint(mx,     y),
        };
        global_dc->DrawSpline(3, points);
        wxPoint points2[] = 
        {
            wxPoint(mx+4,   y+1),
            wxPoint(mx+10,  y-10),
            wxPoint(mx+5,     y-10),
        };
        global_dc->DrawSpline(3, points2);
        
        global_dc->DrawCircle(mx, y, 2);
        global_dc->DrawCircle(mx+5, y-10, 2);
	}
	
	// dotted
	if(dotted)
	{

	}
    
    // triplet
    if(triplet)
    {

    }
}

int ScorePrintable::calculateHeight(LayoutLine& line) const
{
    Track* track = line.getTrack();
    ScoreEditor* scoreEditor = track->graphics->scoreEditor;
    ScoreMidiConverter* converter = scoreEditor->getScoreMidiConverter();
    
    const int from_note = line.getFirstNote();
    const int to_note   = line.getLastNote();
    
    // find highest and lowest note we need to render
    int highest_pitch = -1, lowest_pitch = -1;
    int highest_level = -1, lowest_level = -1;
    for(int n=from_note; n<= to_note; n++)
    {
        const int pitch = track->getNotePitchID(n);
        const int level = converter->noteToLevel(track->getNote(n));
        if(pitch < highest_pitch || highest_pitch == -1)
        {
            highest_pitch = pitch;
            lowest_level = level;
        }
        if(pitch > lowest_pitch  ||  lowest_pitch == -1)
        {
            lowest_pitch  = pitch;
            highest_level  = level;
        }
    }
    
    const int middle_c_level = converter->getScoreCenterCLevel(); //converter->getMiddleCLevel();
    
    const int g_clef_from = middle_c_level-10;
    const int g_clef_to   = middle_c_level-2;
    const int f_clef_from = middle_c_level+2;
    const int f_clef_to   = middle_c_level+10;
    
    const bool g_clef = scoreEditor->isGClefEnabled();
    const bool f_clef = scoreEditor->isFClefEnabled();
    
    int extra_lines_above_g_score = 0;
    int extra_lines_under_g_score = 0;
    int extra_lines_above_f_score = 0;
    int extra_lines_under_f_score = 0;
    if(g_clef and not f_clef)
    {
        if(lowest_level < g_clef_from) extra_lines_above_g_score = (g_clef_from - lowest_level)/2;
        if(highest_level > g_clef_to)  extra_lines_under_g_score = (g_clef_to - highest_level)/2;
        return (g_clef_to - g_clef_from) + abs(extra_lines_above_g_score) + abs(extra_lines_under_g_score);
    }
    else if(f_clef and not g_clef)
    {
        if(lowest_level < f_clef_from) extra_lines_above_f_score = (f_clef_from - lowest_level)/2;
        if(highest_level > f_clef_to)  extra_lines_under_f_score = (f_clef_to - highest_level)/2;
        return (f_clef_to - f_clef_from) + abs(extra_lines_above_f_score) + abs(extra_lines_under_f_score);
    }
    else if(f_clef and g_clef)
    {
        if(lowest_level < g_clef_from) extra_lines_above_g_score = (g_clef_from - lowest_level)/2;
        if(highest_level > f_clef_to)  extra_lines_under_f_score = (f_clef_to - highest_level)/2;
        return (f_clef_to - g_clef_from) + abs(extra_lines_above_g_score) + abs(extra_lines_under_f_score);
    }
    return -1; // should not happen
}

void ScorePrintable::drawLine(LayoutLine& line, wxDC& dc,
                              const int x0, const int y0,
                              const int x1, const int y1,
                              bool show_measure_number)
{
    assertExpr(y0,>,0);
    assertExpr(y1,>,0);
    assertExpr(y0,<,1000);
    assertExpr(y1,<,1000);
    
    std::cout << "==========\nline from note " << line.getFirstNote() << " to " << line.getLastNote() << std::endl;
    
    Track* track = line.getTrack();
    ScoreEditor* scoreEditor = track->graphics->scoreEditor;
    ScoreMidiConverter* converter = scoreEditor->getScoreMidiConverter();
    
    const int from_note = line.getFirstNote();
    const int to_note   = line.getLastNote();
    
    // find highest and lowest note we need to render
    int highest_pitch = -1, lowest_pitch = -1;
    int highest_level = -1, lowest_level = -1;
    for(int n=from_note; n<= to_note; n++)
    {
        const int pitch = track->getNotePitchID(n);
        const int level = converter->noteToLevel(track->getNote(n));
        if(pitch < highest_pitch || highest_pitch == -1)
        {
            highest_pitch = pitch;
            lowest_level = level;
        }
        if(pitch > lowest_pitch  ||  lowest_pitch == -1)
        {
            lowest_pitch  = pitch;
            highest_level  = level;
        }
    }

    //const int highest_level = converter->noteToLevel(track->getNote(highest));
   // const int lowest_level  = converter->noteToLevel(track->getNote(lowest));
    
    const int middle_c_level = converter->getScoreCenterCLevel(); //converter->getMiddleCLevel();

    const int g_clef_from = middle_c_level-10;
    const int g_clef_to   = middle_c_level-2;
    const int f_clef_from = middle_c_level+2;
    const int f_clef_to   = middle_c_level+10;
    
    std::cout << "level --> highest : " << highest_level << ", lowest : " << lowest_level << 
        " (g_clef_from=" << g_clef_from << ", g_clef_to=" << g_clef_to << ")" << std::endl;
    
    const bool g_clef = scoreEditor->isGClefEnabled();
    const bool f_clef = scoreEditor->isFClefEnabled();

    int extra_lines_above_g_score = 0;
    int extra_lines_under_g_score = 0;
    int extra_lines_above_f_score = 0;
    int extra_lines_under_f_score = 0;
    if(g_clef and not f_clef)
    {
        if(lowest_level < g_clef_from) extra_lines_above_g_score = (g_clef_from - lowest_level)/2;
        if(highest_level > g_clef_to)  extra_lines_under_g_score = (g_clef_to - highest_level)/2;
    }
    else if(f_clef and not g_clef)
    {
        if(lowest_level < f_clef_from) extra_lines_above_f_score = (f_clef_from - lowest_level)/2;
        if(highest_level > f_clef_to)  extra_lines_under_f_score = (f_clef_to - highest_level)/2;
    }
    else if(f_clef and g_clef)
    {
        if(lowest_level < g_clef_from) extra_lines_above_g_score = (g_clef_from - lowest_level)/2;
        if(highest_level > f_clef_to)  extra_lines_under_f_score = (f_clef_to - highest_level)/2;
    }
    std::cout << "extra_lines_above_g_score = " << extra_lines_above_g_score <<
        " extra_lines_under_g_score = " << extra_lines_under_g_score << 
        " extra_lines_above_f_score = " << extra_lines_above_f_score <<
        " extra_lines_under_f_score = " << extra_lines_under_f_score << std::endl;
    
    // get the underlying common implementation rolling
    beginLine(&dc, &line, x0, y0, x1, y1, show_measure_number);
    
    // prepare the score analyser
    x_converter = new PrintXConverter(this);
    
    OwnerPtr<ScoreAnalyser> g_clef_analyser;
    OwnerPtr<ScoreAnalyser> f_clef_analyser;
    
    if(g_clef)
    {
        g_clef_analyser = new ScoreAnalyser(scoreEditor, new PrintXConverter(this), middle_c_level-5);
        g_clef_analyser->setStemDrawInfo( 14, 0, 6, 0 );
        g_clef_analyser->setStemPivot(middle_c_level-5);
    }
    if(f_clef)
    {
        f_clef_analyser = new ScoreAnalyser(scoreEditor, new PrintXConverter(this), middle_c_level-5);
        f_clef_analyser->setStemDrawInfo( 14, 0, 6, 0 );
        f_clef_analyser->setStemPivot(middle_c_level+6);
    }
    
    converter->updateConversionData();
    converter->resetAccidentalsForNewRender();
    
    // iterate through layout elements to collect notes in the vector
    // so ScoreAnalyser can prepare the score
    LayoutElement* currentElement;
    while((currentElement = getNextElement()) and (currentElement != NULL))
    {
        if(currentElement->type == LINE_HEADER) // FIME - move to 'drawScore' ?
        {
            //renderGClef(dc, currentElement->x, LEVEL_TO_Y(middle_c_level-5) );
            continue;
        }
        // we're collecting notes here... types other than regular measures
        // don't contain notes and thus don't interest us
        if(currentElement->type != SINGLE_MEASURE) continue;
        
        const int firstNote = line.getFirstNoteInElement(currentElement);
        const int lastNote = line.getLastNoteInElement(currentElement);
        
        for(int n=firstNote; n<lastNote; n++)
        {
            int note_sign;
            const int noteLevel = converter->noteToLevel(track->getNote(n), &note_sign);
            
            if(noteLevel == -1) continue;
            const int noteLength = track->getNoteEndInMidiTicks(n) - track->getNoteStartInMidiTicks(n);
            const int tick = track->getNoteStartInMidiTicks(n);
            
            const int note_x = tickToX(tick);
            
			NoteRenderInfo currentNote(tick, note_x, noteLevel, noteLength, note_sign,
                                       track->isNoteSelected(n), track->getNotePitchID(n));
            
            // add note to either G clef score or F clef score
            if(g_clef and not f_clef)
                g_clef_analyser->addToVector(currentNote, false);
            else if(f_clef and not g_clef)
                f_clef_analyser->addToVector(currentNote, false);
            else if(f_clef and g_clef)
            {
                if(noteLevel < middle_c_level)
                    g_clef_analyser->addToVector(currentNote, false);
                else
                    f_clef_analyser->addToVector(currentNote, false);
            }
        }
        
    }//next element
        
    // if we have only one clef, give it the full space.
    // if we have two, split the space between both
    int g_clef_y_from, g_clef_y_to;
    int f_clef_y_from, f_clef_y_to;
    
    if(g_clef and not f_clef)
    {
        g_clef_y_from = y0;
        g_clef_y_to = y1;
    }
    else if(f_clef and not g_clef)
    {
        f_clef_y_from = y0;
        f_clef_y_to = y1;
    }
    else if(f_clef and g_clef)
    {
        g_clef_y_from = y0;
        g_clef_y_to = y0 + (int)round((y1 - y0)*0.45);
        f_clef_y_from = y0 + (int)round((y1 - y0)*0.55);
        f_clef_y_to = y1;
    }
    
    if(g_clef)
    {
        drawScore(false /*G*/, *g_clef_analyser, line, dc,
                  abs(extra_lines_above_g_score), abs(extra_lines_under_g_score),
                  x0, g_clef_y_from, x1, g_clef_y_to,
                  show_measure_number);
    }
    
    if(f_clef)
    {
        drawScore(true /*F*/, *f_clef_analyser, line, dc,
                  abs(extra_lines_above_f_score), abs(extra_lines_under_f_score),
                  x0, f_clef_y_from, x1, f_clef_y_to,
                  (g_clef ? false : show_measure_number) /* if we have both keys don't show twice */);
    }
    
    delete x_converter;
    x_converter = NULL;
}

// -------------------------------------------------------------------
void ScorePrintable::drawScore(bool f_clef, ScoreAnalyser& analyser, LayoutLine& line, wxDC& dc,
                               const int extra_lines_above, const int extra_lines_under,
                               const int x0, const int y0, const int x1, const int y1,
                               bool show_measure_number)
{
    Track* track = line.getTrack();
    ScoreEditor* scoreEditor = track->graphics->scoreEditor;
    ScoreMidiConverter* converter = scoreEditor->getScoreMidiConverter();
    const int middle_c_level = converter->getScoreCenterCLevel();
    const int first_score_level = middle_c_level + (f_clef? 2 : -10);
    const int last_score_level = first_score_level + 8;
    const int min_level =  first_score_level - extra_lines_above*2;
    const int headRadius = 9;//(int)round((float)lineHeight*0.72);
           
    // draw score background (lines)
    dc.SetPen(  wxPen( wxColour(125,125,125), 1 ) );
    const int lineAmount = 5 + extra_lines_above + extra_lines_under;
    const float lineHeight = (float)(y1 - y0) / (float)(lineAmount-1);
    
    #define LEVEL_TO_Y( lvl ) y0 + 1 + lineHeight*0.5*(lvl - min_level)
    
    for(int lvl=first_score_level; lvl<=last_score_level; lvl+=2)
    {
        const int y = LEVEL_TO_Y(lvl);
        dc.DrawLine(x0, y, x1, y);
    }
    
    // draw notes heads
    { // we scope this because info like 'noteAmount' are bound to change just after
    const int noteAmount = analyser.noteRenderInfo.size();
    for(int i=0; i<noteAmount; i++)
    {
        NoteRenderInfo& noteRenderInfo = analyser.noteRenderInfo[i];
        
        dc.SetPen(  wxPen( wxColour(0,0,0), 2 ) );
        if(noteRenderInfo.hollow_head) dc.SetBrush( *wxTRANSPARENT_BRUSH );
        else dc.SetBrush( *wxBLACK_BRUSH );
        
        const int notey = LEVEL_TO_Y(noteRenderInfo.getBaseLevel());
        wxPoint headLocation( noteRenderInfo.x + headRadius - 3, notey-headRadius/2.0+1 );
        dc.DrawEllipse( headLocation, wxSize(headRadius-1, headRadius-2) );
        noteRenderInfo.setY(notey+headRadius/2.0);
        
        if(noteRenderInfo.dotted)
        {
            wxPoint headLocation( noteRenderInfo.x + headRadius*2.3, notey+1 );
            dc.DrawEllipse( headLocation, wxSize(2,2) );
        }
        
        if(noteRenderInfo.sign == SHARP)        renderSharp  ( dc, noteRenderInfo.x,     noteRenderInfo.getY() - 6  );
        else if(noteRenderInfo.sign == FLAT)    renderFlat   ( dc, noteRenderInfo.x - 2, noteRenderInfo.getY() - 11 );
        else if(noteRenderInfo.sign == NATURAL) renderNatural( dc, noteRenderInfo.x,     noteRenderInfo.getY() - 5  );
        
    } // next note
    }// end scope
    
    // analyse notes to know how to build the score
    analyser.analyseNoteInfo();
    
    // now that score was analysed, draw the remaining note bits
    const int noteAmount = analyser.noteRenderInfo.size();
    for(int i=0; i<noteAmount; i++)
    {
        NoteRenderInfo& noteRenderInfo = analyser.noteRenderInfo[i];
        
        dc.SetPen(  wxPen( wxColour(0,0,0), 2 ) );
        
        // draw stem
        if(noteRenderInfo.stem_type != STEM_NONE)
        {
            dc.DrawLine( analyser.getStemX(noteRenderInfo), LEVEL_TO_Y(analyser.getStemFrom(noteRenderInfo)),
                         analyser.getStemX(noteRenderInfo), LEVEL_TO_Y(analyser.getStemTo(noteRenderInfo))    );
        }
        
        // drag flags
        if(noteRenderInfo.flag_amount>0 and not noteRenderInfo.beam)
        {
            const int stem_end = LEVEL_TO_Y(analyser.getStemTo(noteRenderInfo));
            const int flag_x_origin = analyser.getStemX(noteRenderInfo);
            const int flag_step = (noteRenderInfo.stem_type==STEM_UP ? 7 : -7 );
            
            for(int n=0; n<noteRenderInfo.flag_amount; n++)
            {
                const int flag_y = stem_end + n*flag_step;
                const int orient = (noteRenderInfo.stem_type==STEM_UP ? 1 : -1 );
                
                wxPoint points[] = 
                {
                    wxPoint(flag_x_origin, flag_y),
                    wxPoint(flag_x_origin + 3, flag_y + orient*6),
                    wxPoint(flag_x_origin + 11, flag_y + orient*11),
                    wxPoint(flag_x_origin + 9, flag_y + orient*15)
                };
                dc.DrawSpline(4, points);
            }
        }
        
        // ties
        if(noteRenderInfo.getTiedToPixel() != -1)
        {
            wxPen tiePen( wxColour(0,0,0), 1 ) ;
            //tiePen.SetJoin(wxJOIN_BEVEL);
            dc.SetPen( tiePen );
            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            
            const bool show_above = noteRenderInfo.isTieUp();
            const int base_y = LEVEL_TO_Y( noteRenderInfo.getStemOriginLevel() ) + (show_above ? - 9 : 9); 
            
            dc.DrawEllipticArc(noteRenderInfo.x + headRadius*1.6,
                               base_y - 8,
                               noteRenderInfo.getTiedToPixel() - noteRenderInfo.x /*- headRadius*1.6*/,
                               16,
                               (show_above ? 0   : 180),
                               (show_above ? 180 : 360));
        }
        
        /*
         
         int center_x = noteRenderInfo.x + headRadius*1.8 + (noteRenderInfo.getTiedToPixel() - noteRenderInfo.x)/2;
         int center_y = base_y;
         int radius_x = (noteRenderInfo.getTiedToPixel() - noteRenderInfo.x)/2;
         int radius_y = (show_above ? -8 : 8);
         
         center_x *= 10;
         center_y *= 10;
         radius_x *= 10;
         radius_y *= 10;
         
         dc.SetUserScale(0.1, 0.1);
         wxPoint points[] = 
         {
             wxPoint(center_x + radius_x*cos(0.1), center_y + radius_y*sin(0.1)),
             wxPoint(center_x + radius_x*cos(0.3), center_y + radius_y*sin(0.3)),
             wxPoint(center_x + radius_x*cos(0.6), center_y + radius_y*sin(0.6)),
             wxPoint(center_x + radius_x*cos(0.9), center_y + radius_y*sin(0.9)),
             wxPoint(center_x + radius_x*cos(1.2), center_y + radius_y*sin(1.2)),
             wxPoint(center_x + radius_x*cos(1.5), center_y + radius_y*sin(1.5)),
             wxPoint(center_x + radius_x*cos(1.8), center_y + radius_y*sin(1.8)),
             wxPoint(center_x + radius_x*cos(2.1), center_y + radius_y*sin(2.1)),
             wxPoint(center_x + radius_x*cos(2.4), center_y + radius_y*sin(2.4)),
             wxPoint(center_x + radius_x*cos(2.7), center_y + radius_y*sin(2.7)),
             wxPoint(center_x + radius_x*cos(3.0), center_y + radius_y*sin(3.0)),
         };
         dc.DrawSpline(11, points);
         dc.SetUserScale(1.0, 1.0);
         */
        // beam
        if(noteRenderInfo.beam)
        {
            dc.SetPen(  wxPen( wxColour(0,0,0), 2 ) );
            dc.SetBrush( *wxBLACK_BRUSH );
            
            const int x1 = analyser.getStemX(noteRenderInfo);
            int y1       = LEVEL_TO_Y(analyser.getStemTo(noteRenderInfo));
            int y2       = LEVEL_TO_Y(noteRenderInfo.beam_to_level);
            
            const int y_diff = (noteRenderInfo.stem_type == STEM_UP ? 7 : -7);
            
            for(int n=0; n<noteRenderInfo.flag_amount; n++)
            {
                //dc.DrawLine(x1, y1, noteRenderInfo.beam_to_x, y2);
                wxPoint points[] = 
            {
                wxPoint(x1, y1),
                wxPoint(noteRenderInfo.beam_to_x, y2),
                wxPoint(noteRenderInfo.beam_to_x, y2+3),
                wxPoint(x1, y1+3)
            };
                dc.DrawPolygon(4, points);
                
                y1 += y_diff;
                y2 += y_diff;
            }
            dc.SetBrush( *wxTRANSPARENT_BRUSH );
        }
        
        // triplet
        if (noteRenderInfo.drag_triplet_sign and noteRenderInfo.triplet_arc_x_start != -1)
        {
            wxPen tiePen( wxColour(0,0,0), 1 ) ;
            //tiePen.SetJoin(wxJOIN_BEVEL);
            dc.SetPen( tiePen );
            dc.SetBrush( *wxTRANSPARENT_BRUSH );
            
            const int center_x = (noteRenderInfo.triplet_arc_x_end == -1 ? noteRenderInfo.triplet_arc_x_start : (noteRenderInfo.triplet_arc_x_start + noteRenderInfo.triplet_arc_x_end)/2);
            const int radius_x = (noteRenderInfo.triplet_arc_x_end == -1 or  noteRenderInfo.triplet_arc_x_end == noteRenderInfo.triplet_arc_x_start ?
                                  10 : (noteRenderInfo.triplet_arc_x_end - noteRenderInfo.triplet_arc_x_start)/2);
            
            const int base_y = LEVEL_TO_Y(noteRenderInfo.triplet_arc_level) + (noteRenderInfo.triplet_show_above ? -16 : 1);
            
            dc.DrawEllipticArc(center_x - radius_x,
                               base_y,
                               radius_x*2,
                               16,
                               (noteRenderInfo.triplet_show_above ? 0   : 180),
                               (noteRenderInfo.triplet_show_above ? 180 : 360));
            
            dc.SetTextForeground( wxColour(0,0,0) );
            dc.DrawText( wxT("3"), center_x-2, base_y + (noteRenderInfo.triplet_show_above ? 0 : 5) );
        }
        
        
    } // next note
    
    // render silences
    const int first_measure = line.getFirstMeasure();
    const int last_measure  = line.getLastMeasure();
    
    global_dc = &dc;
    
    if(f_clef)
    {
        const int silences_y = LEVEL_TO_Y(middle_c_level + 4);
        analyser.renderSilences( &renderSilenceCallback, first_measure, last_measure, silences_y );
    }
    else
    {
        const int silences_y = LEVEL_TO_Y(middle_c_level - 7);
        analyser.renderSilences( &renderSilenceCallback, first_measure, last_measure, silences_y );
    }
    
}

}
