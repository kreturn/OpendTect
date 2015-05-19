/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Oct 1999
-*/

static const char* rcsID mUsedVar = "$Id$";


#include "uiodvolrentreeitem.h"

#include "uiattribpartserv.h"
#include "uifiledlg.h"
#include "uimenu.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodattribtreeitem.h"
#include "uiodscenemgr.h"
#include "uiodbodydisplaytreeitem.h"
#include "uislicesel.h"
#include "uistatsdisplay.h"
#include "uistatsdisplaywin.h"
#include "uiseisamplspectrum.h"
#include "uistrings.h"
#include "uitreeview.h"
#include "uiviscoltabed.h"
#include "uivisisosurface.h"
#include "uivispartserv.h"
#include "uivisslicepos3d.h"
#include "vismarchingcubessurface.h"
#include "vismarchingcubessurfacedisplay.h"
#include "visrgbatexturechannel2rgba.h"
#include "visvolorthoslice.h"
#include "visvolumedisplay.h"

#include "filepath.h"
#include "ioobj.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "objdisposer.h"
#include "oddirs.h"
#include "settingsaccess.h"
#include "survinfo.h"
#include "zaxistransform.h"
#include "od_helpids.h"

#define mAddIdx		0
#define mAddCBIdx	1


/* OSG-TODO: Port VolrenDisplay and OrthogonalSlice occurences to OSG
   if these classes are prolongated. */


uiODVolrenParentTreeItem::uiODVolrenParentTreeItem()
    : uiTreeItem("Volume")
{
    //Check if there are any volumes already in the scene
}


uiODVolrenParentTreeItem::~uiODVolrenParentTreeItem()
{}


bool uiODVolrenParentTreeItem::canAddVolumeToScene()
{
    if ( SettingsAccess().doesUserWantShading(true) &&
	 visSurvey::VolumeDisplay::canUseVolRenShading() )
	return true;

    for ( int idx=0; idx<nrChildren(); idx++ )
    {
	mDynamicCastGet( uiODDisplayTreeItem*, itm, getChild(idx) );
	const int displayid = itm ? itm->displayID() : -1;
	mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		    ODMainWin()->applMgr().visServer()->getObject(displayid) );

	if ( vd && !vd->usesShading() )
	{
	    uiMSG().message(
		tr( "Can only display one fixed-function volume per scene.\n"
		    "If available, enabling OpenGL shading for volumes\n"
		    "in the 'Look and Feel' settings may help." ) );

	    return false;
	}
    }
    return true;
}


bool uiODVolrenParentTreeItem::showSubMenu()
{
    uiMenu mnu( getUiParent(), uiStrings::sAction() );
    mnu.insertItem( new uiAction(uiStrings::sAdd(true)), mAddIdx );
    mnu.insertItem( new uiAction(uiStrings::sAddColBlend()), mAddCBIdx );
    const int mnuid = mnu.exec();
    if ( mnuid==mAddIdx || mnuid==mAddCBIdx )
    {
	if ( canAddVolumeToScene() )
	    addChild( new uiODVolrenTreeItem(-1,mnuid==mAddCBIdx), false );
    }

    return true;
}


int uiODVolrenParentTreeItem::sceneID() const
{
    int sceneid;
    if ( !getProperty<int>( uiODTreeTop::sceneidkey(), sceneid ) )
	return -1;
    return sceneid;
}


bool uiODVolrenParentTreeItem::init()
{
    return uiTreeItem::init();
}


const char* uiODVolrenParentTreeItem::parentType() const
{ return typeid(uiODTreeTop).name(); }



uiTreeItem*
    uiODVolrenTreeItemFactory::createForVis( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    ODMainWin()->applMgr().visServer()->getObject(visid) );
    if ( vd )
    {
	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 vd->getChannels2RGBA() );
	return new uiODVolrenTreeItem( visid, rgba );
    }

    return 0;
}


const char* uiODVolrenTreeItemFactory::getName()
{ return typeid(uiODVolrenTreeItemFactory).name(); }



uiODVolrenTreeItem::uiODVolrenTreeItem( int displayid, bool rgba )
    : uiODDisplayTreeItem()
    , positionmnuitem_(tr("Position ..."))
    , rgba_(rgba)
{
    positionmnuitem_.iconfnm = "orientation64";
    displayid_ = displayid;
}


