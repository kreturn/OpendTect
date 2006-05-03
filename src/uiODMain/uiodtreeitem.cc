/*+
___________________________________________________________________

 CopyRight: 	(C) dGB Beheer B.V.
 Author: 	K. Tingdahl
 Date: 		Jul 2003
 RCS:		$Id: uiodtreeitem.cc,v 1.187 2006-05-03 18:54:19 cvskris Exp $
___________________________________________________________________

-*/

#include "uiodtreeitemimpl.h"

#include "attribsel.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdataholder.h"
#include "seisinfo.h"
#include "errh.h"
#include "emhorizon.h"
#include "emfault.h"
#include "ptrman.h"
#include "oddirs.h"
#include "ioobj.h"
#include "ioman.h"
#include "linekey.h"
#include "uimenu.h"
#include "pickset.h"
#include "pixmap.h"
#include "settings.h"
#include "colortab.h"
#include "survinfo.h"
#include "keystrs.h"
#include "segposinfo.h"
#include "zaxistransform.h"

#include "uiattribpartserv.h"
#include "uibinidtable.h"
#include "uiempartserv.h"
#include "uiexecutor.h"
#include "uigeninput.h"
#include "uigeninputdlg.h"
#include "uilistview.h"
#include "uimenuhandler.h"
#include "uimsg.h"
#include "uiodapplmgr.h"
#include "uiodscenemgr.h"
#include "uisoviewer.h"
#include "uivisemobj.h"
#include "uivispartserv.h"
#include "uiwellattribpartserv.h"
#include "uiwellpartserv.h"
#include "uiwellpropdlg.h"
#include "uipickpartserv.h"
#include "uimpepartserv.h"
#include "uiscenepropdlg.h"
#include "uiseispartserv.h"
#include "uislicesel.h"
#include "uipickszdlg.h"
#include "uicolor.h"
#include "uicursor.h"
#include "uigridlinesdlg.h"

#include "visseis2ddisplay.h"
#include "visrandomtrackdisplay.h"
#include "viswelldisplay.h"
#include "vispicksetdisplay.h"
#include "visemobjdisplay.h"
#include "vissurvscene.h"
#include "visplanedatadisplay.h"
#include "viscolortab.h"
#include "viscolorseq.h"
#include "visdataman.h"
#include "visgridlines.h"


const char* uiODTreeTop::sceneidkey = "Sceneid";
const char* uiODTreeTop::viewerptr = "Viewer";
const char* uiODTreeTop::applmgrstr = "Applmgr";
const char* uiODTreeTop::scenestr = "Scene";


uiODTreeTop::uiODTreeTop( uiSoViewer* sovwr, uiListView* lv, uiODApplMgr* am,
			    uiTreeFactorySet* tfs_ )
    : uiTreeTopItem(lv)
    , tfs(tfs_)
{
    setProperty<int>( sceneidkey, sovwr->sceneID() );
    setPropertyPtr( viewerptr, sovwr );
    setPropertyPtr( applmgrstr, am );

    tfs->addnotifier.notify( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.notify( mCB(this,uiODTreeTop,removeFactoryCB) );
}


uiODTreeTop::~uiODTreeTop()
{
    tfs->addnotifier.remove( mCB(this,uiODTreeTop,addFactoryCB) );
    tfs->removenotifier.remove( mCB(this,uiODTreeTop,removeFactoryCB) );
}


int uiODTreeTop::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( sceneidkey, sceneid );
    return sceneid;
}


bool uiODTreeTop::select(int selkey)
{
    applMgr()->visServer()->setSelObjectId(selkey);
    return true;
}


uiODApplMgr* uiODTreeTop::applMgr()
{
    void* res = 0;
    getPropertyPtr( applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


TypeSet<int> uiODTreeTop::getDisplayIds( int& selectedid, bool usechecked )
{
    TypeSet<int> dispids;
    loopOverChildrenIds( dispids, selectedid, usechecked, children );
    return dispids;
}


void uiODTreeTop::loopOverChildrenIds( TypeSet<int>& dispids, int& selectedid,
				       bool usechecked,
				    const ObjectSet<uiTreeItem>& childrenlist )
{
    for ( int idx=0; idx<childrenlist.size(); idx++ )
	loopOverChildrenIds( dispids, selectedid,
			     usechecked, childrenlist[idx]->children );

    for ( int idy=0; idy<childrenlist.size(); idy++ )
    {
	mDynamicCastGet(uiODDisplayTreeItem*,disptreeitem,childrenlist[idy]);
	if ( disptreeitem )
	{
	    if ( usechecked && childrenlist[idy]->uilistviewitem->isChecked() )
		dispids += disptreeitem->displayID();
	    else if ( !usechecked )
		dispids += disptreeitem->displayID();

	    if ( childrenlist[idy]->uilistviewitem->isSelected() )
		selectedid = disptreeitem->displayID();
	}
    }
}


// ***** uiODTreeItem

uiODTreeItem::uiODTreeItem( const char* name__ )
    : uiTreeItem( name__ )
{}


uiODApplMgr* uiODTreeItem::applMgr()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiSoViewer* uiODTreeItem::viewer()
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr, res );
    return reinterpret_cast<uiSoViewer*>( res );
}


int uiODTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey, sceneid );
    return sceneid;
}


void uiODTreeItem::addStandardItems( uiPopupMenu& mnu )
{
    if ( children.size() < 2 ) return;

    mnu.insertSeparator( 100 );
    mnu.insertItem( new uiMenuItem("Show all items"), 101 );
    mnu.insertItem( new uiMenuItem("Hide all items"), 102 );
    mnu.insertItem( new uiMenuItem("Remove all items"), 103 );
}


void uiODTreeItem::handleStandardItems( int mnuid )
{
    for ( int idx=0; idx<children.size(); idx++ )
    {
	if ( mnuid == 101 )
	    children[idx]->setChecked( true );
	else if ( mnuid == 102 )
	    children[idx]->setChecked( false );
    }

    if ( mnuid==103 )
    {
	BufferString msg( "All " ); msg += name(); 
	msg += " items will be removed.\nDo you want to continue?";
	if ( !uiMSG().askGoOn(msg) ) return;

	while ( children.size() )
	{
	    mDynamicCastGet(uiODDisplayTreeItem*,itm,children[0])
	    if ( !itm ) continue;
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
    }
}


void uiODTreeTop::addFactoryCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,idx,cb);
    addChild( tfs->getFactory(idx)->create(), false );
}


void uiODTreeTop::removeFactoryCB(CallBacker* cb)
{
    mCBCapsuleUnpack(int,idx,cb);
    PtrMan<uiTreeItem> dummy = tfs->getFactory(idx)->create();
    const uiTreeItem* child = findChild( dummy->name() );
    if ( children.indexOf(child)==-1 )
	return;

    removeChild( const_cast<uiTreeItem*>(child) );
}

// uiODDataTreeItem

uiODDataTreeItem::uiODDataTreeItem( const char* parenttype )
    : uiTreeItem( "" )
    , parenttype_( parenttype )
    , menu_( 0 )
    , movemnuitem_( "Move ..." )
    , movetotopmnuitem_( "to top" )
    , movetobottommnuitem_( "to bottom" )
    , moveupmnuitem_( "up" )
    , movedownmnuitem_( "down" )
    , removemnuitem_("Remove" )
    , changetransparencyitem_( "Change transparency ..." )
{}


uiODDataTreeItem::~uiODDataTreeItem()
{
    if ( menu_ )
    {
	menu_->createnotifier.remove( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.remove( mCB(this,uiODDataTreeItem,handleMenuCB) );
	menu_->unRef();
    }
}


TypeSet<uiDataTreeItemCreator> uiODDataTreeItem::creators_;

uiODDataTreeItem* uiODDataTreeItem::create( const Attrib::SelSpec& as,
					    const char* pt )
{
    for ( int idx=0; idx<creators_.size(); idx++)
    {
	uiODDataTreeItem* res = creators_[idx]( as, pt );
	if ( res )
	    return res;
    }

    return 0;
}


void uiODDataTreeItem::addFactory( uiDataTreeItemCreator cr )
{ creators_ += cr; }


int uiODDataTreeItem::uiListViewItemType() const
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs( displayID() ) )
	return uiListViewItem::CheckBox;
    else
	return uiTreeItem::uiListViewItemType();
}


uiODApplMgr* uiODDataTreeItem::applMgr() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::applmgrstr, res );
    return reinterpret_cast<uiODApplMgr*>( res );
}


uiSoViewer* uiODDataTreeItem::viewer() const
{
    void* res = 0;
    getPropertyPtr( uiODTreeTop::viewerptr, res );
    return reinterpret_cast<uiSoViewer*>( res );
}


bool uiODDataTreeItem::init()
{
    uiVisPartServer* visserv = applMgr()->visServer();
    if ( visserv->canHaveMultipleAttribs( displayID() ) )
    {
	uilistviewitem->stateChanged.notify(mCB(this,uiODDataTreeItem,checkCB));
	uilistviewitem->setChecked( visserv->isAttribEnabled(displayID(),
		    		    attribNr() ) );
    }

    return uiTreeItem::init();
}


void uiODDataTreeItem::checkCB( CallBacker* cb )
{
    uiVisPartServer* visserv = applMgr()->visServer();
    visserv->enableAttrib( displayID(), attribNr(),
	    		   uilistviewitem->isChecked() );
}


bool uiODDataTreeItem::shouldSelect( int selid ) const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    return selid!=-1 && selid==displayID() &&
	   visserv->getSelAttribNr()==attribNr();
}


int uiODDataTreeItem::sceneID() const
{
    int sceneid=-1;
    getProperty<int>( uiODTreeTop::sceneidkey, sceneid );
    return sceneid;
}



int uiODDataTreeItem::displayID() const
{
    mDynamicCastGet( uiODDisplayTreeItem*, odti, parent );
    return odti ? odti->displayID() : -1;
}


int uiODDataTreeItem::attribNr() const
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    const int nrattribs = visserv->getNrAttribs( displayID() );
    return nrattribs-siblingIndex()-1;
}


bool uiODDataTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify( mCB(this,uiODDataTreeItem,createMenuCB) );
	menu_->handlenotifier.notify( mCB(this,uiODDataTreeItem,handleMenuCB) );
    }

    return menu_->executeMenu(uiMenuHandler::fromTree);
}


void uiODDataTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    uiVisPartServer* visserv = applMgr()->visServer();
    const bool isfirst = !siblingIndex();
    const bool islast = siblingIndex()==visserv->getNrAttribs( displayID())-1;

    const bool islocked = visserv->isLocked( displayID() );

    mAddMenuItem( &movemnuitem_, &movetotopmnuitem_,
	    	  !islocked && !isfirst, false );
    mAddMenuItem( &movemnuitem_, &moveupmnuitem_,
	    	  !islocked && !isfirst, false );
    mAddMenuItem( &movemnuitem_, &movedownmnuitem_,
	    	  !islocked && !islast, false );
    mAddMenuItem( &movemnuitem_, &movetobottommnuitem_,
		  !islocked && !islast, false );

    mAddMenuItem( menu, &movemnuitem_, true, false );
    mAddMenuItem( menu, &removemnuitem_,
		  !islocked && visserv->getNrAttribs( displayID())>1, false );
    if ( visserv->canHaveMultipleAttribs(displayID()) )
	mAddMenuItem( menu, &changetransparencyitem_, true, false )
    else
	mResetMenuItem( &changetransparencyitem_ );
}


