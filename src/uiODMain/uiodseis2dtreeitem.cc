/*+
___________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	K. Tingdahl
 Date:		May 2006
___________________________________________________________________

-*/

#include "uiodseis2dtreeitem.h"

#include "uiattribpartserv.h"
#include "uiattr2dsel.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uinlapartserv.h"
#include "uiodapplmgr.h"
#include "uiodeditattribcolordlg.h"
#include "uiodscenemgr.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uitaskrunner.h"
#include "uivispartserv.h"
#include "visseis2ddisplay.h"

#include "attribdataholder.h"
#include "attribdesc.h"
#include "attribdescid.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "attribprobelayer.h"
#include "attribsel.h"
#include "emmanager.h"
#include "externalattrib.h"
#include "dbman.h"
#include "posinfo2d.h"
#include "probeimpl.h"
#include "probemanager.h"
#include "seisioobjinfo.h"
#include "seistrctr.h"
#include "seis2ddata.h"
#include "survgeom2d.h"


static TypeSet<int> selcomps;

#define cAdd		1000
#define cGridFrom3D	999
#define cFrom3D		998
#define cTo3D		997

#define cAddAttr	996
#define cRemoveAttr	993
#define cReplaceAttr	992
#define cDispAttr	991
#define cHideAttr	990
#define cEditColorSetts 989

#define cDisplayAll	988
#define cHideAll	987

uiODSceneProbeParentTreeItem::Type
	uiODLine2DParentTreeItem::getType( int action ) const
{
    switch( action )
    {
	case 0: return uiODSceneProbeParentTreeItem::Empty; break;
	case 1: return uiODSceneProbeParentTreeItem::Default; break;
	case 2: return uiODSceneProbeParentTreeItem::Select; break;
	default: return uiODSceneProbeParentTreeItem::Empty;
    }
}

uiODLine2DParentTreeItem::uiODLine2DParentTreeItem()
    : uiODSceneProbeParentTreeItem( tr("2D Line") )
    , visserv_(ODMainWin()->applMgr().visServer())
    , additm_(m3Dots(uiStrings::sAdd()),cAdd)
    , create2dgridfrom3ditm_(m3Dots(tr("Create 2D Grid from 3D")),cGridFrom3D)
    , extractfrom3ditm_(m3Dots(tr("Extract from 3D")),cFrom3D)
    , generate3dcubeitm_(m3Dots(tr("Generate 3D Cube")),cTo3D)
    , addattritm_(tr("Add Attribute"),cAddAttr)
    , removeattritm_(tr("Remove Attribute"),cRemoveAttr)
    , replaceattritm_(tr("Replace Attribute"),cReplaceAttr)
    , dispattritm_(tr("Display Attribute"),cDispAttr)
    , hideattritm_(tr("Hide Attribute"),cHideAttr)
    , editcoltabitm_(tr("Edit Color Settings"),cEditColorSetts)
    , displayallitm_(tr("Display All"),cDisplayAll)
    , hideallitm_(tr("Hide All"),cHideAll)
{
}


uiODLine2DParentTreeItem::~uiODLine2DParentTreeItem()
{
    detachAllNotifiers();
}


const char* uiODLine2DParentTreeItem::childObjTypeKey() const
{ return ProbePresentationInfo::sFactoryKey(); }


const char* uiODLine2DParentTreeItem::iconName() const
{ return "tree-geom2d"; }


bool uiODLine2DParentTreeItem::init()
{
    if ( !uiODSceneParentTreeItem::init() )
	return false;

    MenuHandler* menu = visserv_->getMenuHandler();
    mAttachCB( menu->initnotifier, uiODLine2DParentTreeItem::createMenuCB );
    mAttachCB( menu->handlenotifier, uiODLine2DParentTreeItem::handleMenuCB );
    return true;
}


bool uiODLine2DParentTreeItem::showSubMenu()
{
    return visserv_->showMenu( selectionKey(), uiMenuHandler::fromTree() );
}


int uiODLine2DParentTreeItem::selectionKey() const
{
    if ( children_.size() < 1 )
	return -1;

    mDynamicCastGet(const uiODDisplayTreeItem*,itm,children_[0]);
    return itm ? 100000+itm->displayID() : -1;
}


Probe* uiODLine2DParentTreeItem::createNewProbe() const
{
    return new Line2DProbe( geomtobeadded_ );
}