uiODVolrenTreeItem::~uiODVolrenTreeItem()
{
    uitreeviewitem_->stateChanged.remove(
				mCB(this,uiODVolrenTreeItem,checkCB) );
    while ( children_.size() )
	removeChild(children_[0]);

    visserv_->removeObject( displayid_, sceneID() );
}


bool uiODVolrenTreeItem::showSubMenu()
{
    return visserv_->showMenu( displayid_, uiMenuHandler::fromTree() );
}


const char* uiODVolrenTreeItem::parentType() const
{ return typeid(uiODVolrenParentTreeItem).name(); }


bool uiODVolrenTreeItem::init()
{
    visSurvey::VolumeDisplay* voldisp;
    if ( displayid_==-1 )
    {
	voldisp = new visSurvey::VolumeDisplay;
	visserv_->addObject( voldisp, sceneID(), true );
	displayid_ = voldisp->id();
    }
    else
    {
	mDynamicCast( visSurvey::VolumeDisplay*, voldisp,
		      visserv_->getObject(displayid_) );
	if ( !voldisp ) return false;
    }

    if ( rgba_ )
    {
	mDynamicCastGet( visBase::RGBATextureChannel2RGBA*, rgba,
			 voldisp->getChannels2RGBA() );
	if ( !rgba )
	{
	    if ( voldisp->setChannels2RGBA(
				visBase::RGBATextureChannel2RGBA::create()) )
	    {
		voldisp->addAttrib();
		voldisp->addAttrib();
		voldisp->addAttrib();
	    }
	    else
		return false;
	}
    }

    return uiODDisplayTreeItem::init();
}


BufferString uiODVolrenTreeItem::createDisplayName() const
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject( displayid_ ) )
    BufferString info;
    if ( vd )
	vd->getTreeObjectInfo( info );

    return info;
}


uiODDataTreeItem*
	uiODVolrenTreeItem::createAttribItem( const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiODDataTreeItem::factory().create( 0, *as, parenttype, false) : 0;

    if ( !res )
	res = new uiODVolrenAttribTreeItem( parenttype );

    return res;
}


void uiODVolrenTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() ) return;

    const bool islocked = visserv_->isLocked( displayID() );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &positionmnuitem_,
		      !islocked, false );
}


void uiODVolrenTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    MenuHandler* menu = dynamic_cast<MenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() ||
	 menu->menuID() != displayID() )
	return;

    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject( displayid_ ) )

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled( true );
	TrcKeyZSampling maxcs = SI().sampling( true );
	mDynamicCastGet(visSurvey::Scene*,scene,visserv_->getObject(sceneID()));
	if ( scene && scene->getZAxisTransform() )
	    maxcs = scene->getTrcKeyZSampling();

	CallBack dummycb;
	uiSliceSelDlg dlg( getUiParent(), vd->getTrcKeyZSampling(true,true,-1),
			   maxcs, dummycb, uiSliceSel::Vol,
			   scene->zDomainInfo() );
	if ( !dlg.go() ) return;
	TrcKeyZSampling cs = dlg.getTrcKeyZSampling();
	vd->setTrcKeyZSampling( cs );
	visserv_->calculateAttrib( displayid_, 0, false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


uiODVolrenAttribTreeItem::uiODVolrenAttribTreeItem( const char* ptype )
    : uiODAttribTreeItem( ptype )
    , addmnuitem_(uiStrings::sAdd(true))
    , statisticsmnuitem_(uiStrings::sHistogram(false))
    , amplspectrummnuitem_( tr("Amplitude Spectrum ..."))
    , addisosurfacemnuitem_(tr("Iso Surface"))
{
    statisticsmnuitem_.iconfnm = "histogram";
    amplspectrummnuitem_.iconfnm = "amplspectrum";
}


void uiODVolrenAttribTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODAttribTreeItem::createMenu( menu, istb );

    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &statisticsmnuitem_,
		      true, false );
    mAddMenuOrTBItem( istb, menu, &displaymnuitem_, &amplspectrummnuitem_,
		      true, false );
    if ( !istb )
    {
	mAddMenuItem( menu, &displaymnuitem_, true, false );
	mAddMenuItem( &displaymnuitem_, &addmnuitem_, true, false );
	mAddMenuItem( &addmnuitem_, &addisosurfacemnuitem_, true, false );
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(), attribNr() );
    displaymnuitem_.enabled =
		    as->id().asInt()!=Attrib::SelSpec::cAttribNotSel().asInt();
}


void uiODVolrenAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( !menu || mnuid==-1 )
	return;

    if ( menu->isHandled() )
    {
	if ( parent_ )
	    parent_->updateColumnText( uiODSceneMgr::cNameColumn() );
	return;
    }

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv->getObject( displayID() ) )

    if ( mnuid==statisticsmnuitem_.id )
    {
	const DataPack::ID dpid =
			    visserv->getDataPackID( displayID(), attribNr() );
	const DataPackMgr::ID dmid = visserv->getDataPackMgrID( displayID() );
	uiStatsDisplay::Setup su; su.countinplot( false );
	uiStatsDisplayWin* dwin =
	    new uiStatsDisplayWin( applMgr()->applService().parent(),
				   su, 1, false );
	dwin->statsDisplay()->setDataPackID( dpid, dmid );
	dwin->setDataName( DPM(dmid).nameOf(dpid)  );
	dwin->windowClosed.notify( mCB(OBJDISP(),ObjDisposer,go) );
	dwin->show();
        menu->setIsHandled( true );
    }
    else if ( mnuid==amplspectrummnuitem_.id )
    {
	const DataPack::ID dpid =
			    visserv->getDataPackID( displayID(), attribNr() );
	const DataPackMgr::ID dmid = visserv->getDataPackMgrID( displayID() );
	uiSeisAmplSpectrum* asd = new uiSeisAmplSpectrum(
				  applMgr()->applService().parent() );
	asd->setDataPackID( dpid, dmid );
	asd->windowClosed.notify( mCB(OBJDISP(),ObjDisposer,go) );
	asd->show();
	menu->setIsHandled( true );
    }
    else if ( mnuid==addisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	const int surfobjid = vd->addIsoSurface( 0, false );
	const int surfidx = vd->getNrIsoSurfaces()-1;
	visBase::MarchingCubesSurface* mcs = vd->getIsoSurface(surfidx);
	uiSingleGroupDlg dlg( applMgr()->applService().parent(),
		uiDialog::Setup( tr("Iso value selection"), 0,
                                mODHelpKey(mVolrenTreeItemHelpID) ) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg(&dlg,mcs,vd,attribNr()) );
	if ( dlg.go() )
	    addChild( new uiODVolrenSubTreeItem(surfobjid), true );
    }
}


bool uiODVolrenAttribTreeItem::hasTransparencyMenu() const
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		ODMainWin()->applMgr().visServer()->getObject(displayID()) );

    return vd && vd->usesShading();
}


uiODVolrenSubTreeItem::uiODVolrenSubTreeItem( int displayid )
    : resetisosurfacemnuitem_(uiStrings::sSettings(true))
    , convertisotobodymnuitem_(tr("Convert to Body"))
{ displayid_ = displayid; }


uiODVolrenSubTreeItem::~uiODVolrenSubTreeItem()
{
    mDynamicCastGet( visSurvey::VolumeDisplay*, vd,
		     visserv_->getObject(getParentDisplayID()));

    if ( !vd ) return;

    if ( displayid_==vd->volRenID() )
	vd->showVolRen(false);
    else
	vd->removeChild( displayid_ );

    visserv_->getUiSlicePos()->setDisplay( -1 );
}


int uiODVolrenSubTreeItem::getParentDisplayID() const
{
    mDynamicCastGet( uiODDataTreeItem*, datatreeitem, parent_ );
    return datatreeitem ? datatreeitem->displayID() : -1;
}


int uiODVolrenSubTreeItem::getParentAttribNr() const
{
    mDynamicCastGet( uiODDataTreeItem*, datatreeitem, parent_ );
    return datatreeitem ? datatreeitem->attribNr() : -1;
}


bool uiODVolrenSubTreeItem::isIsoSurface() const
{
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    return isosurface;
}


const char* uiODVolrenSubTreeItem::parentType() const
{ return typeid(uiODVolrenAttribTreeItem).name(); }


bool uiODVolrenSubTreeItem::init()
{
    if ( displayid_==-1 ) return false;

//    mDynamicCastGet(visBase::VolrenDisplay*,volren,
//		    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
		    visserv_->getObject(displayid_));
    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    if ( /*!volren && */ !slice && !isosurface )
	return false;

    if ( slice )
    {
	slice->setSelectable( true );
	slice->deSelection()->notify( mCB(this,uiODVolrenSubTreeItem,selChgCB));

	mAttachCB( *slice->selection(), uiODVolrenSubTreeItem::selChgCB );
	mAttachCB( *slice->deSelection(), uiODVolrenSubTreeItem::selChgCB);
	mAttachCB( visserv_->getUiSlicePos()->positionChg,
		  uiODVolrenSubTreeItem::posChangeCB);
    }

    return uiODDisplayTreeItem::init();
}