bool uiODDataTreeItem::select()
{
    applMgr()->visServer()->setSelObjectId( displayID(), attribNr() );
    return true;
}


void uiODDataTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = applMgr()->visServer();

    if ( mnuid==movetotopmnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx<nrattribs-1; idx++ )
	    visserv->swapAttribs( displayID(), idx, idx+1 );

	moveItemToTop();

	menu->setIsHandled( true );
    }
    else if ( mnuid==movetobottommnuitem_.id )
    {
	const int nrattribs = visserv->getNrAttribs( displayID() );
	for ( int idx=attribNr(); idx; idx-- )
	    visserv->swapAttribs( displayID(), idx, idx-1 );

	while ( siblingIndex()<nrattribs-1 )
	    moveItem( siblingBelow() );

	menu->setIsHandled( true );
    }
    else if ( mnuid==moveupmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr<visserv->getNrAttribs( displayID() )-1 )
	{
	    const int targetattribnr = attribnr+1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingAbove() );

	menu->setIsHandled(true);
    }
    else if ( mnuid==movedownmnuitem_.id )
    {
	const int attribnr = attribNr();
	if ( attribnr )
	{
	    const int targetattribnr = attribnr-1;
	    visserv->swapAttribs( displayID(), attribnr, targetattribnr );
	}

	moveItem( siblingBelow() );
	menu->setIsHandled( true );
    }
    else if ( mnuid==changetransparencyitem_.id )
    {
	menu->setIsHandled( true );
	visserv->showAttribTransparencyDlg( displayID(), attribNr() );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	const int attribnr = attribNr();
	visserv->removeAttrib( displayID(), attribNr() );

	prepareForShutdown();
	parent->removeChild( this );
	menu->setIsHandled( true );
    }
}


void uiODDataTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    uiTreeItem::updateColumnText( col );
}


// uiODAttribTreeItem

uiODAttribTreeItem::uiODAttribTreeItem( const char* parenttype )
    : uiODDataTreeItem( parenttype )
    , selattrmnuitem_( sKeySelAttribMenuTxt() )
{}


uiODAttribTreeItem::~uiODAttribTreeItem()
{}


bool uiODAttribTreeItem::anyButtonClick( uiListViewItem* item )
{
    if ( item!=uilistviewitem )
	return uiTreeItem::anyButtonClick( item );

    if ( !select() ) return false;

    uiVisPartServer* visserv = applMgr()->visServer();
    if ( !visserv->isClassification( displayID(), attribNr() ) )
	applMgr()->modifyColorTable( displayID(), attribNr() );

    return true;
}


void uiODAttribTreeItem::createSelMenu( MenuItem& mnu, int visid, int attrib,
					int sceneid)
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( as && visserv->hasAttrib(visid) )
    {
	uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();
	MenuItem* subitem = attrserv->storedAttribMenuItem( *as );
	mAddMenuItem( &mnu, subitem, subitem->nrItems(),
		       subitem->checked );

	subitem = attrserv->calcAttribMenuItem( *as );
	mAddMenuItem( &mnu, subitem, subitem->nrItems(),
			 subitem->checked );

	subitem = attrserv->nlaAttribMenuItem( *as );
	if ( subitem && subitem->nrItems() )
	    mAddMenuItem( &mnu, subitem, true, subitem->checked );

	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneid))
	if ( scene && scene->getDataTransform() )
	{
	    // TODO: get depthdomain key from scene
	    subitem = attrserv->depthdomainAttribMenuItem( *as, sKey::Wheeler );
	    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
	}
    }
}


const char* uiODAttribTreeItem::sKeySelAttribMenuTxt()
{ return "Select Attribute"; }


void uiODAttribTreeItem::createMenuCB( CallBacker* cb )
{
    const uiVisPartServer* visserv = applMgr()->visServer();

    mDynamicCastGet(uiMenuHandler*,menu,cb);

    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_, displayID(), attribNr(), sceneID() );

    if ( selattrmnuitem_.nrItems() )
	mAddMenuItem( menu, &selattrmnuitem_,
		!visserv->isLocked(displayID()), false );

    uiODDataTreeItem::createMenuCB( cb );
}


void uiODAttribTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDataTreeItem::handleMenuCB( cb );

    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu( mnuid, displayID(), attribNr()) )
    {
	menu->setIsHandled(true);
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


bool uiODAttribTreeItem::handleSelMenu( int mnuid, int visid, int attrib )
{
    if ( mnuid==-1 )
	return false;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    if ( !as ) return false;

    uiAttribPartServer* attrserv = ODMainWin()->applMgr().attrServer();

    Attrib::SelSpec myas( *as );
    if ( attrserv->handleAttribSubMenu(mnuid,myas) )
    {
	visserv->setSelSpec( visid, attrib, myas );
	visserv->calculateAttrib( visid, attrib, false );
	return true;
    }

    return false;
}


BufferString uiODAttribTreeItem::createDisplayName( int visid, int attrib )
{
    const uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( visid, attrib );
    BufferString dispname( as ? as->userRef() : 0 );
    if ( as && as->isNLA() )
    {
	dispname = as->objectRef();
	const char* nodenm = as->userRef();
	if ( IOObj::isKey(as->userRef()) )
	    nodenm = IOM().nameOf( as->userRef(), false );
	dispname += " ("; dispname += nodenm; dispname += ")";
    }

    if ( as && as->id()==Attrib::SelSpec::cAttribNotSel() )
	dispname = "<right-click>";
    else if ( !as )
	dispname = visserv->getObjectName( visid );
    else if ( as->id() == Attrib::SelSpec::cNoAttrib() )
	dispname="";

    return dispname;
}


BufferString uiODAttribTreeItem::createDisplayName() const
{
    return createDisplayName( displayID(), attribNr() );
}


void uiODAttribTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cColorColumn() )
    {
	uiVisPartServer* visserv = applMgr()->visServer();
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject( displayID() ))
	if ( !so )
	{
	    uiODDataTreeItem::updateColumnText( col );
	    return;
	}
	
	PtrMan<ioPixmap> pixmap = 0;
	if ( !so->hasColor() )
	{
	    int ctid = so->getColTabID( attribNr() );
	    const visBase::DataObject* obj = ctid>=0 ? 
				       visBase::DM().getObject( ctid ) : 0;
	    mDynamicCastGet(const visBase::VisColorTab*,coltab,obj);
	    if ( coltab )
	    { 
		const char* tablename = coltab->colorSeq().colors().name();
		PtrMan<ioPixmap> pixmap =
		    new ioPixmap(  tablename, cPixmapWidth(), cPixmapHeight() );
		uilistviewitem->setPixmap( uiODSceneMgr::cColorColumn(),
					   *pixmap );
	    }
	}
    }

    uiODDataTreeItem::updateColumnText( col );
}


#define mDisplayInit( inherited, creationfunc, checkfunc ) \
\
    if ( displayid_==-1 ) \
    {	\
	displayid_ = applMgr()->visServer()->creationfunc; \
	if ( displayid_==-1 ) \
	{\
	    return false;\
	}\
    } \
    else if ( !applMgr()->visServer()->checkfunc ) \
	return false;  \
\
    if ( !inherited::init() ) \
	return false; \



#define mParentShowSubMenu( creation ) \
    uiPopupMenu mnu( getUiParent(), "Action" ); \
    mnu.insertItem( new uiMenuItem("Add"), 0 ); \
    addStandardItems( mnu ); \
    const int mnuid = mnu.exec(); \
    if ( mnuid==0 ) \
	creation; \
    handleStandardItems( mnuid ); \
 \
    return true; \


bool uiODDisplayTreeItem::create( uiTreeItem* treeitem, uiODApplMgr* appl,
				  int displayid )
{
    const uiTreeFactorySet* tfs = ODMainWin()->sceneMgr().treeItemFactorySet();
    if ( !tfs )
	return false;

    for ( int idx=0; idx<tfs->nrFactories(); idx++ )
    {
	mDynamicCastGet(const uiODTreeItemFactory*,itmcreater,
			tfs->getFactory(idx))
	if ( !itmcreater ) continue;

	uiTreeItem* res = itmcreater->create( displayid, treeitem );
	if ( res )
	{
	    treeitem->addChild( res, true );
	    return true;
	}
    }

    return false;
}


static const int sAttribIdx = 1000;
static const int sDuplicateIdx = 900;
static const int sLockIdx = -900;
static const int sRemoveIdx = -1000;

uiODDisplayTreeItem::uiODDisplayTreeItem()
    : uiODTreeItem(0)
    , displayid_(-1)
    , visserv(ODMainWin()->applMgr().visServer())
    , addattribmnuitem_("Add attribute",sAttribIdx)
    , duplicatemnuitem_("Duplicate",sDuplicateIdx)
    , lockmnuitem_("Lock",sLockIdx)
    , removemnuitem_("Remove",sRemoveIdx)
{
}


uiODDisplayTreeItem::~uiODDisplayTreeItem()
{
    MenuHandler* menu = visserv->getMenuHandler();
    if ( menu )
    {
	menu->createnotifier.remove(mCB(this,uiODDisplayTreeItem,createMenuCB));
	menu->handlenotifier.remove(mCB(this,uiODDisplayTreeItem,handleMenuCB));
    }

    if ( uilistviewitem->pixmap(0) )
	delete uilistviewitem->pixmap(0);
}


int uiODDisplayTreeItem::selectionKey() const { return displayid_; }


bool uiODDisplayTreeItem::shouldSelect( int selkey ) const
{
    return uiTreeItem::shouldSelect( selkey ) && visserv->getSelAttribNr()==-1;
}


uiODDataTreeItem* uiODDisplayTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as ? uiODDataTreeItem::create( *as, parenttype ) :0;
    if ( !res ) res = new uiODAttribTreeItem( parenttype );
    return res;
}


bool uiODDisplayTreeItem::init()
{
    if ( !uiTreeItem::init() ) return false;

    if ( visserv->hasAttrib( displayid_ ) )
    {
	for ( int attrib=0; attrib<visserv->getNrAttribs(displayid_); attrib++ )
	{
	    const Attrib::SelSpec* as = visserv->getSelSpec(displayid_,attrib);
	    uiODDataTreeItem* item = createAttribItem( as );
	    if ( item ) addChild( item, false );
	}
    }

    visserv->setSelObjectId( displayid_ );
    uilistviewitem->setChecked( visserv->isOn(displayid_) );
    uilistviewitem->stateChanged.notify( mCB(this,uiODDisplayTreeItem,checkCB));

    name_ = createDisplayName();

    MenuHandler* menu = visserv->getMenuHandler();
    menu->createnotifier.notify( mCB(this,uiODDisplayTreeItem,createMenuCB) );
    menu->handlenotifier.notify( mCB(this,uiODDisplayTreeItem,handleMenuCB) );

    return true;
}


void uiODDisplayTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = createDisplayName();

    else if ( col==uiODSceneMgr::cColorColumn() )
    {
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject(displayid_))
	if ( !so )
	{
	    uiTreeItem::updateColumnText( col );
	    return;
	}
	
	PtrMan<ioPixmap> pixmap = 0;
	if ( so->hasColor() )
	{
	    pixmap = new ioPixmap( uiODDataTreeItem::cPixmapWidth(),
		    		   uiODDataTreeItem::cPixmapHeight() );
	    pixmap->fill( so->getColor() );
	}

	if ( pixmap ) uilistviewitem->setPixmap( uiODSceneMgr::cColorColumn(),
						 *pixmap );
    }

    uiTreeItem::updateColumnText( col );
}


bool uiODDisplayTreeItem::showSubMenu()
{
    return visserv->showMenu( displayid_, uiMenuHandler::fromTree );
}


void uiODDisplayTreeItem::checkCB( CallBacker* )
{
    if ( !visserv->isSoloMode() )
	visserv->turnOn( displayid_, uilistviewitem->isChecked() );
}


int uiODDisplayTreeItem::uiListViewItemType() const
{
    return uiListViewItem::CheckBox;
}


BufferString uiODDisplayTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
		const_cast<uiODDisplayTreeItem*>(this)->applMgr()->visServer();
    return cvisserv->getObjectName( displayid_ );
}


const char* uiODDisplayTreeItem::getLockMenuText() 
{ 
    return visserv->isLocked(displayid_) ? "Unlock" : "Lock";
}


void uiODDisplayTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    if ( visserv->hasAttrib(displayid_) &&
	 visserv->canHaveMultipleAttribs(displayid_) )
    {
	mAddMenuItem( menu, &addattribmnuitem_,
		      !visserv->isLocked(displayid_), false );
    }
    else
	mResetMenuItem( &addattribmnuitem_ );

    lockmnuitem_.text = getLockMenuText(); 
    mAddMenuItem( menu, &lockmnuitem_, true, false );
    
    mAddMenuItemCond( menu, &duplicatemnuitem_, true, false,
		      visserv->canDuplicate(displayid_) );

    mAddMenuItem( menu, &removemnuitem_, !visserv->isLocked(displayid_), false);
}


void uiODDisplayTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==lockmnuitem_.id )
    {
	menu->setIsHandled(true);
	mDynamicCastGet(visSurvey::SurveyObject*,so,
			visserv->getObject(displayid_))
	if ( !so )
	    return;

	so->lock( !so->isLocked() );
	PtrMan<ioPixmap> pixmap = 0;
	if ( so->isLocked() )
	    pixmap = new ioPixmap( GetIconFileName("lock_small.png") );
	else
	    pixmap = new ioPixmap();
	uilistviewitem->setPixmap( 0, *pixmap );
	ODMainWin()->sceneMgr().updateStatusBar();
    }
    else if ( mnuid==duplicatemnuitem_.id )
    {
	menu->setIsHandled(true);
	int newid =visserv->duplicateObject(displayid_,sceneID());
	if ( newid!=-1 )
	    uiODDisplayTreeItem::create( this, applMgr(), newid );
    }
    else if ( mnuid==removemnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( askContinueAndSaveIfNeeded() )
	{
	    prepareForShutdown();
	    visserv->removeObject( displayid_, sceneID() );
	    parent->removeChild( this );
	}
    }
    else if ( mnuid==addattribmnuitem_.id )
    {
	uiODDataTreeItem* newitem = createAttribItem(0);
	visserv->addAttrib( displayid_ );
	addChild( newitem, false );
	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText( uiODSceneMgr::cColorColumn() );
	menu->setIsHandled(true);
    }
}


uiODDataTreeItem* uiODEarthModelSurfaceTreeItem::createAttribItem(
					const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as ? uiODDataTreeItem::create( *as, parenttype) : 0;
    if ( !res ) res = new uiODEarthModelSurfaceDataTreeItem( emid, uivisemobj,
	    						     parenttype );
	    
    return res;
}


uiODEarthModelSurfaceTreeItem::uiODEarthModelSurfaceTreeItem(
						const EM::ObjectID& nemid )
    : emid(nemid)
    , uivisemobj(0)
    , savemnuitem_("Save")
    , saveasmnuitem_("Save as ...")
    , enabletrackingmnuitem_("Enable tracking")
    , changesetupmnuitem_("Change setup ...")
    , reloadmnuitem_("Reload")
    , relationsmnuitem_("Relations ...")
    , starttrackmnuitem_("Start tracking ...")
{}


uiODEarthModelSurfaceTreeItem::~uiODEarthModelSurfaceTreeItem()
{ 
    delete uivisemobj;
}


#define mDelRet { delete uivisemobj; uivisemobj = 0; return false; }


bool uiODEarthModelSurfaceTreeItem::init()
{
    delete uivisemobj;
    if ( displayid_!=-1 )
    {
	uivisemobj = new uiVisEMObject( getUiParent(), displayid_, visserv );
	if ( !uivisemobj->isOK() )
	    mDelRet;

	emid = uivisemobj->getObjectID();
    }
    else
    {
	uivisemobj = new uiVisEMObject( getUiParent(), emid, sceneID(),
					visserv );
	displayid_ = uivisemobj->id();
	if ( !uivisemobj->isOK() )
	    mDelRet;
    }

    if ( !uiODDisplayTreeItem::init() )
	return false;

    initNotify();

    return true;
}


void uiODEarthModelSurfaceTreeItem::checkCB( CallBacker* cb )
{
    uiODDisplayTreeItem::checkCB(cb);

    const int trackerid = applMgr()->mpeServer()->getTrackerID(emid);
    if ( trackerid==-1 )
    {
	prevtrackstatus = false;
	return;
    }

    if ( uilistviewitem->isChecked() )
	applMgr()->mpeServer()->enableTracking(trackerid, prevtrackstatus);
    else
    {
	prevtrackstatus = applMgr()->mpeServer()->isTrackingEnabled(trackerid);
	applMgr()->mpeServer()->enableTracking(trackerid,false);
    }
}


void uiODEarthModelSurfaceTreeItem::prepareForShutdown()
{
    applMgr()->EMServer()->askUserToSave(emid);
}


uiODEarthModelSurfaceDataTreeItem::uiODEarthModelSurfaceDataTreeItem(
							EM::ObjectID objid,
							uiVisEMObject* uv,
       							const char* parenttype )
    : uiODAttribTreeItem( parenttype )
    , depthattribmnuitem_("Z values")
    , savesurfacedatamnuitem_("Save attribute ...")
    , loadsurfacedatamnuitem_("Surface data ...")
    , emid(objid)
    , uivisemobj(uv)
{}


void uiODEarthModelSurfaceDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::createMenuCB( cb );
    mDynamicCastGet(uiMenuHandler*,menu,cb);

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(),
	    					     attribNr() );

    const bool islocked = visserv->isLocked( displayID() );
    mAddMenuItem( &selattrmnuitem_, &loadsurfacedatamnuitem_,
		  !islocked, false );
    mAddMenuItem( &selattrmnuitem_, &depthattribmnuitem_, !islocked,
		  as->id()==Attrib::SelSpec::cNoAttrib() );

    mAddMenuItem( menu, &savesurfacedatamnuitem_, as && as->id() >= 0, false );
}


void uiODEarthModelSurfaceDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    if ( mnuid==savesurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	ObjectSet<const BinIDValueSet> vals;
	visserv->getRandomPosCache( displayID(), attribNr(), vals );
	if ( vals.size() && vals[0]->nrVals()>=2 )
	{
	    const int auxnr =
		applMgr()->EMServer()->setAuxData( emid, vals, name_, 1 );
	    applMgr()->EMServer()->storeAuxData( emid, true );
	}
    }
    else if ( mnuid==depthattribmnuitem_.id )
    {
	menu->setIsHandled( true );
	uivisemobj->setDepthAsAttrib( attribNr() );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==loadsurfacedatamnuitem_.id )
    {
	menu->setIsHandled( true );
	const int auxdatanr = applMgr()->EMServer()->showLoadAuxDataDlg(emid);
	if ( auxdatanr<0 ) return;

	BufferString attrnm;
	ObjectSet<BinIDValueSet> vals;
	applMgr()->EMServer()->getAuxData( emid, auxdatanr, attrnm, vals );
	visserv->setSelSpec( displayID(), attribNr(),
		Attrib::SelSpec(attrnm,Attrib::SelSpec::cOtherAttrib()) );
	visserv->setRandomPosData( displayID(), attribNr(), &vals );
	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
}


BufferString uiODEarthModelSurfaceDataTreeItem::createDisplayName() const
{
    uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
    const Attrib::SelSpec* as = visserv->getSelSpec( displayID(),
	    					     attribNr() );

    if ( as->id()==Attrib::SelSpec::cNoAttrib() )
	return BufferString("Z values");

    return uiODAttribTreeItem::createDisplayName();
}


#define mIsObject(typestr) \
	!strcmp(uivisemobj->getObjectType(displayid_),typestr)

void uiODEarthModelSurfaceTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    MenuItem* trackmnu = menu->findItem(uiVisEMObject::trackingmenutxt);
    if ( uilistviewitem->isChecked() && trackmnu )
    {
	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );

	EM::SectionID section = -1;
	if ( uivisemobj->nrSections()==1 )
	    section = uivisemobj->getSectionID(0);
	else if ( menu->getPath() )
	    section = uivisemobj->getSectionID( menu->getPath() );

	const bool hastracker = applMgr()->mpeServer()->getTrackerID(emid)>=0;
	if ( !hastracker && !visserv->isLocked(displayid_) )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, true, false );
	    mResetMenuItem( &changesetupmnuitem_ );
	    mResetMenuItem( &enabletrackingmnuitem_ );
	    mResetMenuItem( &relationsmnuitem_ );
	}
	else if ( hastracker && section!=-1 && !visserv->isLocked(displayid_) )
	{
	    mAddMenuItem( trackmnu, &starttrackmnuitem_, false, false );
	    mAddMenuItem( trackmnu, &changesetupmnuitem_, true, false );
	    mAddMenuItem( trackmnu, &enabletrackingmnuitem_, true,
		   applMgr()->mpeServer()->isTrackingEnabled(
		      applMgr()->mpeServer()->getTrackerID(emid)) );

	    mResetMenuItem( &relationsmnuitem_ );
	    //mAddMenuItem( trackmnu, &relationsmnuitem_,
	//	    mIsObject(EM::Horizon::typeStr()), false );
	}

    }
    else
    {
	mResetMenuItem( &starttrackmnuitem_ );
	mResetMenuItem( &changesetupmnuitem_ );
	mResetMenuItem( &enabletrackingmnuitem_ );
	mResetMenuItem( &relationsmnuitem_ );
    }

    mAddMenuItem( menu, &savemnuitem_,
		  applMgr()->EMServer()->isChanged(emid) && 
		  applMgr()->EMServer()->isFullyLoaded(emid), false );

    mAddMenuItem( menu, &saveasmnuitem_, true, false );


