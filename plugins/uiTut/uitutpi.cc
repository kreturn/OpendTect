
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : NOv 2003
-*/

static const char* rcsID = "$Id: uitutpi.cc,v 1.3 2007-05-11 12:55:07 cvsbert Exp $";

#include "uitutseistools.h"
#include "uiodmenumgr.h"
#include "uimenu.h"
#include "uimsg.h"
#include "plugins.h"

extern "C" int GetuiTutPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiTutPluginInfo()
{
    static PluginInfo retpi = {
	"Tutorial plugin development",
	"dGB (Raman/Bert)",
	"3.0",
    	"Shows some simple plugin basics." };
    return &retpi;
}


class uiTutMgr :  public CallBacker
{
public:
			uiTutMgr(uiODMain*);

    uiODMain*		appl;

    void		doDirSeis(CallBacker*);
    void		doAttrSeis(CallBacker*);
    void		doHor(CallBacker*);
};


uiTutMgr::uiTutMgr( uiODMain* a )
	: appl(a)
{
    uiODMenuMgr& mnumgr = appl->menuMgr();
    uiPopupMenu* mnu = new uiPopupMenu( appl, "&Tut Tools" );
    mnu->insertItem( new uiMenuItem("&Seismic (Direct) ...",
			mCB(this,uiTutMgr,doDirSeis)) );
    mnu->insertItem( new uiMenuItem("Seismic (&Attribute-based) ...",
			mCB(this,uiTutMgr,doAttrSeis)) );
    mnu->insertItem( new uiMenuItem("&Horizon ...",
			mCB(this,uiTutMgr,doHor)) );
    mnumgr.utilMnu()->insertItem( mnu );
}


void uiTutMgr::doDirSeis( CallBacker* )
{
    uiTutSeisTools dlg( appl );
    dlg.go();
}


void uiTutMgr::doAttrSeis( CallBacker* )
{
    uiMSG().message( "Attribute-based not yet implemented" );
}


void uiTutMgr::doHor( CallBacker* )
{
    uiMSG().message( "Horizontools not yet implemented" );
}


extern "C" const char* InituiTutPlugin( int, char** )
{
    static uiTutMgr* mgr = 0; if ( mgr ) return 0;
    mgr = new uiTutMgr( ODMainWin() );
    return 0;
}