uiODPrManagedTreeItem* uiODLine2DParentTreeItem::addChildItem(
	const OD::ObjPresentationInfo& prinfo )
{
    mDynamicCastGet(const ProbePresentationInfo*,probeprinfo,&prinfo)
    if ( !probeprinfo )
	return 0;

    RefMan<Probe> probe = ProbeMGR().fetchForEdit( probeprinfo->storedID() );
    mDynamicCastGet(Line2DProbe*,l2dprobe,probe.ptr())
    if ( !l2dprobe )
	return 0;

    uiOD2DLineTreeItem* inlitem = new uiOD2DLineTreeItem( *probe );
    addChild( inlitem, false );
    return inlitem;
}


void uiODLine2DParentTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( !menu || menu->menuID()!=selectionKey() )
	return;

    createMenu( menu, false );
}


#define mAddAttrBasedItem( attritm ) \
    mAddMenuItem( menu, &attritm, true, false ); \
    attritm.removeItems(); \
    for ( int idx=0; idx<displayedattribs.size(); idx++ ) \
    attritm.addItem( \
	 new MenuItem(toUiString(displayedattribs.get(idx)),varmenuid++), \
		      true );

#define mAddDispHideAllItems( itm ) \
    mAddMenuItem( menu, &itm, true, false ); \
    itm.removeItems(); \
    itm.addItem( new MenuItem(uiStrings::sLineName(mPlural),varmenuid++),true);\
    itm.addItem( new MenuItem(uiStrings::s2DPlane(mPlural),varmenuid++),true); \
    itm.addItem( new MenuItem(uiStrings::sLineGeometry(),varmenuid++), true );


BufferStringSet uiODLine2DParentTreeItem::getDisplayedAttribNames() const
{
    BufferStringSet displayedattribs;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(const uiOD2DLineTreeItem*,l2dtreeitem,children_[idx]);
	if ( !l2dtreeitem )
	    continue;

	for ( int ich=0; ich<l2dtreeitem->nrChildren(); ich++ )
	{
	    mDynamicCastGet(const uiODAttribTreeItem*,attrtreeitem,
			    l2dtreeitem->getChild(ich));
	    if ( !attrtreeitem )
		continue;

	    BufferString attribname( attrtreeitem->name().getFullString() );
	    displayedattribs.addIfNew( attribname );
	}
    }

    return displayedattribs;
}


void uiODLine2DParentTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    mAddMenuItem( menu, &additm_, true, false );
    if ( SI().has3D() )
    {
	mAddMenuItem( menu, &create2dgridfrom3ditm_, true, false );
	mAddMenuItem( menu, &extractfrom3ditm_, true, false );
    }

#ifdef __debug__
    mAddMenuItem( menu, &generate3dcubeitm_, true, false );
#endif


    int varmenuid = 500;
    BufferStringSet displayedattribs = getDisplayedAttribNames();
    if ( !children_.isEmpty() )
    {
	mAddMenuItem( menu, &addattritm_, true, false );
	if ( !displayedattribs.isEmpty() )
	{
	    mAddAttrBasedItem( replaceattritm_ );
	    if ( displayedattribs.size()>1 )
		{ mAddAttrBasedItem( removeattritm_ ); }

	    if ( displayedattribs.size() )
	    {
		mAddAttrBasedItem( dispattritm_ );
		mAddAttrBasedItem( hideattritm_ );
		mAddAttrBasedItem( editcoltabitm_ );
	    }
	}

	mAddDispHideAllItems( displayallitm_ );
	mAddDispHideAllItems( hideallitm_ );
    }

    addStandardItems( menu );
}


//TODO PrIMPL relook into multiple line data sel
bool uiODLine2DParentTreeItem::getSelAttrSelSpec(
	Probe& probe , Attrib::SelSpec& selspec ) const
{
    if ( selattr_.id()==Attrib::SelSpec::cAttribNotSel() )
    {
	if ( !uiODSceneProbeParentTreeItem::getSelAttrSelSpec(probe,selspec) )
	    return false;

	selattr_ = selspec;
    }

    selspec = selattr_;
    return true;
}