#ifdef __debug__
    mAddMenuItem( menu, &reloadmnuitem_, true, false );
#else
    mResetMenuItem( &reloadmnuitem_ );
#endif
}


void uiODEarthModelSurfaceTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    EM::SectionID sectionid = -1;
    if ( uivisemobj->nrSections()==1 )
	sectionid = uivisemobj->getSectionID(0);
    else if ( menu->getPath() )
	sectionid = uivisemobj->getSectionID( menu->getPath() );

    if ( mnuid==savemnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject( emid, false );
    }
    else if ( mnuid==saveasmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->EMServer()->storeObject( emid, true );
	applMgr()->visServer()->setObjectName( displayid_,
		(const char*) applMgr()->EMServer()->getName(emid) );

	updateColumnText( uiODSceneMgr::cNameColumn() );
    }
    else if ( mnuid==starttrackmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( sectionid < 0 ) return;

	applMgr()->enableMenusAndToolbars(false);
	applMgr()->enableTree(false);

	if ( applMgr()->mpeServer()->addTracker(emid,menu->getPickedPos())!=-1 )
	{
	    uivisemobj->checkTrackingStatus();
	    applMgr()->visServer()->showMPEToolbar();
	}

	applMgr()->enableMenusAndToolbars(true);
	applMgr()->enableTree(true);
    }
    else if ( mnuid==changesetupmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->mpeServer()->showSetupDlg( emid, sectionid );
    }
    else if ( mnuid==reloadmnuitem_.id )
    {
	menu->setIsHandled(true);
	uiTreeItem* parent__ = parent;

	const MultiID mid = applMgr()->EMServer()->getStorageID(emid);

	applMgr()->visServer()->removeObject( displayid_, sceneID() );
	delete uivisemobj; uivisemobj = 0;

	if ( !applMgr()->EMServer()->loadSurface(mid) )
	    return;

	emid = applMgr()->EMServer()->getObjectID(mid);

	uivisemobj = new uiVisEMObject( getUiParent(), emid, sceneID(),visserv);
	displayid_ = uivisemobj->id();
    }
    else if ( mnuid==relationsmnuitem_.id )
    {	
	menu->setIsHandled(true);
	applMgr()->mpeServer()->showRelationsDlg( emid, sectionid );
    }
    else if ( mnuid==enabletrackingmnuitem_.id )
    {
	menu->setIsHandled(true);
	const int trackerid = applMgr()->mpeServer()->getTrackerID(emid);
	applMgr()->mpeServer()->enableTracking(trackerid,
		!applMgr()->mpeServer()->isTrackingEnabled(trackerid));
    }
}

/*
uiTreeItem* uiODBodyTreeItemFactory::create( int visid, uiTreeItem*) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Horizontal Tube")
	? new uiODBodyTreeItem(visid, true ) : 0;
}
*/


uiODSeis2DParentTreeItem::uiODSeis2DParentTreeItem()
    : uiODTreeItem("2D" )
{}



bool uiODSeis2DParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Add"), 0 );

    const int mnuid = mnu.exec();
    if ( mnuid < 0 ) return false;

    MultiID mid;
    bool success = ODMainWin()->applMgr().seisServer()->select2DSeis( mid );
    if ( !success ) return false;

    uiOD2DLineSetTreeItem* newitm = new uiOD2DLineSetTreeItem( mid );
    addChild( newitm, true );
    newitm->selectAddLines();

    return true;
}


uiTreeItem* Seis2DTreeItemFactory::create( int visid,
					   uiTreeItem* treeitem ) const
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    ODMainWin()->applMgr().visServer()->getObject(visid))
    if ( !s2d || !treeitem ) return 0;

    uiOD2DLineSetSubItem* newsubitm =
	new uiOD2DLineSetSubItem( s2d->name(), visid );
    mDynamicCastGet(uiOD2DLineSetSubItem*,subitm,treeitem);
    if ( subitm )
	return newsubitm;

    const MultiID& setid = s2d->lineSetID();
    BufferString linesetname;
    ODMainWin()->applMgr().seisServer()->get2DLineSetName( setid, linesetname );
    uiTreeItem* linesetitm = treeitem->findChild( linesetname );
    if ( linesetitm )
    {
	linesetitm->addChild( newsubitm, true );
	return 0;
    }

    uiOD2DLineSetTreeItem* newlinesetitm = new uiOD2DLineSetTreeItem( setid );
    treeitem->addChild( newlinesetitm, true );
    newlinesetitm->addChild( newsubitm, true );
    return 0;
}




uiOD2DLineSetTreeItem::uiOD2DLineSetTreeItem( const MultiID& mid )
    : setid_( mid )
    , menuhandler_( 0 )
    , addlinesitm_( "Add line(s) ..." )
    , showitm_( "Show all" )
    , hideitm_( "Hide all" )
    , showlineitm_( "Lines" )
    , hidelineitm_( "Lines" )
    , showlblitm_( "Linenames" )
    , hidelblitm_( "Linenames" )
    , removeitm_( "Remove" )
    , storeditm_( "Stored 2D data" )
    , selattritm_( "Select Attribute" )
{}


uiOD2DLineSetTreeItem::~uiOD2DLineSetTreeItem()
{ 
}


bool uiOD2DLineSetTreeItem::showSubMenu()
{
    if ( !menuhandler_ )
    {
	menuhandler_ = new uiMenuHandler( getUiParent(), -1 );
	menuhandler_->createnotifier.notify(
		mCB(this,uiOD2DLineSetTreeItem,createMenuCB) );
	menuhandler_->handlenotifier.notify(
		mCB(this,uiOD2DLineSetTreeItem,handleMenuCB) );
    }

    menuhandler_->executeMenu( uiMenuHandler::fromTree );
    return true;
}


void uiOD2DLineSetTreeItem::selectAddLines()
{
    BufferStringSet linenames;
    applMgr()->seisServer()->select2DLines( setid_, linenames );

    uiCursorChanger cursorchgr( uiCursor::Wait );
    for ( int idx=linenames.size()-1; idx>=0; idx-- )
	addChild( new uiOD2DLineSetSubItem(linenames.get(idx)), true );
}


void uiOD2DLineSetTreeItem::createMenuCB( CallBacker* cb )
{
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    mAddMenuItem( menu, &addlinesitm_, true, false );

    if ( children.size() )
    {
	Attrib::SelSpec as;
	MenuItem* dummy = applMgr()->attrServer()->storedAttribMenuItem( as );
	dummy->removeItems();

	BufferStringSet allstored;
	Attrib::SelInfo::getAttrNames( setid_, allstored );
	allstored.sort();
	storeditm_.createItems( allstored );
	mAddMenuItem( &selattritm_, &storeditm_, storeditm_.nrItems(), false );

	MenuItem* attrmenu = applMgr()->attrServer()->calcAttribMenuItem( as );
	mAddMenuItem( &selattritm_, attrmenu, attrmenu->nrItems(), false );

	MenuItem* nla = applMgr()->attrServer()->nlaAttribMenuItem( as );
	if ( nla && nla->nrItems() )
	    mAddMenuItem( &selattritm_, nla, true, false );

	mAddMenuItem( menu, &selattritm_, true, false );
	mAddMenuItem( menu, &showitm_, true, false );
	mAddMenuItem( &showitm_, &showlineitm_, true, false );
	mAddMenuItem( &showitm_, &showlblitm_, true, false );

	mAddMenuItem( menu, &hideitm_, true, false );
	mAddMenuItem( &hideitm_, &hidelineitm_, true, false );
	mAddMenuItem( &hideitm_, &hidelblitm_, true, false );
    }
    else
    {
	mResetMenuItem( &showitm_ );
	mResetMenuItem( &showlineitm_ );
	mResetMenuItem( &showlblitm_ );

	mResetMenuItem( &hideitm_ );
	mResetMenuItem( &hidelineitm_ );
	mResetMenuItem( &hidelblitm_ );
    }

    mAddMenuItem( menu, &removeitm_, true, false );
}


void uiOD2DLineSetTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller)
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==addlinesitm_.id )
    {
	selectAddLines();
	menu->setIsHandled(true);
	return;
    }

    Attrib::SelSpec as;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled( true );
	const char idx = storeditm_.itemIndex( mnuid );
	const char* attribnm = storeditm_.getItem(idx)->text;
	for ( int idx=0; idx<children.size(); idx++ )
	    ((uiOD2DLineSetSubItem*)children[idx])->displayStoredData(attribnm);
    }
    else if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,as) )
    {
	menu->setIsHandled( true );
	for ( int idx=0; idx<children.size(); idx++ )
	    ((uiOD2DLineSetSubItem*)children[idx])->setAttrib( as );
    }
    else if ( mnuid==removeitm_.id )
    {
	menu->setIsHandled( true );
	while( children.size() )
	{
	    uiOD2DLineSetSubItem* itm = (uiOD2DLineSetSubItem*)children[0];
	    applMgr()->visServer()->removeObject( itm->displayID(), sceneID() );
	    removeChild( itm );
	}
	parent->removeChild( this );
    }
    else if ( mnuid==showlineitm_.id || mnuid==hidelineitm_.id )
    {
	menu->setIsHandled( true );
	const bool turnon = mnuid==showlineitm_.id;
	for ( int idx=0; idx<children.size(); idx++ )
	    children[idx]->setChecked( turnon );
    }
    else if ( mnuid==showlblitm_.id || mnuid==hidelblitm_.id )
    {
	menu->setIsHandled( true );
	const bool turnon = mnuid==showlblitm_.id;
	for ( int idx=0; idx<children.size(); idx++ )
	    ((uiOD2DLineSetSubItem*)children[idx])->showLineName( turnon );
    }
}


const char* uiOD2DLineSetTreeItem::parentType() const
{ return typeid(uiODSeis2DParentTreeItem).name(); }



bool uiOD2DLineSetTreeItem::init()
{
    applMgr()->seisServer()->get2DLineSetName( setid_, name_ );
    return true;
}


uiOD2DLineSetSubItem::uiOD2DLineSetSubItem( const char* nm, int displayid )
    : linenmitm_("Show linename")
    , positionitm_("Position ...")
{
    name_ = nm;
    displayid_ = displayid;
}


uiOD2DLineSetSubItem::~uiOD2DLineSetSubItem()
{
    applMgr()->getOtherFormatData.remove(
	    mCB(this,uiOD2DLineSetSubItem,getNewData) );
}