void uiODVolrenSubTreeItem::updateColumnText(int col)
{
    if ( col!=1 )
    {
	uiODDisplayTreeItem::updateColumnText(col);
	return;
    }

    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    visserv_->getObject(getParentDisplayID()))
    if ( !vd ) return;

    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
		    visserv_->getObject(displayid_));
    if ( slice )
    {
	float dispval = vd->slicePosition( slice );
	if ( slice->getDim() == 0 )
	{
	    mDynamicCastGet(visSurvey::Scene*,scene,
		ODMainWin()->applMgr().visServer()->getObject(sceneID()));
	    dispval *= scene->zDomainUserFactor();
	}

        uitreeviewitem_->setText( toString(mNINT32(dispval)), col );
    }

    mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
		    visserv_->getObject(displayid_));
    if ( isosurface && isosurface->getSurface() )
    {
	const float isoval = vd->isoValue(isosurface);
	BufferString coltext;
        if ( mIsUdf(isoval) )
	    coltext = "";
	else coltext = isoval;
	uitreeviewitem_->setText( coltext.buf(), col );
    }
}


void uiODVolrenSubTreeItem::createMenu( MenuHandler* menu, bool istb )
{
    uiODDisplayTreeItem::createMenu( menu, istb );
    if ( !menu || menu->menuID()!=displayID() || istb ) return;

    if ( !isIsoSurface() )
    {
	mResetMenuItem( &resetisosurfacemnuitem_ );
	mResetMenuItem( &convertisotobodymnuitem_ );
	return;
    }

    mAddMenuItem( menu, &resetisosurfacemnuitem_, true, false );
    mAddMenuItem( menu, &convertisotobodymnuitem_, true, false );
}


void uiODVolrenSubTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    MenuHandler* menu = dynamic_cast<MenuHandler*>(caller);
    if ( !menu || mnuid==-1 || menu->isHandled() ||
	 menu->menuID() != displayID() )
	return;

    if ( mnuid==resetisosurfacemnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(getParentDisplayID()));

	uiSingleGroupDlg dlg( getUiParent(),
		uiDialog::Setup( tr("Iso Value Selection"), 0, mNoHelpKey ) );
	dlg.setGroup( new uiVisIsoSurfaceThresholdDlg(&dlg, isosurface, vd,
						      getParentAttribNr()) );
	if ( dlg.go() )
	    updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid==convertisotobodymnuitem_.id )
    {
	menu->setIsHandled( true );
	mDynamicCastGet(visBase::MarchingCubesSurface*,isosurface,
			visserv_->getObject(displayid_));
	mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
			visserv_->getObject(getParentDisplayID()));

	isosurface->ref();

	RefMan<visSurvey::MarchingCubesDisplay> mcdisplay =
	    new visSurvey::MarchingCubesDisplay;

	BufferString newname = "Iso ";
	newname += vd->isoValue( isosurface );
	mcdisplay->setName( newname.buf() );

	if ( !mcdisplay->setVisSurface( isosurface ) )
	{
	    isosurface->unRef();
	    return; //TODO error msg.
	}

	visserv_->addObject( mcdisplay, sceneID(), true );
	addChild( new uiODBodyDisplayTreeItem(mcdisplay->id(),true), false );
	prepareForShutdown();
	vd->removeChild( isosurface->id() );
	isosurface->unRef();

	parent_->removeChild( this );
    }
}


void uiODVolrenSubTreeItem::posChangeCB( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::VolumeDisplay*,vd,
		    visserv_->getObject(getParentDisplayID()));
    mDynamicCastGet(visBase::OrthogonalSlice*,slice,
		    visserv_->getObject(displayid_));
    if ( !slice || !vd || !vd->getSelectedSlice() ) return;

    uiSlicePos3DDisp* slicepos = visserv_->getUiSlicePos();
    if ( slicepos->getDisplayID() != getParentDisplayID() ||
	    vd->getSelectedSlice()->id() != displayid_ )
	return;

    vd->setSlicePosition( slice, slicepos->getTrcKeyZSampling() );
}


void uiODVolrenSubTreeItem::selChgCB( CallBacker* cb )
{
    visserv_->getUiSlicePos()->setDisplay( getParentDisplayID() );
}