void uiODLine2DParentTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, menuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || menu->isHandled() || menu->menuID()!=selectionKey() ||
	    menuid==-1 )
	return;

    TypeSet<Pos::GeomID> displayedgeomids;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx]);
	mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
			visserv_->getObject(itm->displayID()));
	if ( !s2d ) continue;

	displayedgeomids += s2d->getGeomID();
    }

    if ( menuid == additm_.id )
    {
	int action = 0;
	TypeSet<Pos::GeomID> geomids;
	selattr_ = Attrib::SelSpec();
	applMgr()->seisServer()->select2DLines( geomids, action );
	if ( geomids.isEmpty() )
	    return;

	typetobeadded_ = getType( action );
	MouseCursorChanger cursorchgr( MouseCursor::Wait );
	for ( int idx=geomids.size()-1; idx>=0; idx-- )
	{
	    setMoreObjectsToDoHint( idx>0 );
	    geomtobeadded_ = geomids[idx];
	    if ( !addChildProbe() )
		return;
	}
	cursorchgr.restore();
    }
    else if ( menuid == create2dgridfrom3ditm_.id )
	ODMainWin()->applMgr().create2DGrid();
    else if ( menuid == extractfrom3ditm_.id )
	ODMainWin()->applMgr().create2DFrom3D();
    else if ( menuid == addattritm_.id )
    {
	selattr_ = Attrib::SelSpec();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,itm,children_[idx]);
	    Probe* l2dprobe = itm->getProbe();
	    AttribProbeLayer* attriblay = new AttribProbeLayer();
	    Attrib::SelSpec selattrselspec;
	    if ( !getSelAttrSelSpec(*l2dprobe,selattrselspec) )
	    {
		delete attriblay;
		return;
	    }

	    attriblay->setSelSpec( selattrselspec );
	    attriblay->useStoredColTabPars();
	    l2dprobe->addLayer( attriblay );
	    uiODDataTreeItem* attrtreeitem =
		itm->createProbeLayerItem( *attriblay );
	    if ( attrtreeitem )
	    {
		itm->addChild( attrtreeitem, false );
		attrtreeitem->updateDisplay();
	    }
	}
    }
    else if ( menuid == generate3dcubeitm_.id )
	ODMainWin()->applMgr().create3DFrom2D();
    else if ( replaceattritm_.findItem(menuid) )
    {
	const MenuItem* itm = replaceattritm_.findItem( menuid );
	FixedString attrnm = itm->text.getOriginalString();
	selattr_ = Attrib::SelSpec();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitm,children_[idx]);
	    Probe* l2dprobe = lineitm->getProbe();
	    for ( int ich=0; ich<lineitm->nrChildren(); ich++ )
	    {
		mDynamicCastGet(uiODAttribTreeItem*,attrtreeitem,
				lineitm->getChild(ich));
		if ( !attrtreeitem ||
		     attrtreeitem->attribProbeLayer()->name()!=attrnm )
		    continue;

		Attrib::SelSpec selattrselspec;
		if ( !getSelAttrSelSpec(*l2dprobe,selattrselspec) )
		    return;

		AttribProbeLayer* attrlayer = attrtreeitem->attribProbeLayer();
		attrlayer->setSelSpec( selattrselspec );
	    }
	}
    }
    else if ( removeattritm_.findItem(menuid) )
    {
	const MenuItem* itm = removeattritm_.findItem( menuid );
	FixedString attrnm = itm->text.getOriginalString();
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,lineitm,children_[idx]);
	    if ( lineitm ) lineitm->removeAttrib( attrnm );
	}
    }
    else if ( dispattritm_.findItem(menuid) || hideattritm_.findItem(menuid) )
    {
	const MenuItem* itm = dispattritm_.findItem( menuid );
	const bool disp = itm;
	if ( !itm ) itm = hideattritm_.findItem( menuid );
	const FixedString attrnm = itm->text.getOriginalString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	for ( int idx=0; idx<set.size(); idx++ )
	    set[idx]->setChecked( disp, true );
    }
    else if ( editcoltabitm_.findItem(menuid) )
    {
	const MenuItem* itm = editcoltabitm_.findItem( menuid );
	const FixedString attrnm = itm->text.getOriginalString();
	ObjectSet<uiTreeItem> set;
	findChildren( attrnm, set );
	if ( set.size() )
	{
	    uiODEditAttribColorDlg dlg( ODMainWin(), set, attrnm );
	    dlg.go();
	}
    }
    else if ( displayallitm_.findItem(menuid) || hideallitm_.findItem(menuid) )
    {
	const MenuItem* itm = displayallitm_.findItem( menuid );
	const bool disp = itm;
	if ( !itm ) itm = hideallitm_.findItem( menuid );
	const int itemidx = disp ? displayallitm_.itemIndex(menuid)
				 : hideallitm_.itemIndex(menuid);
	for ( int idx=0; idx<children_.size(); idx++ )
	{
	    mDynamicCastGet(uiOD2DLineTreeItem*,treeitm,children_[idx]);
	    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
			    visserv_->getObject(treeitm->displayID()));
	    if ( !s2d ) continue;

	    switch ( itemidx )
	    {
		case 0: s2d->showLineName( disp ); break;
		case 1: s2d->showPanel( disp ); break;
		case 2: s2d->showPolyLine( disp ); break;
	    }
	}
    }
    else
	handleStandardMenuCB( cb );
}


