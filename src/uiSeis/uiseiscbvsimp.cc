/*+
________________________________________________________________________

 CopyRight:     (C) de Groot-Bril Earth Sciences B.V.
 Author:        Bert Bril
 Date:          Jun 2002
 RCS:		$Id: uiseiscbvsimp.cc,v 1.11 2003-01-16 11:26:25 bert Exp $
________________________________________________________________________

-*/

#include "uiseiscbvsimp.h"
#include "seistrctr.h"
#include "ioman.h"
#include "iostrm.h"
#include "iodir.h"
#include "iopar.h"
#include "ctxtioobj.h"
#include "ptrman.h"
#include "survinfo.h"
#include "filegen.h"

#include "uimsg.h"
#include "uifileinput.h"
#include "uiioobjsel.h"
#include "uiseistransf.h"
#include "uiexecutor.h"


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p )
	: uiDialog(p,Setup("Import CBVS cube",
		    	   "Specify import parameters",
			   "103.0.1"))
	, inctio_(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
	, outctio_(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
{
    init( false );
}


uiSeisImpCBVS::uiSeisImpCBVS( uiParent* p, const IOObj* ioobj )
	: uiDialog(p,Setup("Copy cube data",
		    	   "Specify copy parameters",
			   "103.1.1"))
	, inctio_(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
	, outctio_(*new CtxtIOObj(SeisTrcTranslator::ioContext()))
{
    if ( ioobj ) inctio_.ioobj = ioobj->clone();
    init( true );
    oinpSel(0);
}


void uiSeisImpCBVS::init( bool fromioobj )
{
    finpfld = 0; modefld = typefld = 0; oinpfld = 0;
    setTitleText( fromioobj ? "Specify transfer parameters"
	    		    : "Create CBVS cube definition" );

    if ( fromioobj )
    {
	inctio_.ctxt.forread = true;
	inctio_.ctxt.trglobexpr = "CBVS";
	oinpfld = new uiIOObjSel( this, inctio_, "Input data" );
	oinpfld->selectiondone.notify( mCB(this,uiSeisImpCBVS,oinpSel) );
    }
    else
    {
	finpfld = new uiFileInput( this, "(First) CBVS file name", GetDataDir(),
				    true, "*.cbvs;;*" );
	finpfld->valuechanged.notify( mCB(this,uiSeisImpCBVS,finpSel) );

	StringListInpSpec spec;
	spec.addString( "Input data cube" );
	spec.addString( "Generated attribute cube" );
	spec.addString( "Steering cube" );
	typefld = new uiGenInput( this, "Cube type", spec );
	typefld->attach( alignedBelow, finpfld );

	modefld = new uiGenInput( this, "Import mode",
				  BoolInpSpec("Copy the data","Use in-place") );
	modefld->attach( alignedBelow, typefld );
	modefld->valuechanged.notify( mCB(this,uiSeisImpCBVS,modeSel) );
    }

    transffld = new uiSeisTransfer( this, true );
    transffld->attach( alignedBelow, (constraintType*) modefld
	    ? (uiGroup*) modefld : (uiGroup*) oinpfld );

    outctio_.ctxt.forread = false;
    outctio_.ctxt.trglobexpr = "CBVS";
    IOM().to( outctio_.ctxt.stdSelKey() );
    seissel = new uiIOObjSel( this, outctio_, "Cube name" );
    seissel->attach( alignedBelow, transffld );
}


uiSeisImpCBVS::~uiSeisImpCBVS()
{
    delete outctio_.ioobj; delete &outctio_;
    delete inctio_.ioobj; delete &inctio_;
}


IOObj* uiSeisImpCBVS::getfInpIOObj( const char* inp ) const
{
    IOStream* iostrm = new IOStream( "tmp", "100010.9999" );
    iostrm->setGroup( outctio_.ctxt.trgroup->name() );
    iostrm->setTranslator( outctio_.ctxt.trglobexpr );
    iostrm->setFileName( inp );
    return iostrm;
}


void uiSeisImpCBVS::modeSel( CallBacker* )
{
    if ( modefld )
	transffld->display( modefld->getBoolValue() );
}


void uiSeisImpCBVS::oinpSel( CallBacker* )
{
    if ( inctio_.ioobj )
	transffld->updateFrom( *inctio_.ioobj );
}


void uiSeisImpCBVS::finpSel( CallBacker* )
{
    const char* out = seissel->getInput();
    if ( *out ) return;
    BufferString inp = finpfld->text();
    if ( !*(const char*)inp ) return;

    if ( !File_isEmpty(inp) )
    {
	PtrMan<IOObj> ioobj = getfInpIOObj( inp );
	transffld->updateFrom( *ioobj );
    }

    inp = File_getFileName( inp );
    if ( !*(const char*)inp ) return;

    // convert underscores to spaces
    char* ptr = inp.buf();
    while ( *ptr )
    {
	if ( *ptr == '_' ) *ptr = ' ';
	ptr++;
    }

    // remove .cbvs extension
    ptr--;
    if ( *ptr == 's' && *(ptr-1) == 'v' && *(ptr-2) == 'b'
     && *(ptr-3) == 'c' && *(ptr-4) == '.' )
	*(ptr-4) = '\0';

    seissel->setInputText( inp );
}


#define mErrRet(s) \
	{ uiMSG().error(s); return false; }

bool uiSeisImpCBVS::acceptOK( CallBacker* )
{
    if ( !seissel->commitInput(true) )
    {
	uiMSG().error( "Please choose a valid name for the cube" );
	return false;
    }

    const bool dolink = modefld && !modefld->getBoolValue();
    if ( oinpfld )
    {
	if ( !oinpfld->commitInput(false) )
	{
	    uiMSG().error( "Please select an input cube" );
	    return false;
	}
	outctio_.ioobj->pars() = inctio_.ioobj->pars();
    }
    else
    {
	const char* fname = finpfld->text();
	if ( !fname || !*fname )
	{
	    uiMSG().error( "Please select the input filename" );
	    return false;
	}
	const int seltyp = typefld->getIntValue();
	if ( !seltyp )
	    outctio_.ioobj->pars().removeWithKey( "Type" );
	else
	    outctio_.ioobj->pars().set( "Type",
				     seltyp == 1 ? "Attribute" : "Steering" );

	outctio_.ioobj->setTranslator( "CBVS" );
	if ( !dolink )
	    inctio_.setObj( getfInpIOObj(fname) );
	else
	{
	    mDynamicCastGet(IOStream*,iostrm,outctio_.ioobj);
	    iostrm->setFileName( fname );
	}
    }

    IOM().dirPtr()->commitChanges( outctio_.ioobj );
    if ( dolink )
	return true;

    const char* titl = oinpfld ? "Import CBVS seismic cube"
				: "Copy seismic data";
    PtrMan<Executor> stp = transffld->getTrcProc( inctio_.ioobj, outctio_.ioobj,
	   			titl, "Loading data" );

    uiExecutor dlg( this, *stp );
    return dlg.go() == 1
	&& transffld->provideUserInfo(*outctio_.ioobj);
}
