#ifdef RENDERER_WXWIDGETS

#include "wxDCString.h"
#include "Utils.h"

#include <wx/dc.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include "AriaCore.h"
#include "PreferencesData.h"
#include "Renderers/RenderAPI.h"

namespace AriaMaestosa
{
    
#if 0
#pragma mark -
#pragma mark wxDCString
#endif
    

    wxDCString::wxDCString(Model<wxString>* model, bool own)
    {
        m_model = model;
        m_consolidated = false;
        m_max_width = -1;
        m_warp = false;
        m_w = -1;
        m_h = -1;
        
        if (not own) m_model.owner = false;
        
        model->setListener(this);
    }
    
    wxDCString::~wxDCString()
    {
    }
    
    
    void wxDCString::setMaxWidth(const int w, const bool warp)
    {
        m_max_width = w;
        m_warp = warp;
    }
    
    void wxDCString::setFont(const wxFont& font)
    {
        m_font = font;
    }
    
    void wxDCString::bind()
    {
        if (not m_consolidated)
        {
            if (m_font.IsOk()) Display::renderDC->SetFont(m_font);
            else               Display::renderDC->SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
            
            Display::renderDC->GetTextExtent(m_model->getValue(), &m_w, &m_h);

            m_consolidated = true;
        }
    }
    
    int wxDCString::getWidth()
    {
        ASSERT_E(m_w, >=, 0);
        ASSERT_E(m_w, <, 90000);
        return m_w;
    }
    
    int wxDCString::getHeight()
    {
        ASSERT_E(m_h, >=, 0);
        ASSERT_E(m_h, <, 90000);
        return m_h;
    }
    
    void wxDCString::scale(float f)
    {
    }
    void wxDCString::rotate(int angle)
    {
    }
    
    void wxDCString::render(const int x, const int y)
    {
        if (not m_consolidated) bind();
            
        ASSERT_E(m_h, >=, 0);
        ASSERT_E(m_h, <, 90000);
        ASSERT(m_consolidated);
        
        if (m_font.IsOk()) Display::renderDC->SetFont(m_font);
        else               Display::renderDC->SetFont(wxSystemSettings::GetFont(wxSYS_SYSTEM_FONT));
        
        if (m_max_width != -1 and getWidth() > m_max_width)
        {
            if (not m_warp)
            {
                wxString shortened = m_model->getValue();
                
                while (Display::renderDC->GetTextExtent(shortened).GetWidth() > m_max_width)
                {
                    shortened = shortened.Truncate(shortened.size()-1);
                }
                Display::renderDC->DrawText(shortened, x, y - m_h);
            }
            else // wrap
            {
                int my_y = y - m_h;
                wxString multiline = m_model->getValue();
                multiline.Replace(wxT(" "),wxT("\n"));
                multiline.Replace(wxT("/"),wxT("/\n"));

                wxStringTokenizer tkz(multiline, wxT("\n"));
                while ( tkz.HasMoreTokens() )
                {
                    wxString token = tkz.GetNextToken();
                    Display::renderDC->DrawText(token, x, my_y);
                    my_y += m_h;
                }
            }
        }
        else
        {
            Display::renderDC->DrawText(m_model->getValue(), x, y - m_h);
        }
    }
    
    
    
    
#if 0
#pragma mark -
#pragma mark wxDCNumberRenderer
#endif
    
    DEFINE_SINGLETON( wxDCNumberRenderer );
    
    wxDCNumberRenderer::wxDCNumberRenderer()
    {
        m_consolidated = false;
        m_w = -1;
        m_h = -1;
    }
    wxDCNumberRenderer::~wxDCNumberRenderer(){}
    
    void wxDCNumberRenderer::bind()
    {
        if (not m_consolidated)
        {
            Display::renderDC->SetFont(getNumberFont());
            Display::renderDC->GetTextExtent(wxT("0"), &m_w, &m_h);
            
            m_consolidated = true;
        }
    }
    
    void wxDCNumberRenderer::renderNumber(const wxString& s, int x, int y)
    {
        ASSERT_E(m_h, >, -1);
        ASSERT_E(m_h, <, 90000);

        Display::renderDC->SetFont( getNumberFont() );
        Display::renderDC->DrawText(s, x, y - m_h);
    }
    
    void wxDCNumberRenderer::renderNumber(int i, int x, int y)
    {
        renderNumber( to_wxString(i), x, y );
    }
    void wxDCNumberRenderer::renderNumber(float f, int x, int y)
    {
        renderNumber( to_wxString(f), x, y );
    }
    
    
#if 0
#pragma mark -
#pragma mark wxDCStringArray
#endif
    
    wxDCStringArray::wxDCStringArray()
    {
        m_consolidated = false;
    }
    wxDCStringArray::wxDCStringArray(const wxString strings_arg[], int amount)
    {
        addStrings(strings_arg, amount);
    }
    
    wxDCStringArray::~wxDCStringArray(){}
    
    wxDCString& wxDCStringArray::get(const int id)
    {
        return m_strings[id];
    }

    void wxDCStringArray::addStrings(const wxString strings_arg[], int amount)
    {
        for (int n=0; n<amount; n++)
        {
            m_strings.push_back( new wxDCString( new Model<wxString>(strings_arg[n]), true ) );
        }
        m_consolidated = false;
    }


    void wxDCStringArray::addString(const wxString& newstring)
    {
        m_strings.push_back( new wxDCString( new Model<wxString>(newstring), true ) );
    }
    
    void wxDCStringArray::bind()
    {
        if (not m_consolidated)
        {
            if (not m_font.IsOk()) return;
            
            const int amount = m_strings.size();
            for (int n=0; n<amount; n++)
            {
                m_strings[n].setFont(m_font);
            }
            
            m_consolidated = true;
        }
    }
    
    void wxDCStringArray::setFont(const wxFont& font)
    {
        m_font = font;
        m_consolidated = false;
    }
    

}
#endif