// Line2DTreeItemFactory
uiTreeItem*
    Line2DTreeItemFactory::createForVis( int visid, uiTreeItem* treeitem ) const
{
    pErrMsg( "Deprecated , to be removed later" );
    return 0;
}


uiOD2DLineTreeItem::uiOD2DLineTreeItem( Probe& probe, int displayid )
    : uiODSceneProbeTreeItem( probe )
    , linenmitm_(tr("Show Linename"))
    , panelitm_(tr("Show 2D Plane"))
    , polylineitm_(tr("Show Line Geometry"))
    , positionitm_(m3Dots(tr("Position")))
{
    mDynamicCastGet(Line2DProbe*,l2dprobe,getProbe());
    if ( l2dprobe )
	name_ = toUiString(Survey::GM().getName( l2dprobe->geomID() ));
    displayid_ = displayid;

    positionitm_.iconfnm = "orientation64";
    linenmitm_.checkable = true;
    panelitm_.checkable = true;
    polylineitm_.checkable = true;
}


uiOD2DLineTreeItem::~uiOD2DLineTreeItem()
{
}


const char* uiOD2DLineTreeItem::parentType() const
{ return typeid(uiODLine2DParentTreeItem).name(); }


bool uiOD2DLineTreeItem::init()
{
    Probe* probe = getProbe();
    mDynamicCastGet(Line2DProbe*,l2dprobe,probe);
    if ( !probe || !l2dprobe )
    {
	pErrMsg( "Shared Object not of type Line2D Probe" );
	return false;
    }

    bool newdisplay = false;
    if ( displayid_==-1 )
    {
	mDynamicCastGet(uiODLine2DParentTreeItem*,parentitm,parent_)
	if ( !parentitm ) return false;

	visSurvey::Seis2DDisplay* s2d = new visSurvey::Seis2DDisplay;
	visserv_->addObject( s2d, sceneID(), true );
	displayid_ = s2d->id();

	s2d->turnOn( true );
	newdisplay = true;
    }

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return false;

    const Survey::Geometry* geom = Survey::GM().getGeometry(l2dprobe->geomID());
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,geom);
    if ( !geom2d )
	return false;

    s2d->setProbe( getProbe() );
    s2d->setName( toUiString(geom2d->getName()) );
    //If restore, we use the old display range after set the geometry.
    const Interval<int> oldtrcnrrg = s2d->getTraceNrRange();
    const Interval<float> oldzrg = s2d->getZRange( true );
    if ( newdisplay && (geom2d->data().positions().size() > 300000000 ||
			geom2d->data().zRange().nrSteps() > 299999999) )
    {
       uiString msg = tr("Either trace size or z size is beyond max display "
			 "size of 3 X 10 e8. You can right click the line name "
			 "to change position range to view part of the data.");
       uiMSG().warning( msg );
    }

    s2d->setGeometry( geom2d->data() );
    TrcKeyZSampling probepos = probe->position();
    if ( !newdisplay )
    {
	if ( !oldtrcnrrg.isUdf() )
	    probepos.hsamp_.setTrcRange( oldtrcnrrg );

	if ( !oldzrg.isUdf() )
	    probepos.zsamp_ = oldzrg;
    }
    else
    {
	const bool hasworkzrg = SI().zRange(true) != SI().zRange(false);
	if ( hasworkzrg )
	{
	    StepInterval<float> newzrg = geom2d->data().zRange();
	    newzrg.limitTo( SI().zRange(true) );
	    probepos.zsamp_ = newzrg;
	}
    }

    probe->setPos( probepos );
    return uiODSceneProbeTreeItem::init();
}


void uiOD2DLineTreeItem::updateDisplay()
{
    const Probe* probe = getProbe();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d || !probe ) return;

    const TrcKeyZSampling probepos = probe->position();
    s2d->setTraceNrRange( probepos.hsamp_.trcRange() );
    s2d->setZRange( probepos.zsamp_ );
}


void uiOD2DLineTreeItem::objChangedCB( CallBacker* )
{
    updateDisplay();
}


void uiOD2DLineTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !menu || menu->menuID() != displayID() || !s2d ) return;

    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &linenmitm_,
		      true, s2d->isLineNameShown() );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &panelitm_,
		      true, s2d->isPanelShown() );
    mAddMenuOrTBItem( istb, 0, &displaymnuitem_, &polylineitm_,
		      true, s2d->isPolyLineShown() );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionitm_, true, false);
}


