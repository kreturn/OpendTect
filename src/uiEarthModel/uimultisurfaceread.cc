/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          July 2003
________________________________________________________________________

-*/

#include "uimultisurfaceread.h"

#include "uiioobjselgrp.h"
#include "uilistbox.h"
#include "uipossubsel.h"
#include "uimsg.h"

#include "ioobjctxt.h"
#include "emioobjinfo.h"
#include "emsurfaceiodata.h"
#include "emsurfacetr.h"
#include "dbman.h"
#include "ioobj.h"
#include "od_helpids.h"


uiMultiSurfaceReadDlg::uiMultiSurfaceReadDlg(uiParent* p, const char* type)
   : uiDialog(p,uiDialog::Setup( uiStrings::phrSelect(tr("Input %1").arg(
				 toUiString(type))),mNoDlgTitle,
                                 mODHelpKey(mMultiSurfaceReadDlgHelpID) )
                                 .nrstatusflds(1) )
{
    surfacefld_ = new uiMultiSurfaceRead( this, type );
    surfacefld_->objselGrp()->newStatusMsg.notify(
				mCB(this,uiMultiSurfaceReadDlg,statusMsg) );
    surfacefld_->singleSurfaceSelected.notify( mCB(this,uiDialog,accept) );
}


void uiMultiSurfaceReadDlg::statusMsg( CallBacker* cb )
{
    mCBCapsuleUnpack(const char* ,msg,cb);
    toStatusBar( toUiString(msg) );
}


bool uiMultiSurfaceReadDlg::acceptOK()
{
    return surfacefld_->objselGrp()->nrChosen() > 0;
}


// ***** uiMultiSurfaceRead *****
uiMultiSurfaceRead::uiMultiSurfaceRead( uiParent* p, const char* typ )
    : uiIOSurface(p,true,typ)
    , singleSurfaceSelected(this)
{
    ioobjselgrp_ = new uiIOObjSelGrp( this, *ctio_,
			uiIOObjSelGrp::Setup(OD::ChooseAtLeastOne) );
    ioobjselgrp_->selectionChanged.notify( mCB(this,uiMultiSurfaceRead,selCB) );
    ioobjselgrp_->getListField()->doubleClicked.notify(
					mCB(this,uiMultiSurfaceRead,dClck) );

    mkRangeFld();
    rgfld_->attach( leftAlignedBelow, ioobjselgrp_ );

    const FixedString type( typ );
    if ( type == EMHorizon2DTranslatorGroup::sGroupName() ||
	 type == EMFaultStickSetTranslatorGroup::sGroupName() ||
	 type == EMFault3DTranslatorGroup::sGroupName() )
    {
	rgfld_->display( false, true );
    }

    selCB(0);
}


uiMultiSurfaceRead::~uiMultiSurfaceRead()
{
}


void uiMultiSurfaceRead::dClck( CallBacker* )
{
    singleSurfaceSelected.trigger();
}


void uiMultiSurfaceRead::selCB( CallBacker* cb )
{
    if ( !rgfld_->mainObject() || !rgfld_->mainObject()->isDisplayed() ) return;

    const int nrsel = ioobjselgrp_->nrChosen();
    if( nrsel == 0 )
	return;

    if ( nrsel > 1 )
    {
	EM::SurfaceIOData sd;
	TrcKeySampling hs( false );
	if ( !processInput() ) return;
	for ( int idx=0; idx<nrsel; idx++ )
	{
	    const DBKey& mid = ioobjselgrp_->chosenID( idx );

	    EM::IOObjInfo eminfo( mid );
	    if ( !eminfo.isOK() ) continue;
	    TrcKeySampling emhs;
	    emhs.set( eminfo.getInlRange(), eminfo.getCrlRange() );

	    if ( hs.isEmpty() )
		hs = emhs;
	    else if ( !emhs.isEmpty() )
		hs.include( emhs, false );
	}

	fillRangeFld( hs );
	return;
    }

    if ( processInput() )
    {
	EM::SurfaceIOData sd;
	if ( getSurfaceIOData(ioobjselgrp_->chosenID(0),sd,cb) )
	    fillFields( sd );
    }
}


void uiMultiSurfaceRead::getSurfaceIds( DBKeySet& mids ) const
{
    mids.erase();
    const int nrsel = ioobjselgrp_->nrChosen();
    uiStringSet errormsgstr;
    for ( int idx=0; idx<nrsel; idx++ )
    {
	const DBKey mid = ioobjselgrp_->chosenID( idx );
	const EM::IOObjInfo info( mid );
	EM::SurfaceIOData sd;
	uiString errmsg;
	if ( info.getSurfaceData(sd,errmsg) )
	    mids += mid;
	else
	{
	    if ( !info.ioObj() )
		continue;

	    errormsgstr += tr("%1 :  %2\n").arg(info.ioObj()->uiName())
			.arg( errmsg );
	}

    }

    if ( !errormsgstr.isEmpty() )
    {
	if ( nrsel == 1  )
	    uiMSG().error( errormsgstr );
	else
	    uiMSG().error(
		    tr("The following selections will not be loaded:\n\n%1")
		    .arg(errormsgstr.cat()) );
    }

}


void uiMultiSurfaceRead::getSurfaceSelection(
					EM::SurfaceIODataSelection& sel ) const
{
    uiIOSurface::getSelection( sel );

    if ( ioobjselgrp_->nrChosen() != 1 )
	return;

    const DBKey mid = ioobjselgrp_->chosenID( 0 );
    const EM::IOObjInfo info( mid );
    EM::SurfaceIOData sd;
    uiString errmsg;
    if ( !info.getSurfaceData(sd,errmsg) || sd.sections.size() < 2
	    || !info.isHorizon() )
	return;

    uiDialog dlg( const_cast<uiParent*>(parent()),
	    uiDialog::Setup(uiStrings::phrSelect(tr("section(s)"))
			    ,mNoDlgTitle,mNoHelpKey) );
    uiListBox* lb = new uiListBox( &dlg, "Patches", OD::ChooseAtLeastOne );
    lb->addItems( sd.sections.getUiStringSet() );
    lb->chooseAll( true );
    if ( dlg.go() )
    {
	sel.selsections.erase();
	lb->getChosen( sel.selsections );
	if ( sel.selsections.isEmpty() )
	    sel.selsections += 0;
    }
}