const char* uiOD2DLineSetSubItem::parentType() const
{ return typeid(uiOD2DLineSetTreeItem).name(); }


bool uiOD2DLineSetSubItem::init()
{
    bool newdisplay = false;
    if ( displayid_==-1 )
    {
	mDynamicCastGet(uiOD2DLineSetTreeItem*,lsitm,parent)
	if ( !lsitm ) return false;

	visSurvey::Seis2DDisplay* s2d = visSurvey::Seis2DDisplay::create();
	visserv->addObject( s2d, sceneID(), false );
	s2d->setLineName( name_ );
	s2d->setLineSetID( lsitm->lineSetID() );
	displayid_ = s2d->id();

	s2d->turnOn( true );
	newdisplay = true;
    }

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( !s2d ) return false;

    PtrMan<PosInfo::Line2DData> geometry = new PosInfo::Line2DData;
    if ( !applMgr()->seisServer()->get2DLineGeometry( s2d->lineSetID(), name_,
	  *geometry ) )
	return false;

    CubeSampling cs = s2d->getCubeSampling();
    s2d->setGeometry( *geometry, newdisplay ? 0 : &cs );

    if ( applMgr() )
    {
	applMgr()->getOtherFormatData.notify(
	    mCB(this,uiOD2DLineSetSubItem,getNewData) );
    }

    return uiODDisplayTreeItem::init();
}


BufferString uiOD2DLineSetSubItem::createDisplayName() const
{
    return BufferString( visserv->getObjectName( displayid_ ) );
}


uiODDataTreeItem*
uiOD2DLineSetSubItem::createAttribItem( const Attrib::SelSpec* as ) const
{
    const char* parenttype = typeid(*this).name();
    uiODDataTreeItem* res = as
	? uiOD2DLineSetAttribItem::create( *as, parenttype )
	: 0;

    if ( !res ) res = new uiOD2DLineSetAttribItem( parenttype );
    return res;
}



void uiOD2DLineSetSubItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb)
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( !menu || menu->menuID() != displayID() || !s2d ) return;

    mAddMenuItem( menu, &linenmitm_, true, s2d->lineNameShown() );
    mAddMenuItem( menu, &positionitm_, true, false );
}


void uiOD2DLineSetSubItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
    visserv->getObject(displayid_));
    if ( !menu || !s2d || mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==linenmitm_.id )
    {
	menu->setIsHandled(true);
	s2d->showLineName( !s2d->lineNameShown() );
    }
    else if ( mnuid==positionitm_.id )
    {
	menu->setIsHandled(true);
	PtrMan<PosInfo::Line2DData> geometry = new PosInfo::Line2DData;
	    !applMgr()->seisServer()->get2DLineGeometry( s2d->lineSetID(),
	    s2d->name(), *geometry );
	CubeSampling maxcs = s2d->getCubeSampling();
	assign( maxcs.zrg, geometry->zrg );
	const TypeSet<PosInfo::Line2DPos>& pos = geometry->posns;
	if ( !pos.size() ) { pErrMsg( "Huh" ); return; }

	maxcs.hrg.start.crl = pos[0].nr;
	maxcs.hrg.stop.crl = pos[pos.size()-1].nr;

	CallBack dummycb;
	uiSliceSel positiondlg( getUiParent(), s2d->getCubeSampling(),
				maxcs, dummycb, uiSliceSel::TwoD );
	if ( !positiondlg.go() ) return;
	CubeSampling cs = positiondlg.getCubeSampling();
	s2d->setGeometry( *geometry, &cs );
	if ( s2d->getSelSpec(0) && s2d->getSelSpec(0)->id()>=0 )
	    visserv->calculateAttrib( displayid_, 0, false );
    }
}


bool uiOD2DLineSetSubItem::displayStoredData( const char* nm )
{
    if ( !children.size() ) return false;

    mDynamicCastGet( uiOD2DLineSetAttribItem*, lsai, children[0] );
    if ( !lsai ) return false;

    return lsai->displayStoredData(nm);
}


void uiOD2DLineSetSubItem::setAttrib( const Attrib::SelSpec& myas )
{
    if ( !children.size() ) return;

    mDynamicCastGet( uiOD2DLineSetAttribItem*, lsai, children[0] );
    if ( !lsai ) return;

    lsai->setAttrib( myas );
}


void uiOD2DLineSetSubItem::getNewData( CallBacker* cb )
{
    mCBCapsuleUnpack(int,visid,cb);
    if ( visid != displayid_ ) return;
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( !s2d ) return;

    const Attrib::SelSpec& as = *s2d->getSelSpec(0);
    const CubeSampling cs = s2d->getCubeSampling();

    const char* objnm = s2d->name();
    /*
    if ( as.id() == Attrib::SelSpec::otherAttrib )
    {
	ObjectSet<Attrib::DataHolder> dataset;
	ObjectSet<SeisTrcInfo> trcinfoset;
	LineKey linekey( objnm, as.userRef() );
	applMgr()->seisServer()->create2DOutput( s2d->lineSetID(), linekey,
	cs, dataset, trcinfoset );
	s2d->setTraceData( dataset, trcinfoset );
	return;
    }
*/
    applMgr()->attrServer()->setTargetSelSpec( as );
    RefMan<Attrib::Data2DHolder> dataset = new Attrib::Data2DHolder;

    if ( !applMgr()->attrServer()->create2DOutput( cs, objnm, *dataset) )
	return;
    if ( dataset->size() )
	s2d->setTraceData( *dataset );

}


void uiOD2DLineSetSubItem::showLineName( bool yn )
{
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayid_))
    if ( s2d ) s2d->showLineName( yn );
}


uiOD2DLineSetAttribItem::uiOD2DLineSetAttribItem( const char* pt )
    : uiODAttribTreeItem( pt )
    , attrnoneitm_("None")
    , storeditm_("Stored 2D data")
{}


void uiOD2DLineSetAttribItem::createMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    const uiVisPartServer* visserv = applMgr()->visServer();

    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject( displayID() ))
    if ( !menu || !s2d ) return;

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::SelSpec& as = *visserv->getSelSpec( displayID(), 0 );
    const char* objnm = visserv->getObjectName( displayID() );

    BufferStringSet attribnames;
    seisserv->get2DStoredAttribs( s2d->lineSetID(), objnm, attribnames );
    const int idx = attribnames.indexOf( "Steering" );
    if ( idx>=0 ) attribnames.remove( idx );

    const Attrib::DescSet* ads = attrserv->curDescSet();
    const Attrib::Desc* desc = ads->getDesc( as.id() );
    const bool isstored = desc && desc->isStored();

    selattrmnuitem_.removeItems();

    bool docheckparent = false;
    storeditm_.removeItems();
    for ( int idx=0; idx<attribnames.size(); idx++ )
    {
	const char* nm = attribnames.get(idx);
	MenuItem* item = new MenuItem(nm);
	const bool docheck = isstored && !strcmp(nm,as.userRef());
	if ( docheck ) docheckparent=true;
	mAddManagedMenuItem( &storeditm_,item,true,docheck);
    }

    mAddMenuItem( &selattrmnuitem_, &storeditm_, true, docheckparent );

    MenuItem* attrmenu = attrserv->calcAttribMenuItem(as);
    mAddMenuItem( &selattrmnuitem_, attrmenu, attrmenu->nrItems(), false );

    MenuItem* nla = attrserv->nlaAttribMenuItem(as);
    if ( nla && nla->nrItems() )
	mAddMenuItem( &selattrmnuitem_, nla, true, false );

    mAddMenuItem( &selattrmnuitem_, &attrnoneitm_, as.id()>=0, false );
}


void uiOD2DLineSetAttribItem::handleMenuCB( CallBacker* cb )
{
    uiODAttribTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
    visserv->getObject( displayID() ));
    if ( !menu || !s2d || mnuid==-1 || menu->isHandled() )
	return;

    Attrib::SelSpec myas;
    if ( storeditm_.itemIndex(mnuid)!=-1 )
    {
	uiCursorChanger cursorchgr( uiCursor::Wait );
	menu->setIsHandled(true);
	displayStoredData( storeditm_.findItem(mnuid)->text );
    }
    else if ( applMgr()->attrServer()->handleAttribSubMenu(mnuid,myas ) )
    {
	menu->setIsHandled(true);
	setAttrib( myas );
    }
    else if ( mnuid==attrnoneitm_.id )
    {
	uiCursorChanger cursorchgr( uiCursor::Wait );
	menu->setIsHandled(true);
	s2d->clearTexture();
	updateColumnText(0);
    }
}


bool uiOD2DLineSetAttribItem::displayStoredData( const char* attribnm )
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject( displayID() ))
    if ( !s2d ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    const Attrib::DescID attribid =
		attrserv->createStored2DAttrib( s2d->lineSetID(), attribnm );
    if ( attribid < 0 ) return false;

    const Attrib::SelSpec* as = visserv->getSelSpec(  displayID(),0 );
    Attrib::SelSpec myas( *as );
    LineKey linekey( s2d->name(), attribnm );
    myas.set( attribnm, attribid, false, 0 );
    attrserv->setTargetSelSpec( myas );
    RefMan<Attrib::Data2DHolder> dataset = new Attrib::Data2DHolder;

    if ( !applMgr()->attrServer()->create2DOutput( s2d->getCubeSampling(),
	    linekey, *dataset) )
	return false;

    if ( dataset->size() )
    {
	uiCursorChanger cursorchgr( uiCursor::Wait );
	s2d->setSelSpec( 0, myas );
	s2d->setTraceData( *dataset );
    }
    updateColumnText(0);
    setChecked( s2d->isOn() );

    return true;
}


void uiOD2DLineSetAttribItem::setAttrib( const Attrib::SelSpec& myas )
{
    const uiVisPartServer* visserv = applMgr()->visServer();
    mDynamicCastGet(visSurvey::Seis2DDisplay*,s2d,
		    visserv->getObject(displayID()))

    CubeSampling cs = s2d->getCubeSampling();
    BufferString linekey = s2d->name();
    applMgr()->attrServer()->setTargetSelSpec( myas );
    RefMan<Attrib::Data2DHolder> dataset = new Attrib::Data2DHolder;

    if ( !applMgr()->attrServer()->
	    create2DOutput( cs, linekey, *dataset ) )
	return;

    if ( dataset->size() )
    {
	s2d->setSelSpec( 0, myas );
	s2d->setTraceData( *dataset );
    }

    updateColumnText(0);
    setChecked( s2d->isOn() );
}



uiTreeItem* uiODRandomLineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd, 
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return rtd ? new uiODRandomLineTreeItem(visid) : 0;
}



uiODRandomLineParentTreeItem::uiODRandomLineParentTreeItem()
    : uiODTreeItem( "Random line" )
{}


