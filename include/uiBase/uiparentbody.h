#ifndef uiparentbody_h
#define uiparentbody_h

/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        A.H. Lammertink
 Date:          21/06/2001
 RCS:           $Id: uiparentbody.h,v 1.3 2001-09-13 14:30:39 arend Exp $
________________________________________________________________________

-*/

#include "uiparent.h"
#include "uilayout.h"
#include "sets.h"
#include "uiobj.h"
#include "uibody.h"

#include "uigroup.h"

class uiParentBody : public uiBody
{
friend class uiObjectBody;
public:
				uiParentBody()
				    : finalised( false )
				{}

    virtual			~uiParentBody()		{ deepErase(children);}

    void			addChild( uiObjHandle& child )
				    { 
					if( children.indexOf(&child ) < 0 )
					    children += &child; 
				    }

				//! child becomes mine.
    void			manageChld( uiObjHandle& child, uiObjectBody& b)
				{
				    addChild( child );
				    manageChld_(child,b);
				}

    virtual void		attachChild ( constraintType tp, 
					      uiObject* child, 
					      uiObject* other, int margin ) =0;

    virtual int			minTextWidgetHeight() const 	{ return 30; }
    virtual void 		setMinTextWidgetHeight(int h=10) const {}

    virtual void 		finalise()		{ finaliseChildren(); }
    void      			finaliseChildren();	// body: uiobj.cc
    void      			clearChildren();	// body: uiobj.cc


protected:

    virtual void		manageChld_(uiObjHandle&, uiObjectBody& ){}

    ObjectSet<uiObjHandle>		children;

private:

    bool finalised;
};

#endif
