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


#include <cmath>
#include "Config.h"

#include "wx/wx.h"

#include "OpenGL.h"

#include "Actions/EditAction.h"
#include "Actions/AddNote.h"

#include "Images/Drawable.h"
#include "Images/ImageProvider.h"
#include "Images/Image.h"
#include "Editors/KeyboardEditor.h"
#include "Editors/RelativeXCoord.h"
#include "Midi/Sequence.h"
#include "Midi/Track.h"
#include "Pickers/MagneticGrid.h"
#include "Pickers/VolumeSlider.h"
#include "Pickers/KeyPicker.h"
#include "GUI/GLPane.h"
#include "GUI/GraphicalTrack.h"

#include "main.h"



namespace AriaMaestosa {
	
const int y_step = 10;
	
// ***********************************************************************************************************************************************************
// **********************************************************    CONSTRUCTOR      ****************************************************************************
// ***********************************************************************************************************************************************************

KeyboardEditor::KeyboardEditor(Track* track) : Editor(track)
{
    note_greyed_out[0] = false;
    note_greyed_out[1] = true;
    note_greyed_out[2] = false;
    note_greyed_out[3] = false;
    note_greyed_out[4] = true;
    note_greyed_out[5] = false;
    note_greyed_out[6] = true;
    note_greyed_out[7] = false;
    note_greyed_out[8] = false;
    note_greyed_out[9] = true;
    note_greyed_out[10] = false;
    note_greyed_out[11] = true;
    sb_position=0.5;
}

KeyboardEditor::~KeyboardEditor()
{
    
}

// ****************************************************************************************************************************************************
// *********************************************************    EVENTS      ***************************************************************************
// ****************************************************************************************************************************************************

void KeyboardEditor::mouseDown(RelativeXCoord x, const int y)
{
    // user clicked on left bar to change tuning
	if(x.getRelativeTo(EDITOR)<-20 and x.getRelativeTo(WINDOW)>15 and y>getEditorYStart())
	{
		getMainFrame()->keyPicker->setParent(track->graphics);
		getMainFrame()->keyPicker->noChecks();
		getGLPane()->PopupMenu( getMainFrame()->keyPicker, x.getRelativeTo(WINDOW), y);
        return;
	}
    
    Editor::mouseDown(x, y);
}

// ****************************************************************************************************************************************************
// ****************************************************    EDITOR METHODS      ************************************************************************
// ****************************************************************************************************************************************************


void KeyboardEditor::addNote(const int snapped_start_tick, const int snapped_end_tick, const int mouseY)
{
	const int note = (mouseY - getEditorYStart() + getYScrollInPixels())/y_step;
	track->action( new Action::AddNote(note, snapped_start_tick, snapped_end_tick, 80 ) );
}

void KeyboardEditor::noteClicked(const int id)
{
	track->selectNote(ALL_NOTES, false);
	track->selectNote(id, true);
	track->playNote(id);
}

NoteSearchResult KeyboardEditor::noteAt(RelativeXCoord x, const int y, int& noteID)
{
	const int x_edit = x.getRelativeTo(EDITOR);
	
	const int noteAmount = track->getNoteAmount();
	for(int n=0; n<noteAmount; n++)
	{
		const int x1=track->getNoteStartInPixels(n) - sequence->getXScrollInPixels();
		const int x2=track->getNoteEndInPixels(n) - sequence->getXScrollInPixels();
		const int y1=track->getNotePitchID(n)*y_step + getEditorYStart() - getYScrollInPixels();

		if(x_edit > x1 and x_edit < x2 and y > y1 and y < y1+12)
		{
			noteID = n;
			
			if(track->isNoteSelected(n) and !getGLPane()->isSelectLessPressed())
			{
				// clicked on a selected note
				return FOUND_SELECTED_NOTE;
			}
			else
			{
				return FOUND_NOTE;
			}
			
			
		}//end if
	}//next
	
	return FOUND_NOTHING;
}

void KeyboardEditor::selectNotesInRect(RelativeXCoord& mousex_current, int mousey_current,
									   RelativeXCoord& mousex_initial, int mousey_initial)
{
	for(int n=0; n<track->getNoteAmount(); n++)
	{
		
		int x1=track->getNoteStartInPixels(n);
		int x2=track->getNoteEndInPixels(n);
		int from_note=track->getNotePitchID(n);
		
		if( x1>std::min( mousex_current.getRelativeTo(EDITOR), mousex_initial.getRelativeTo(EDITOR) ) + sequence->getXScrollInPixels() and
			x2<std::max( mousex_current.getRelativeTo(EDITOR), mousex_initial.getRelativeTo(EDITOR) ) + sequence->getXScrollInPixels() and
			from_note*y_step+getEditorYStart()-getYScrollInPixels() > std::min(mousey_current, mousey_initial) and
			from_note*y_step+getEditorYStart()-getYScrollInPixels() < std::max(mousey_current, mousey_initial) )
		{
			
			track->selectNote(n, true);
			
		}else{
			track->selectNote(n, false);
		}
	}//next
	
}

int KeyboardEditor::getYScrollInPixels()
{
    return (int)(   sb_position*(120*11-height-20)   );
}

void KeyboardEditor::moveNote(Note& note, const int relativeX, const int relativeY)
{
    if(note.startTick+relativeX < 0) return; // refuse to move before song start
    if(note.pitchID+relativeY < 0) return; // reject moves that would make illegal notes
    if(note.pitchID+relativeY >= 128) return;
    
    note.startTick += relativeX;
    note.endTick   += relativeX;
    note.pitchID   += relativeY;
}

// ***********************************************************************************************************************************************************
// ************************************************************    RENDER      *******************************************************************************
// ***********************************************************************************************************************************************************

void KeyboardEditor::render()
{
	render( RelativeXCoord_empty(), -1, RelativeXCoord_empty(), -1, true );
}

void KeyboardEditor::render(RelativeXCoord mousex_current, int mousey_current,
							RelativeXCoord mousex_initial, int mousey_initial, bool focus)
{
    
    if(!ImageProvider::imagesLoaded()) return;

    assert(sbArrowDrawable->image!=NULL);
    assert(sbBackgDrawable->image!=NULL);
    assert(sbBarDrawable->image!=NULL);
    assert(noteTrackDrawable->image!=NULL);
    //assert(noteTrackBackgDrawable->image!=NULL);
    
    glEnable(GL_SCISSOR_TEST);
    // glScissor doesn't seem to follow the coordinate system so this ends up in all kinds of weird code to map to my coord system (from_y going down)
    glScissor(10, getGLPane()->getHeight() - (20+height + from_y+barHeight+20), width-15, 20+height);
    
    // ------------------ draw lined background ----------------
    //glEnable(GL_TEXTURE_2D);
    
    glLoadIdentity();
    int levelid = getYScrollInPixels()/y_step;
    const int yscroll = getYScrollInPixels();
    const int y1 = getEditorYStart();
    const int last_note = ( yscroll + getYEnd() - getEditorYStart() )/y_step;
    const int x1 = getEditorXStart();
    const int x2 = getXEnd();
    
    // white background
    glColor4f(1,1,1,1);
    glBegin(GL_QUADS);
    glVertex2f(x1, getEditorYStart());
    glVertex2f(x2, getEditorYStart());
    glVertex2f(x2, getYEnd());
    glVertex2f(x1, getYEnd());
    glEnd();
    
    // horizontal lines
    glColor4f(0.94, 0.94, 0.94, 1);
    while(levelid < last_note)
    {
        const int note12 = 11 - ((levelid - 3) % 12);
        if(note_greyed_out[note12])
        {
            glBegin(GL_QUADS);
            glVertex2f(x1, y1 + levelid*y_step - yscroll+1);
            glVertex2f(x2, y1 + levelid*y_step - yscroll+1);
            glVertex2f(x2, y1 + (levelid+1)*y_step - yscroll+1);
            glVertex2f(x1, y1 + (levelid+1)*y_step - yscroll+1);
            glEnd();
        }
        else
        {
            glBegin(GL_LINES);
            glVertex2f(x1, y1 + (levelid+1)*y_step - yscroll+1);
            glVertex2f(x2, y1 + (levelid+1)*y_step - yscroll+1);
            glEnd();
        }
        
        levelid++;
    }

    drawVerticalMeasureLines(getEditorYStart(), getYEnd());

    // ---------------------- draw notes ----------------------------
    const int noteAmount = track->getNoteAmount();
    for(int n=0; n<noteAmount; n++)
    {

        int x1=track->getNoteStartInPixels(n) - sequence->getXScrollInPixels();
        int x2=track->getNoteEndInPixels(n) - sequence->getXScrollInPixels();
        
        // don't draw notes that won't be visible
        if(x2<0) continue;
        if(x1>width) break;
        
        int y=track->getNotePitchID(n);
        float volume=track->getNoteVolume(n)/127.0;
        
        if(track->isNoteSelected(n) and focus) glColor3f((1-volume)*1, (1-(volume/2))*1, 0);
        else glColor3f((1-volume)*0.9, (1-volume)*0.9, (1-volume)*0.9);
        
        glBegin(GL_QUADS);
        glVertex2f(x1+getEditorXStart()+1, y*y_step+1 + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x1+getEditorXStart()+1, (y+1)*y_step + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x2+getEditorXStart()-1, (y+1)*y_step + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x2+getEditorXStart()-1, y*y_step+1 + getEditorYStart() - getYScrollInPixels());
        glEnd();

        glColor3f(0, 0, 0);
        glBegin(GL_LINES);
        glVertex2f(x1+getEditorXStart()+1, y*y_step+1 + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x1+getEditorXStart()+1, (y+1)*y_step + getEditorYStart() - getYScrollInPixels());
        
        glVertex2f(x1+getEditorXStart()+1, (y+1)*y_step+1 + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x2+getEditorXStart()-1, (y+1)*y_step+1 + getEditorYStart() - getYScrollInPixels());
        
        glVertex2f(x2+getEditorXStart(), (y+1)*y_step + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x2+getEditorXStart(), y*y_step+1 + getEditorYStart() - getYScrollInPixels());
        
        glVertex2f(x1+getEditorXStart()+1, y*y_step+1 + getEditorYStart() - getYScrollInPixels());
        glVertex2f(x2+getEditorXStart()-1, y*y_step+1 + getEditorYStart() - getYScrollInPixels());
        glEnd();
        
    }
    
    
    // ------------------ draw keyboard ----------------
    
    // grey background
    glDisable(GL_TEXTURE_2D);
    if(!focus) glColor3f(0.4, 0.4, 0.4);
    else glColor3f(0.8, 0.8, 0.8);
    glBegin(GL_QUADS);
    
    glVertex2f( 0, getEditorYStart());
    glVertex2f( 0, getYEnd());
    glVertex2f( getEditorXStart()-25, getYEnd());
    glVertex2f( getEditorXStart()-25, getEditorYStart());
    
    glEnd();
    
    for(int g_octaveID=0; g_octaveID<11; g_octaveID++)
    {
        int g_octave_y=g_octaveID*120-getYScrollInPixels();
        if(g_octave_y>-120 and g_octave_y<height+20)
        {
            
            glEnable(GL_TEXTURE_2D);
            
            if(!focus) glColor3f(0.5, 0.5, 0.5);
            else glColor3f(1,1,1);
            
            noteTrackDrawable->move(getEditorXStart()-noteTrackDrawable->image->width, from_y+barHeight+20 + g_octave_y);
            noteTrackDrawable->render();
            
            
            glLoadIdentity();
            glDisable(GL_TEXTURE_2D);
            glColor3f(0,0,0);
            
            glBegin(GL_LINES);
            glVertex2f(0, from_y+barHeight+20 + g_octave_y+1);
            glVertex2f(getEditorXStart()-25, from_y+barHeight+20 + g_octave_y+1);
            glEnd();
            
            glRasterPos2f(30,from_y+barHeight+21 + g_octave_y +120/2);
            
            char buffer[2];
            sprintf (buffer, "%d", 10-g_octaveID);
            
            for(int i=0; buffer[i]; i++){
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
            }
            glEnable(GL_TEXTURE_2D);
            
        }//end if
    }//next

    
    
    glEnable(GL_TEXTURE_2D);
    
    // ------------------------- mouse drag (preview) ------------------------
    
    if(!clickedOnNote)
    {
        if(mouse_is_in_editor)
        {
            
            // selection
			if(selecting)
			{
                glDisable(GL_TEXTURE_2D);
                glColor3f(0,0,0);
                glBegin(GL_LINES);
                
				//std::cout << "mousex_initial.getRelativeTo(WINDOW) = " << mousex_initial.getRelativeTo(WINDOW) << std::endl;
				
                glVertex2f(mousex_initial.getRelativeTo(WINDOW), mousey_current);
                glVertex2f(mousex_initial.getRelativeTo(WINDOW), mousey_initial);
                
                glVertex2f(mousex_current.getRelativeTo(WINDOW), mousey_initial);
                glVertex2f(mousex_initial.getRelativeTo(WINDOW), mousey_initial);
                
                glVertex2f(mousex_current.getRelativeTo(WINDOW), mousey_initial);
                glVertex2f(mousex_current.getRelativeTo(WINDOW), mousey_current);
                
                glVertex2f(mousex_initial.getRelativeTo(WINDOW), mousey_current);
                glVertex2f(mousex_current.getRelativeTo(WINDOW), mousey_current);
                
                glEnd();
                glEnable(GL_TEXTURE_2D);
                
            }
			else
			{
                // ----------------------- add note (preview) --------------------
                
                    glDisable(GL_TEXTURE_2D);
                    glColor3f(1, 0.85, 0);
                    
                    
                    int preview_x1=
                        (int)(
                              (snapMidiTickToGrid(mousex_initial.getRelativeTo(MIDI) /*+ sequence->getXScrollInMidiTicks()*/) -
                               sequence->getXScrollInMidiTicks())*sequence->getZoom()
                              );
                    int preview_x2=
                        (int)(
                              (snapMidiTickToGrid(mousex_current.getRelativeTo(MIDI) /*+ sequence->getXScrollInMidiTicks()*/) -
                               sequence->getXScrollInMidiTicks())*sequence->getZoom()
                              );
                    
                    if(!(preview_x1<0 || preview_x2<0) and preview_x2>preview_x1)
					{
                        
						glBegin(GL_QUADS);
						
						glVertex2f(preview_x1+getEditorXStart(), ((mousey_initial - getEditorYStart() + getYScrollInPixels())/y_step)*y_step + getEditorYStart() - getYScrollInPixels());
						glVertex2f(preview_x1+getEditorXStart(), ((mousey_initial - getEditorYStart() + getYScrollInPixels())/y_step)*y_step+y_step + getEditorYStart() - getYScrollInPixels());
						glVertex2f(preview_x2+getEditorXStart(), ((mousey_initial - getEditorYStart() + getYScrollInPixels())/y_step)*y_step+y_step + getEditorYStart() - getYScrollInPixels());
						glVertex2f(preview_x2+getEditorXStart(), ((mousey_initial - getEditorYStart() + getYScrollInPixels())/y_step)*y_step + getEditorYStart() - getYScrollInPixels());
						
						glEnd();
						
                    }
                    
                    glEnable(GL_TEXTURE_2D);
            }// end if selection or addition
        }// end if dragging on track
        
    } // end if !clickedOnNote
    
    // ------------------------- move note (preview) -----------------------
    if(clickedOnNote)
    {
        
        glDisable(GL_TEXTURE_2D);
        
        glColor4f(1, 0.85, 0, 0.5);
        
        int x_difference = mousex_current.getRelativeTo(MIDI) - mousex_initial.getRelativeTo(MIDI);
        int y_difference = mousey_current - mousey_initial;
        
        const int x_step_move = (int)( snapMidiTickToGrid(x_difference) * sequence->getZoom() );
        const int y_step_move = (int)round( (float)y_difference/ (float)y_step );
            
        // move a single note
        if(lastClickedNote!=-1)
        {
            int x1=track->getNoteStartInPixels(lastClickedNote) - sequence->getXScrollInPixels();
            int x2=track->getNoteEndInPixels(lastClickedNote) - sequence->getXScrollInPixels();
            int y=track->getNotePitchID(lastClickedNote);
            
            glBegin(GL_QUADS);
            glVertex2f(x1+x_step_move+getEditorXStart(), (y+y_step_move)*y_step+1 + getEditorYStart() - getYScrollInPixels());
            glVertex2f(x1+x_step_move+getEditorXStart(), (y+y_step_move+1)*y_step + getEditorYStart() - getYScrollInPixels());
            glVertex2f(x2-1+x_step_move+getEditorXStart(), (y+y_step_move+1)*y_step + getEditorYStart() - getYScrollInPixels());
            glVertex2f(x2-1+x_step_move+getEditorXStart(), (y+y_step_move)*y_step+1 + getEditorYStart() - getYScrollInPixels());
            glEnd();
            
        }
        else
        {
            // move a bunch of notes
            
            for(int n=0; n<track->getNoteAmount(); n++)
            {
                if(!track->isNoteSelected(n)) continue;
                
                int x1=track->getNoteStartInPixels(n) - sequence->getXScrollInPixels();
                int x2=track->getNoteEndInPixels(n) - sequence->getXScrollInPixels();
                int y=track->getNotePitchID(n);
                //float volume=track->getNoteVolume(n)/127.0;
                
                glBegin(GL_QUADS);
                glVertex2f(x1+x_step_move+getEditorXStart(), (y+y_step_move)*y_step+1 + getEditorYStart() - getYScrollInPixels());
                glVertex2f(x1+x_step_move+getEditorXStart(), (y+y_step_move+1)*y_step + getEditorYStart() - getYScrollInPixels());
                glVertex2f(x2-1+x_step_move+getEditorXStart(), (y+y_step_move+1)*y_step + getEditorYStart() - getYScrollInPixels());
                glVertex2f(x2-1+x_step_move+getEditorXStart(), (y+y_step_move)*y_step+1 + getEditorYStart() - getYScrollInPixels());
                glEnd();
            }//next
            
        }
        
        glEnable(GL_TEXTURE_2D);
        glLoadIdentity();
        
    }

    // ---------------------------- scrollbar -----------------------
    if(!focus) glColor3f(0.5, 0.5, 0.5);
    else glColor3f(1,1,1);
    
    renderScrollbar();
    
    glColor3f(1,1,1);
    
    glDisable(GL_SCISSOR_TEST);
    
}


// if key is e.g. G Major, "major_note" will be set to note12 equivalent of G.
void KeyboardEditor::loadKey(const int major_note12)
{
#define NEXT n--; if(n<0) n+=12
    int n = major_note12 + 7;
    if(n > 11) n -= 12;
    
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = true; NEXT;
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = true; NEXT;
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = true; NEXT;
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = true; NEXT;
    note_greyed_out[n] = false; NEXT;
    note_greyed_out[n] = true;
#undef NEXT
    
}

}