bool uiODRandomLineParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add Random lines to this scene" );
	return false;
    }

    mParentShowSubMenu( addChild(new uiODRandomLineTreeItem(-1),false); );
}


uiODRandomLineTreeItem::uiODRandomLineTreeItem( int id )
    : editnodesmnuitem_("Edit nodes ...")
    , insertnodemnuitem_("Insert node")
    , usewellsmnuitem_("Create from wells...")
{ displayid_ = id; } 


bool uiODRandomLineTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::RandomTrackDisplay* rtd =
				    visSurvey::RandomTrackDisplay::create();
	displayid_ = rtd->id();
	visserv->addObject( rtd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
			visserv->getObject(displayid_));
	if ( !rtd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODRandomLineTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, &editnodesmnuitem_, !visserv->isLocked(displayid_), 
	    	  false );
    mAddMenuItem( menu, &insertnodemnuitem_, !visserv->isLocked(displayid_), 
	    	  false );
    insertnodemnuitem_.removeItems();

    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv->getObject(displayid_));
    for ( int idx=0; idx<=rtd->nrKnots(); idx++ )
    {
	if ( visserv->isLocked(displayid_) ) break;
	BufferString nodename;
	if ( idx==rtd->nrKnots() )
	{
	    nodename = "after node ";
	    nodename += idx-1;
	}
	else
	{
	    nodename = "before node ";
	    nodename += idx;
	}

	mAddManagedMenuItem(&insertnodemnuitem_,new MenuItem(nodename), 
			    rtd->canAddKnot(idx), false );
    }
    mAddMenuItem( menu, &usewellsmnuitem_, !visserv->isLocked(displayid_), 
	    	  false );
}


void uiODRandomLineTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;
	
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv->getObject(displayid_));

    if ( mnuid==editnodesmnuitem_.id )
    {
	editNodes();
	menu->setIsHandled(true);
    }
    else if ( insertnodemnuitem_.itemIndex(mnuid)!=-1 )
    {
	menu->setIsHandled(true);
	rtd->addKnot(insertnodemnuitem_.itemIndex(mnuid));
    }
    else if ( mnuid==usewellsmnuitem_.id )
    {
	menu->setIsHandled(true);
	applMgr()->wellServer()->selectWellCoordsForRdmLine();
    }
}


void uiODRandomLineTreeItem::editNodes()
{
    mDynamicCastGet(visSurvey::RandomTrackDisplay*,rtd,
		    visserv->getObject(displayid_));

    TypeSet<BinID> bids;
    rtd->getAllKnotPos( bids );
    uiDialog dlg( getUiParent(),
	    	  uiDialog::Setup("Random lines","Specify node positions","") );
    uiBinIDTable* table = new uiBinIDTable( &dlg, true );
    table->setBinIDs( bids );

    Interval<float> zrg = rtd->getDataTraceRange();
    zrg.scale( SI().zFactor() );
    table->setZRange( zrg );
    if ( dlg.go() )
    {
	TypeSet<BinID> newbids;
	table->getBinIDs( newbids );
	rtd->setKnotPositions( newbids );

	Interval<float> zrg;
	table->getZRange( zrg );
	zrg.scale( 1/SI().zFactor() );
	rtd->setDepthInterval( zrg );

	visserv->setSelObjectId( rtd->id() );
	for ( int attrib=0; attrib<visserv->getNrAttribs(rtd->id()); attrib++ )
	    visserv->calculateAttrib( rtd->id(), attrib, false );

	ODMainWin()->sceneMgr().updateTrees();
    }
}


uiODFaultParentTreeItem::uiODFaultParentTreeItem()
   : uiODTreeItem( "Fault" )
{}


bool uiODFaultParentTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("New ..."), 1 );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	TypeSet<EM::ObjectID> emids;
	applMgr()->EMServer()->selectFaults( emids );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    if ( emids[idx] < 0 ) continue;
	    addChild( new uiODFaultTreeItem(emids[idx]), false );
	}
    }
    else if ( mnuid == 1 )
    {
	//Will be restored by event (evWizardClosed) from mpeserv
	applMgr()->enableMenusAndToolbars(false);
	applMgr()->enableTree(false);

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );
	mps->addTracker( EM::Fault::typeStr() );
	return true;
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODFaultTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Fault")
	? new uiODFaultTreeItem(visid,true) : 0;
}


uiODFaultTreeItem::uiODFaultTreeItem( const EM::ObjectID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODFaultTreeItem::uiODFaultTreeItem( int id, bool dummy )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid_=id; }


uiODHorizonParentTreeItem::uiODHorizonParentTreeItem()
    : uiODTreeItem( "Horizon" )
{}


bool uiODHorizonParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add Horizons to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("New ..."), 1 );
    if ( children.size() )
    {
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("Display all only at sections"), 2 );
	mnu.insertItem( new uiMenuItem("Show all in full"), 3 );
    }
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid == 0 )
    {
	TypeSet<EM::ObjectID> emids;
	applMgr()->EMServer()->selectHorizons( emids );
	for ( int idx=0; idx<emids.size(); idx++ )
	{
	    if ( emids[idx] < 0 ) continue;
	    addChild( new uiODHorizonTreeItem(emids[idx]), false );
	}
    }
    else if ( mnuid == 1 )
    {
	//Will be restored by event (evWizardClosed) from mpeserv
	applMgr()->enableMenusAndToolbars(false);
	applMgr()->enableTree(false);

	uiMPEPartServer* mps = applMgr()->mpeServer();
	mps->setCurrentAttribDescSet( applMgr()->attrServer()->curDescSet() );
	mps->addTracker( EM::Horizon::typeStr() );
	return true;
    }
    else if ( mnuid == 2 || mnuid == 3 )
    {
	const bool onlyatsection = mnuid == 2;
	for ( int idx=0; idx<children.size(); idx++ )
	{
	    mDynamicCastGet(uiODEarthModelSurfaceTreeItem*,itm,children[idx])
	    if ( itm )
	    {
		itm->visEMObject()->setOnlyAtSectionsDisplay( onlyatsection );
		itm->updateColumnText(uiODSceneMgr::cColorColumn());
	    }
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODHorizonTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    const char* objtype = uiVisEMObject::getObjectType(visid);
    return objtype && !strcmp(objtype, "Horizon")
	? new uiODHorizonTreeItem(visid,true) : 0;
}


uiODHorizonTreeItem::uiODHorizonTreeItem( const EM::ObjectID& mid_ )
    : uiODEarthModelSurfaceTreeItem( mid_ )
{}


uiODHorizonTreeItem::uiODHorizonTreeItem( int id, bool )
    : uiODEarthModelSurfaceTreeItem( 0 )
{ displayid_=id; }


uiODHorizonTreeItem::~uiODHorizonTreeItem()
{}


void uiODHorizonTreeItem::initNotify()
{
    mDynamicCastGet(visSurvey::EMObjectDisplay*,
	    	    emd,visserv->getObject(displayid_));
    if ( emd )
	emd->changedisplay.notify( mCB(this,uiODHorizonTreeItem,dispChangeCB) );
}


BufferString uiODHorizonTreeItem::createDisplayName() const
{
    const uiVisPartServer* cvisserv =
	const_cast<uiODHorizonTreeItem*>(this)->applMgr()->visServer();

    BufferString res = cvisserv->getObjectName( displayid_ );

    if (  uivisemobj && uivisemobj->getShift() )
    {
	res += " (";
	res += uivisemobj->getShift();
	res += ")";
    }

    return res;
}


void uiODHorizonTreeItem::dispChangeCB(CallBacker*)
{
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


uiODWellParentTreeItem::uiODWellParentTreeItem()
    : uiODTreeItem( "Well" )
{}


bool uiODWellParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add Wells to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("Load ..."), 0 );
    mnu.insertItem( new uiMenuItem("New WellTrack ..."), 1 );
    if ( children.size() )
	mnu.insertItem( new uiMenuItem("Properties ..."), 2 );
    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return false;
    if ( mnuid == 0 )
    {
	ObjectSet<MultiID> emwellids;
	applMgr()->selectWells( emwellids );
	if ( !emwellids.size() )
	    return false;

	for ( int idx=0; idx<emwellids.size(); idx++ )
	    addChild( new uiODWellTreeItem(*emwellids[idx]), false );

	deepErase( emwellids );
    }
    else if ( mnuid == 1 )
    {
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	wd->setupPicking(true);
	BufferString name;
	Color color;
	if ( !applMgr()->wellServer()->setupNewWell(name,color) )
	    return false;
	wd->setLineStyle( LineStyle(LineStyle::Solid,1,color) );
	wd->setName( name );
	visserv->addObject( wd, sceneID(), true );
	addChild( new uiODWellTreeItem(wd->id()), false );
    }
    else if ( mnuid == 2 || mnuid == 3 )
    {
	uiVisPartServer* visserv = ODMainWin()->applMgr().visServer();
	TypeSet<int> wdids;
	visserv->findObject( typeid(visSurvey::WellDisplay), wdids );
	ObjectSet<visSurvey::WellDisplay> wds;
	for ( int idx=0; idx<wdids.size(); idx++ )
	{
	    mDynamicCastGet(visSurvey::WellDisplay*,wd,
			    visserv->getObject(wdids[idx]))
	    wds += wd;
	}

	if ( !wds.size() ) return false;

	if ( mnuid == 2 )
	{
	    uiWellPropDlg dlg( getUiParent(), wds );
	    dlg.go();

	    for ( int idx=0; idx<children.size(); idx++ )
	    {
		mDynamicCastGet(uiODWellTreeItem*,itm,children[idx])
		if ( itm )
		    itm->updateColumnText(uiODSceneMgr::cColorColumn());
	    }
	}
	else if ( mnuid == 3 )
	{
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODWellTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd, 
		    ODMainWin()->applMgr().visServer()->getObject(visid));
    return wd ? new uiODWellTreeItem(visid) : 0;
}


uiODWellTreeItem::uiODWellTreeItem( int did )
{
    displayid_ = did;
    initMenuItems();
}


uiODWellTreeItem::uiODWellTreeItem( const MultiID& mid_ )
{
    mid = mid_;
    initMenuItems();
}


uiODWellTreeItem::~uiODWellTreeItem()
{
}


void uiODWellTreeItem::initMenuItems()
{
    attrmnuitem_.text = "Create attribute log...";
    sellogmnuitem_.text = "Select logs ...";
    propertiesmnuitem_.text = "Properties ...";
    namemnuitem_.text = "Well name";
    markermnuitem_.text = "Markers";
    markernamemnuitem_.text = "Marker names";
    showlogmnuitem_.text = "Logs" ;
    showmnuitem_.text = "Show" ;
    editmnuitem_.text = "Edit Welltrack" ;
    storemnuitem_.text = "Store ...";
}


bool uiODWellTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::WellDisplay* wd = visSurvey::WellDisplay::create();
	displayid_ = wd->id();
	visserv->addObject( wd, sceneID(), true );
	if ( !wd->setWellId(mid) )
	{
	    visserv->removeObject( wd, sceneID() );
	    return false;
	}
    }
    else
    {
	mDynamicCastGet(visSurvey::WellDisplay*,wd,
			visserv->getObject(displayid_));
	if ( !wd )
	    return false;

	mDynamicCastGet(visSurvey::Scene*,scene,
			visserv->getObject(sceneID()));
	if ( scene )
	    wd->setDisplayTransformation( scene->getUTM2DisplayTransform() );
    }

    return uiODDisplayTreeItem::init();
}
	    
	
void uiODWellTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID()!=displayID() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid_));

    mAddMenuItem( menu, &sellogmnuitem_,
		  applMgr()->wellServer()->hasLogs(wd->wellId()), false );
    mAddMenuItem( menu, &attrmnuitem_, true, false );
    mAddMenuItem( menu, &propertiesmnuitem_, true, false );
    mAddMenuItem( menu, &editmnuitem_, true, false );
    mAddMenuItem( menu, &storemnuitem_, true, false );
    mAddMenuItem( menu, &showmnuitem_, true, false );
    mAddMenuItem( &showmnuitem_, &namemnuitem_, true,  wd->wellNameShown() );
    mAddMenuItem( &showmnuitem_, &markermnuitem_, wd->canShowMarkers(),
		 wd->markersShown() );
    mAddMenuItem( &showmnuitem_, &markernamemnuitem_, wd->canShowMarkers(),
		  wd->canShowMarkers() && wd->markerNameShown() );
    mAddMenuItem( &showmnuitem_, &showlogmnuitem_,
		  applMgr()->wellServer()->hasLogs(wd->wellId()), 
		  wd->logsShown() );
}


void uiODWellTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid_))
    const MultiID& wellid = wd->wellId();
    if ( mnuid == attrmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->wellAttribServer()->setAttribSet( 
				*applMgr()->attrServer()->curDescSet() );
	applMgr()->wellAttribServer()->createAttribLog( wellid );
    }
    else if ( mnuid==sellogmnuitem_.id )
    {
	menu->setIsHandled( true );
	int selidx = -1;
	int lognr = 1;
	applMgr()->wellServer()->selectLogs( wellid, selidx, lognr );
	if ( selidx > -1 )
	    wd->displayLog( selidx, lognr, false );
    }
    else if ( mnuid == propertiesmnuitem_.id )
    {
	menu->setIsHandled( true );
	uiWellPropDlg dlg( getUiParent(), wd );
	dlg.go();
	updateColumnText( uiODSceneMgr::cColorColumn() );
    }
    else if ( mnuid == namemnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showWellName( !wd->wellNameShown() );
    }
    else if ( mnuid == markermnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showMarkers( !wd->markersShown() );

    }
    else if ( mnuid == markernamemnuitem_.id )
    {
	menu->setIsHandled( true );
	wd->showMarkerName( !wd->markerNameShown() );
    }
    else if ( mnuid == showlogmnuitem_.id )
	wd->showLogs( !wd->logsShown() );
    
    else if ( mnuid == storemnuitem_.id )
    {
	BufferString errmsg;
	menu->setIsHandled( true );
	if ( wd->hasChanged() )
	    applMgr()->wellServer()->storeWell( wd->getWellCoords(), 
		    				wd->name(), errmsg );
    }
    else if ( mnuid == editmnuitem_.id )
    {
	menu->setIsHandled( true );
	bool yn = wd->isHomeMadeWell();
	wd->setupPicking(!yn);
	if ( !yn )
	{
	    uiCursorChanger cursorchgr( uiCursor::Wait );
	    wd->showKnownPositions();
	}
    }
}


bool uiODWellTreeItem::askContinueAndSaveIfNeeded()
{
    mDynamicCastGet(visSurvey::WellDisplay*,wd,visserv->getObject(displayid_));
    if ( wd->hasChanged() )
    {
	BufferString warnstr = "this well has changed since the last save.\n";
	warnstr += "Do you want to save it?";
	int retval = uiMSG().notSaved(warnstr.buf());
	if ( !retval ) return true;
	else if ( retval == -1 ) return false;
	else
	    applMgr()->wellServer()->storeWell( wd->getWellCoords(),
		                                wd->name(), 0 );
    }
    return true;
}

    
uiODPickSetParentTreeItem::uiODPickSetParentTreeItem()
    : uiODTreeItem( "PickSet" )
{}


bool uiODPickSetParentTreeItem::showSubMenu()
{
    mDynamicCastGet(visSurvey::Scene*,scene,
	    	    ODMainWin()->applMgr().visServer()->getObject(sceneID()));
    if ( scene && scene->getDataTransform() )
    {
	uiMSG().message( "Cannot add PickSets to this scene" );
	return false;
    }

    uiPopupMenu mnu( getUiParent(), "Action" );
    mnu.insertItem( new uiMenuItem("New/Load ..."), 0 );
    if ( children.size()>0 )
    {
	mnu.insertItem( new uiMenuItem("Store ..."), 1);
	mnu.insertSeparator();
	mnu.insertItem( new uiMenuItem("Display picks only at sections"), 2 );
	mnu.insertItem( new uiMenuItem("Show all picks"), 3 );
    }

    addStandardItems( mnu );

    const int mnuid = mnu.exec();
    if ( mnuid<0 ) return false;
    if ( mnuid==0 )
    {
	if ( !applMgr()->pickServer()->fetchPickSets() ) return -1;
	PickSetGroup& psg = applMgr()->pickServer()->group();
	if ( psg.nrSets() )
	{
	    for ( int idx=0; idx<psg.nrSets(); idx++ )
	    {
		//TODO make sure it's not in list already
		const PickSet* ps = psg.get( idx );
		if ( ps )
		    addChild( new uiODPickSetTreeItem(*ps), false );
	    }
	}
	else
	{
	    PickSet pset( psg.name() );
	    pset.color_ = applMgr()->getPickColor();
	    addChild( new uiODPickSetTreeItem(pset), false );
	}
    }
    else if ( mnuid==1 )
    {
	applMgr()->storePickSets();
    }
    else if ( mnuid==2 || mnuid==3 )
    {
	const bool showall = mnuid == 3;
	for ( int idx=0; idx<children.size(); idx++ )
	{
	    mDynamicCastGet(uiODPickSetTreeItem*,itm,children[idx])
	    if ( !itm ) continue;

	    itm->showAllPicks( showall );
	    itm->updateColumnText( uiODSceneMgr::cColorColumn() );
	}
    }
    else
	handleStandardItems( mnuid );

    return true;
}


uiTreeItem* uiODPickSetTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PickSetDisplay*, psd, 
		     ODMainWin()->applMgr().visServer()->getObject(visid));
    return psd ? new uiODPickSetTreeItem(visid) : 0;
}


uiODPickSetTreeItem::uiODPickSetTreeItem( const PickSet& ps )
    : ps_(new PickSet(ps))
    , renamemnuitem_("Rename ...")
    , storemnuitem_("Store ...")
    , dirmnuitem_("Set directions ...")
    , showallmnuitem_("Show all")
    , propertymnuitem_("Properties ...")
{}


uiODPickSetTreeItem::uiODPickSetTreeItem( int id )
    : ps_(0)
    , renamemnuitem_("Rename ...")
    , storemnuitem_("Store ...")
    , dirmnuitem_("Set directions ...")
    , showallmnuitem_("Show all")
    , propertymnuitem_("Properties ...")
{ displayid_ = id; }


uiODPickSetTreeItem::~uiODPickSetTreeItem()
{ 
    delete ps_; 
}


bool uiODPickSetTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::PickSetDisplay* psd = visSurvey::PickSetDisplay::create();
	displayid_ = psd->id();
	psd->copyFromPickSet( *ps_ );
	visserv->addObject(psd,sceneID(),true);
    }
    else
    {
	mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
			visserv->getObject(displayid_));
	if ( !psd ) return false;
    }

    return uiODDisplayTreeItem::init();
}


void uiODPickSetTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet( uiMenuHandler*, menu, cb );
    if ( menu->menuID()!=displayID() )
	return;

    mAddMenuItem( menu, &renamemnuitem_, true, false );
    mAddMenuItem( menu, &storemnuitem_, true, false );
    mAddMenuItem( menu, &dirmnuitem_, true, false );

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));

    mAddMenuItem( menu, &showallmnuitem_, true, psd->allShown() );
    mAddMenuItem( menu, &propertymnuitem_, true, false );
}


void uiODPickSetTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( uiMenuHandler*, menu, caller );
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;

    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));
    if ( mnuid==renamemnuitem_.id )
    {
	menu->setIsHandled(true);
	BufferString newname;
	const char* oldname = visserv->getObjectName( displayid_ );
	applMgr()->pickServer()->renamePickset( oldname, newname );
	visserv->setObjectName( displayid_, newname );
    }
    else if ( mnuid==storemnuitem_.id )
    {
	menu->setIsHandled( true );
	PickSet* ps = new PickSet( psd->name() );
	psd->copyToPickSet( *ps );
	applMgr()->pickServer()->storeSinglePickSet( ps );
	psd->setChanged( false );
    }
    else if ( mnuid==dirmnuitem_.id )
    {
	menu->setIsHandled( true );
	applMgr()->setPickSetDirs( displayid_ );
    }
    else if ( mnuid==showallmnuitem_.id )
    {
	showAllPicks( !psd->allShown() );
    }
    else if ( mnuid==propertymnuitem_.id )
    {
	uiPickSizeDlg dlg( getUiParent(), psd );
	dlg.go();
    }

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText( uiODSceneMgr::cColorColumn() );
}


void uiODPickSetTreeItem::showAllPicks( bool yn )
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
		    visserv->getObject(displayid_));
    psd->showAll( yn );
    mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()));
    scene->objectMoved(0);
}


bool uiODPickSetTreeItem::askContinueAndSaveIfNeeded()
{
    mDynamicCastGet(visSurvey::PickSetDisplay*,psd,
	    	    visserv->getObject(displayid_));
    if ( !psd->hasChanged() ) return true;

    BufferString warnstr = "This pickset has changed since the last save.\n"
			   "Do you want to save it?";
    const int retval = uiMSG().notSaved( warnstr.buf() );
    if ( retval == 0 )
	return true;
    else if ( retval == -1 )
	return false;
    else
    {
	PickSet* ps = new PickSet( psd->name() );
	psd->copyToPickSet( *ps );
	applMgr()->pickServer()->storeSinglePickSet( ps );
	psd->setChanged( false );
    }
    return true;
}


static const int sPositionIdx = 990;
static const int sGridLinesIdx = 980;

uiODPlaneDataTreeItem::uiODPlaneDataTreeItem( int did, int dim_ )
    : dim(dim_)
    , positiondlg(0)
    , positionmnuitem_("Position ...",sPositionIdx)
    , gridlinesmnuitem_("Gridlines ...",sGridLinesIdx)
{ displayid_ = did; }


