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

#ifndef _config_
#define _config_

#include "IO/IOUtils.h"

// uncomment this if your system doesn't like OpenGL. EXPERIMENTAL.
// #define NO_OPENGL

// values
#define DEFAULT_SONG_LENGTH 12

// -------------------- my assert stuff -----------------
#include <iostream>


#ifdef _MORE_DEBUG_CHECKS

#undef assert
#define assert(expr) if(! (expr)){assertFailed( wxT("Assert failed: ") + fromCString( #expr ) + wxT("\n@ ") + extract_filename( fromCString(__FILE__) ) + wxT(": ") + to_wxString(__LINE__));}
#define assertExpr(v1,sign,v2) if(!((v1) sign (v2))){ std::cout << "assert failed values : " << v1 << #sign << v2 << std::endl; assertFailed( wxT("Assert failed: ") + fromCString( #v1 ) + fromCString( #sign ) + fromCString( #v2 ) + wxT("\n@ ") + extract_filename(fromCString(__FILE__)) + wxT(": ") + to_wxString(__LINE__) ); }

#else

#undef assert
#define assert(expr)
#define assertExpr(v1,sign,v2)

#endif


#include "LeakCheck.h"

template<typename T>
class AutoDeletePtr
{
public:
    T* ptr;
    AutoDeletePtr()
    {
        ptr = NULL;
    }
    ~AutoDeletePtr()
    {
#ifdef _MORE_DEBUG_CHECKS
        if(ptr == NULL) std::cerr << "Warning, ptr_hold declared but not inited properly" << std::endl;
        else
#endif
            delete this->ptr;
    }
};
template<typename T>
class WxAutoDeletePtr
{
public:
    T* ptr;
    WxAutoDeletePtr()
    {
        ptr = NULL;
    }
    ~WxAutoDeletePtr()
    {
#ifdef _MORE_DEBUG_CHECKS
        if(ptr == NULL) std::cerr << "Warning, ptr_hold declared but not inited properly" << std::endl;
        else
#endif
            this->ptr->Destroy();
    }
};

#define PTR_HOLD( type, ptr_name ) type* ptr_name; AutoDeletePtr<type> ptr_name##_ptrhold;
#define WX_PTR_HOLD( type, ptr_name ) type* ptr_name; WxAutoDeletePtr<type> ptr_name##_ptrhold;
#define INIT_PTR( ptr_name ) ptr_name = ptr_name##_ptrhold . ptr



#endif