void uiOD2DLineTreeItem::handleMenuCB( CallBacker* cb )
{
    Probe* probe = getProbe();
    mDynamicCastGet(Line2DProbe*,l2dprobe,probe);
    if ( !probe || !l2dprobe )
    {
	pErrMsg( "Shared Object not of type Line2D Probe" );
	return;
    }

    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( !menu || menu->isHandled() || mnuid==-1 )
	return;

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_));
    if ( !s2d || menu->menuID() != displayID() )
	return;

    if ( mnuid==linenmitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showLineName( !s2d->isLineNameShown() );
    }
    else if ( mnuid==panelitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showPanel( !s2d->isPanelShown() );
    }
    else if ( mnuid==polylineitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showPolyLine( !s2d->isPolyLineShown() );
    }
    else if ( mnuid==positionitm_.id )
    {
	menu->setIsHandled(true);

	TrcKeyZSampling maxcs;
	assign( maxcs.zsamp_, s2d->getMaxZRange(true)  );
	maxcs.hsamp_.start_.crl() = s2d->getMaxTraceNrRange().start;
	maxcs.hsamp_.stop_.crl() = s2d->getMaxTraceNrRange().stop;

	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()))
	CallBack dummy;
	TrcKeyZSampling probepos = probe->position();
	uiSliceSelDlg positiondlg( getUiParent(), probepos,
				   maxcs, dummy, uiSliceSel::TwoD,
				   scene->zDomainInfo() );
	if ( !positiondlg.go() ) return;
	const TrcKeyZSampling newcs = positiondlg.getTrcKeyZSampling();
	probe->setPos( newcs );

	updateColumnText(0);
    }
}


void uiOD2DLineTreeItem::showLineName( bool yn )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( s2d ) s2d->showLineName( yn );
}


void uiOD2DLineTreeItem::setZRange( const Interval<float> newzrg )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv_->getObject(displayid_))
    if ( !s2d ) return;

    s2d->annotateNextUpdateStage( true );
    s2d->setZRange( newzrg );
    s2d->annotateNextUpdateStage( true );
    for ( int idx=0; idx<s2d->nrAttribs(); idx++ )
    {
	if ( s2d->getSelSpec(idx) && s2d->getSelSpec(idx)->id().isValid() )
	    visserv_->calculateAttrib( displayid_, idx, false );
    }
    s2d->annotateNextUpdateStage( false );
}


void uiOD2DLineTreeItem::removeAttrib( const char* attribnm )
{
    BufferString itemnm = attribnm;
    int nrattribitms = 0;
    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiOD2DLineAttribTreeItem*,item,children_[idx]);
	if ( item ) nrattribitms++;
    }

    for ( int idx=0; idx<children_.size(); idx++ )
    {
	mDynamicCastGet(uiODDataTreeItem*,dataitem,children_[idx]);
	mDynamicCastGet(uiOD2DLineAttribTreeItem*,attribitem,children_[idx]);
	if ( !dataitem || itemnm!=dataitem->name().getFullString() ) continue;

	if ( attribitem && nrattribitms<=1 )
	{
	    attribitem->clearAttrib();
	    return;
	}

	dataitem->prepareForShutdown();
	removeChild( dataitem );
	idx--;
    }
}


uiOD2DLineAttribTreeItem::uiOD2DLineAttribTreeItem( const char* pt )
    : uiODAttribTreeItem( pt )
{}


uiODDataTreeItem* uiOD2DLineAttribTreeItem::create( ProbeLayer& prblay )
{
    const char* parenttype = typeid(uiOD2DLineTreeItem).name();
    uiOD2DLineAttribTreeItem* attribtreeitem =
	new uiOD2DLineAttribTreeItem( parenttype );
    attribtreeitem->setProbeLayer( &prblay );
    return attribtreeitem;

}


void uiOD2DLineAttribTreeItem::initClass()
{
    uiODDataTreeItem::fac().addCreateFunc(
	    create, AttribProbeLayer::sFactoryKey(),
	    Line2DProbe::sFactoryKey() );

}


void uiOD2DLineAttribTreeItem::updateDisplay()
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    applMgr()->visServer()->getObject( displayID() ));
    if ( !s2d )
	return;

    s2d->showPanel( true );
    uiODAttribTreeItem::updateDisplay();
}


void uiOD2DLineAttribTreeItem::clearAttrib()
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    applMgr()->visServer()->getObject( displayID() ));
    if ( !s2d )
	return;

    s2d->clearTexture( attribNr() );
    updateColumnText(0);
    applMgr()->updateColorTable( displayID(), attribNr() );
}
