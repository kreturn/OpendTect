/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bruno
 * DATE     : July 2011
-*/

static const char* rcsID = "$Id: uiprestackanglemutecomputer.cc,v 1.1 2011-07-12 10:51:55 cvsbruno Exp $";

#include "uiprestackanglemutecomputer.h"
#include "uiprestackanglemute.h"

#include "horsampling.h"
#include "prestackanglemutecomputer.h"
#include "prestackmute.h"
#include "prestackmutedeftransl.h"
#include "uiioobjsel.h"
#include "uigeninput.h"
#include "uimsg.h"
#include "uiraytrace1d.h"
#include "uiseissubsel.h"
#include "uitaskrunner.h"
#include "uiveldesc.h"


namespace PreStack
{

uiAngleMuteComputer::uiAngleMuteComputer( uiParent* p )
    : uiDialog( p, uiDialog::Setup("Angel Mute Computer",
				    mNoDlgTitle,mTODOHelpID) )
    , outctio_( *mMkCtxtIOObj(MuteDef) )
    , processor_(new AngleMuteComputer) 
{
    anglemutegrp_ = new uiAngleMuteGrp( this, processor_->params(), true );

    subsel_ = uiSeisSubSel::get( this, Seis::SelSetup( false ) );
    subsel_->attach( ensureBelow, anglemutegrp_ );

    outctio_.ctxt.forread = false;
    mutedeffld_ = new uiIOObjSel( this, outctio_ );
    mutedeffld_->attach( alignedBelow, subsel_ );
}


uiAngleMuteComputer::~uiAngleMuteComputer()
{
    delete processor_;
    delete &outctio_;
}


bool uiAngleMuteComputer::acceptOK(CallBacker*)
{
    if ( !anglemutegrp_->acceptOK() )
	return false;

    if ( !mutedeffld_->commitInput() || !outctio_.ioobj )
    {
	uiMSG().error("Please select a valid output mute function");
	return false;
    }
    HorSampling hrg;
    subsel_->getSampling( hrg );
    processor_->params().hrg_ = hrg;
    processor_->params().outputmutemid_ = mutedeffld_->key(true); 
    anglemutegrp_->rayTracer()->getOffsets( processor_->params().offsetrg_ );

    uiTaskRunner tr(this);
    if ( !tr.execute( *processor_ ) )
    {
	uiMSG().error( processor_->errMsg() );
	return false;
    }
    return true;
}

}; //namespace