uiODPlaneDataTreeItem::~uiODPlaneDataTreeItem()
{ delete positiondlg; }


bool uiODPlaneDataTreeItem::init()
{
    if ( displayid_==-1 )
    {
	visSurvey::PlaneDataDisplay* pdd=visSurvey::PlaneDataDisplay::create();
	displayid_ = pdd->id();
	pdd->setOrientation( (visSurvey::PlaneDataDisplay::Orientation) dim );
	visserv->addObject( pdd, sceneID(), true );
    }
    else
    {
	mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
			visserv->getObject(displayid_));
	if ( !pdd ) return false;
    }

    getItem()->moveForwdReq.notify(
			mCB(this,uiODPlaneDataTreeItem,moveForwdCB) );
    getItem()->moveBackwdReq.notify(
			mCB(this,uiODPlaneDataTreeItem,moveBackwdCB) );

    return uiODDisplayTreeItem::init();
}


BufferString uiODPlaneDataTreeItem::createDisplayName() const
{
    BufferString res;
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv->getObject(displayid_))
    const CubeSampling cs = pdd->getCubeSampling(true,true);
    const visSurvey::PlaneDataDisplay::Orientation orientation =
						    pdd->getOrientation();

    if ( orientation==visSurvey::PlaneDataDisplay::Inline )
	res = cs.hrg.start.inl;
    else if ( orientation==visSurvey::PlaneDataDisplay::Crossline )
	res = cs.hrg.start.crl;
    else
    {
	float zfactor = 1;
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
	if ( scene && !scene->getDataTransform() )
	    zfactor = SI().zFactor();
	res = cs.zrg.start * zfactor;
    }

    return res;
}


void uiODPlaneDataTreeItem::createMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::createMenuCB(cb);
    mDynamicCastGet(uiMenuHandler*,menu,cb);
    if ( menu->menuID() != displayID() )
	return;

    mAddMenuItem( menu, &positionmnuitem_, !visserv->isLocked(displayid_),
	          false );
    mAddMenuItem( menu, &gridlinesmnuitem_, true, false );

    uiSeisPartServer* seisserv = applMgr()->seisServer();
    int type = menu->getMenuType();
    if ( type==uiMenuHandler::fromScene )
    {
	MenuItem* displaygathermnu = seisserv->storedGathersSubMenu( true );
	if ( displaygathermnu )
	{
	    mAddMenuItem( menu, displaygathermnu, displaygathermnu->nrItems(),
		         false );
	    displaygathermnu->placement = -500;
	}
    }
}


void uiODPlaneDataTreeItem::handleMenuCB( CallBacker* cb )
{
    uiODDisplayTreeItem::handleMenuCB(cb);
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(uiMenuHandler*,menu,caller);
    if ( menu->menuID()!=displayID() || mnuid==-1 || menu->isHandled() )
	return;
    
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv->getObject(displayid_))

    if ( mnuid==positionmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;
	delete positiondlg;
	CubeSampling maxcs = SI().sampling(true);
	mDynamicCastGet(visSurvey::Scene*,scene,visserv->getObject(sceneID()))
	if ( scene && scene->getDataTransform() )
	{
	    const Interval<float> zintv =
		scene->getDataTransform()->getZInterval( false );
	    maxcs.zrg.start = zintv.start;
	    maxcs.zrg.stop = zintv.stop;
	}

	positiondlg = new uiSliceSel( getUiParent(),
				pdd->getCubeSampling(true,true), maxcs,
				mCB(this,uiODPlaneDataTreeItem,updatePlanePos), 
				(uiSliceSel::Type)dim );
	positiondlg->windowClosed.notify( 
		mCB(this,uiODPlaneDataTreeItem,posDlgClosed) );
	positiondlg->go();
	pdd->getMovementNotification()->notify(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
	applMgr()->enableMenusAndToolbars( false );
//	applMgr()->enableSceneManipulation( false );
    }
    else if ( mnuid == gridlinesmnuitem_.id )
    {
	menu->setIsHandled(true);
	if ( !pdd ) return;

	uiGridLinesDlg gldlg( getUiParent(), pdd );
	gldlg.go();
    }
    else
    {
	menu->setIsHandled(true);
	const Coord3 inlcrlpos = visserv->getMousePos(false);
	const BinID bid( (int)inlcrlpos.x, (int)inlcrlpos.y );
	applMgr()->seisServer()->handleGatherSubMenu( mnuid, bid );
    }
}


void uiODPlaneDataTreeItem::updatePositionDlg( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))
    const CubeSampling newcs = pdd->getCubeSampling();
    positiondlg->setCubeSampling( newcs );
}


void uiODPlaneDataTreeItem::posDlgClosed( CallBacker* )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))
    CubeSampling newcs = positiondlg->getCubeSampling();
    bool samepos = newcs == pdd->getCubeSampling();
    if ( positiondlg->uiResult() && !samepos )
    {
	pdd->setCubeSampling( newcs );
	pdd->resetManipulation();
	for ( int attrib=visserv->getNrAttribs(displayid_); attrib>=0; attrib--)
	    visserv->calculateAttrib( displayid_, attrib, false );

	updateColumnText( uiODSceneMgr::cNameColumn() );
	updateColumnText(1);
    }

    applMgr()->enableMenusAndToolbars( true );
    applMgr()->enableSceneManipulation( true );
    pdd->getMovementNotification()->remove(
		mCB(this,uiODPlaneDataTreeItem,updatePositionDlg) );
}


void uiODPlaneDataTreeItem::updatePlanePos( CallBacker* cb )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))
    mDynamicCastGet(uiSliceSel*,dlg,cb)
    if ( !dlg ) return;

    CubeSampling cs = dlg->getCubeSampling();
    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    for ( int attrib=visserv->getNrAttribs(displayid_); attrib>=0; attrib--)
	visserv->calculateAttrib( displayid_, attrib, false );

    updateColumnText( uiODSceneMgr::cNameColumn() );
    updateColumnText(1);
}


void uiODPlaneDataTreeItem::movePlane( const CubeSampling& cs )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
	    	    visserv->getObject(displayid_))

    pdd->setCubeSampling( cs );
    pdd->resetManipulation();
    for ( int attrib=visserv->getNrAttribs(displayid_); attrib>=0; attrib--)
	visserv->calculateAttrib( displayid_, attrib, false );
    updateColumnText(0);
    updateColumnText(1);
}


void uiODPlaneDataTreeItem::moveForwdCB( CallBacker* cb )
{
    movePlane( true );
}


void uiODPlaneDataTreeItem::moveBackwdCB( CallBacker* cb )
{
    movePlane( false );
}


void uiODPlaneDataTreeItem::movePlane( bool forward )
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd,
		    visserv->getObject(displayid_))

    CubeSampling cs = pdd->getCubeSampling();
    const int dir = forward ? 1 : -1;

    if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Inline )
    {
	cs.hrg.start.inl += cs.hrg.step.inl * dir;
	cs.hrg.stop.inl = cs.hrg.start.inl;
    }
    else if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Crossline )
    {
	cs.hrg.start.crl += cs.hrg.step.crl * dir;
	cs.hrg.stop.crl = cs.hrg.start.crl;
    }
    else if ( pdd->getOrientation() == visSurvey::PlaneDataDisplay::Timeslice )
    {
	cs.zrg.start += cs.zrg.step * dir;
	cs.zrg.stop = cs.zrg.start;
    }
    else
	return;

    movePlane( cs );
}


uiTreeItem* uiODInlineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet(visSurvey::PlaneDataDisplay*,pdd, 
	    	    ODMainWin()->applMgr().visServer()->getObject(visid))
    return pdd && pdd->getOrientation()==visSurvey::PlaneDataDisplay::Inline
    	   ? new uiODInlineTreeItem(visid) : 0;
}


uiODInlineParentTreeItem::uiODInlineParentTreeItem()
    : uiODTreeItem( "Inline" )
{ }


bool uiODInlineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODInlineTreeItem(-1),false); );
}


uiODInlineTreeItem::uiODInlineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 0 )
{}


uiTreeItem* uiODCrosslineTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getOrientation()==visSurvey::PlaneDataDisplay::Crossline
	? new uiODCrosslineTreeItem(visid) : 0;
}


uiODCrosslineParentTreeItem::uiODCrosslineParentTreeItem()
    : uiODTreeItem( "Crossline" )
{ }


bool uiODCrosslineParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODCrosslineTreeItem(-1),false); );
}


uiODCrosslineTreeItem::uiODCrosslineTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 1 )
{}


uiTreeItem* uiODTimesliceTreeItemFactory::create( int visid, uiTreeItem* ) const
{
    mDynamicCastGet( visSurvey::PlaneDataDisplay*, pdd, 
	    	     ODMainWin()->applMgr().visServer()->getObject(visid));
    return pdd && pdd->getOrientation()==visSurvey::PlaneDataDisplay::Timeslice
	? new uiODTimesliceTreeItem(visid) : 0;
}


uiODTimesliceParentTreeItem::uiODTimesliceParentTreeItem()
    : uiODTreeItem( "Timeslice" )
{}


bool uiODTimesliceParentTreeItem::showSubMenu()
{
    mParentShowSubMenu( addChild(new uiODTimesliceTreeItem(-1),false); );
}


uiODTimesliceTreeItem::uiODTimesliceTreeItem( int id )
    : uiODPlaneDataTreeItem( id, 2 )
{}


uiODSceneTreeItem::uiODSceneTreeItem( const char* name__, int id )
    : uiODTreeItem( name__ )
    , displayid_( id )
{}


void uiODSceneTreeItem::updateColumnText( int col )
{
    if ( col==uiODSceneMgr::cNameColumn() )
	name_ = applMgr()->visServer()->getObjectName( displayid_ );

    uiTreeItem::updateColumnText( col );
}


#define mProperties	0
#define mDumpIV		1

bool uiODSceneTreeItem::showSubMenu()
{
    uiPopupMenu mnu( getUiParent(), "Action" );

    uiMenuItem* anntxt = new uiMenuItem( "Properties ..." );
    mnu.insertItem( anntxt, mProperties );

    bool yn = false;
    Settings::common().getYN( IOPar::compKey("dTect","Dump OI Menu"), yn );
    if ( yn )
	mnu.insertItem( new uiMenuItem("Dump OI ..."), mDumpIV );

    uiVisPartServer* visserv = applMgr()->visServer();
    const int mnuid=mnu.exec();
    if ( mnuid==mProperties )
    {
	ObjectSet<uiSoViewer> viewers;
	ODMainWin()->sceneMgr().getSoViewers( viewers );

	mDynamicCastGet( visSurvey::Scene*, templscene,
			 visserv->getObject(displayid_) );

	uiScenePropertyDlg dlg( getUiParent(), templscene,
				viewer(), viewers, visserv );
	dlg.go();
    }
    else if ( mnuid==mDumpIV )
	visserv->dumpOI( displayid_ );

    return true;
}
