/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          August 2004
 RCS:           $Id: od_process_attrib_em.cc,v 1.6 2005-04-18 07:15:01 cvsnanne Exp $
________________________________________________________________________

-*/

#include "batchprog.h"
#include "attribdescsetproc.h"
#include "attribdescset.h"
#include "attribdescsettr.h"
#include "attribexecutor.h"
#include "iopar.h"
#include "ioman.h"
#include "ioobj.h"
#include "ptrman.h"
#include "progressmeter.h"
#include "hostdata.h"
#include "separstr.h"
#include "timefun.h"
#include "filegen.h"
#include "filepath.h"

#include "emmanager.h"
#include "emsurface.h"
#include "emsurfacegeometry.h"
#include "emsurfaceauxdata.h"
#include "emsurfaceiodata.h"
#include "parametricsurface.h"
#include "executor.h"
#include "survinfo.h"
#include "attribengman.h"
#include "attriboutputimpl.h"
#include "attribsel.h"


#include "attribfact.h"
#define mDestroyWorkers \
	{ delete exec; exec = 0; }


#define mErrRet(s) \
{ \
    strm << (stepout ? "0 0" : "0") << std::endl; \
    std::cerr << s << std::endl; \
    return false; \
}

static bool attribSetQuery( std::ostream& strm, const IOPar& iopar,
			    bool stepout )
{
    AttribDescSet initialset;
    PtrMan<IOPar> attribs = iopar.subselect("Attributes");
    if ( !initialset.usePar( *attribs ) )
	mErrRet( initialset.errMsg() )

    const char* res = iopar.find( "Output.1.Attributes.0" );
    if ( !res )
	mErrRet( "No target attribute found" )
    int outid = atoi( res );
    if ( initialset.descNr(outid) < 0 )
	mErrRet( "Target attribute not present in attribute set" )

    if ( !stepout )
	strm << "1" << std::endl;
    else
    {
	PtrMan<AttribDescSet> attrset = initialset.optimizeClone( outid );
	BinID maxstepout( attrset->getMaximumStepout() );
	strm << maxstepout.inl << ' ' << maxstepout.crl << std::endl;
    }

    return true;
}


static void getPositions( std::ostream& strm, const MultiID& id,
			  ObjectSet<BinIDValueSet>& data )
{
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.multiID2ObjectID(id);
    EM::EMObject* obj = em.getObject( objid );
    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( !surface ) return;

    strm << "\nFetching surface positions ...\n" ;
    ProgressMeter pm( strm );
    deepErase( data );
    const int nrsect = surface->geometry.nrSections();
    for ( int sectionidx=0; sectionidx<nrsect; sectionidx++ )
    {
	const EM::SectionID sectionid = 
	    			surface->geometry.sectionID( sectionidx );
	const Geometry::ParametricSurface* psurf = 
	    			surface->geometry.getSurface(sectionid);

	BinIDValueSet& res = *new BinIDValueSet( 1, false );
	data += &res;

	const int nrnodes = psurf->nrKnots();
	for ( int idy=0; idy<nrnodes; idy++ )
	{
	    const Coord3 crd = psurf->getKnot( psurf->getKnotRowCol(idy) );
	    const BinID bid = SI().transform(crd);
	    res.add( bid, crd.z );
	    ++pm;
	}
    }

    pm.finish();
    strm << "Done!" << std::endl;
}


static void addSurfaceData( const MultiID& id, const BufferStringSet& attrnms,
			    const ObjectSet<BinIDValueSet>& data )
{
    EM::EMManager& em = EM::EMM();
    EM::ObjectID objid = em.multiID2ObjectID(id);
    EM::EMObject* obj = em.getObject( objid );
    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( !surface ) return;

    surface->auxdata.removeAll();
    for ( int idx=0; idx<attrnms.size(); idx++ )
	surface->auxdata.addAuxData( attrnms.get(idx) );

    for ( int sectionidx=0; sectionidx<data.size(); sectionidx++ )
    {
	const EM::SectionID sectionid = 
	    			surface->geometry.sectionID( sectionidx );
	BinIDValueSet& bivs = *data[sectionidx];

	EM::PosID posid( objid, sectionid );
	BinIDValueSet::Pos pos;
	BinID bid; TypeSet<float> vals;
	while ( bivs.next(pos) )
	{
	    bivs.get( pos, bid, vals );
	    const EM::SubID subid = 
		surface->geometry.rowCol2SubID( RowCol(bid.inl,bid.crl) );
	    posid.setSubID( subid );
	    for ( int validx=1; validx<vals.size(); validx++ )
		surface->auxdata.setAuxDataVal( validx-1, posid, vals[validx] );
	}
    }
}


static void showHostName( std::ostream& strm )
{ strm << "Processing on " << HostData::localHostName() << '.' << std::endl; }


static bool prepare( std::ostream& strm, const IOPar& iopar, const char* idstr,
		     MultiID& mid, BufferString& errmsg )
{
    strm << "Preparing processing\n"; strm.flush();
    BufferString outstr( "Output.1." ); outstr += idstr;
    const char* objid = iopar.find( outstr );
    if ( !objid )
    {
	errmsg = "No "; errmsg += outstr; 
	errmsg += " defined in parameter file";
	return false;
    }
    else
    {
	PtrMan<IOObj> ioobj = IOM().get( objid );
	if ( !ioobj )
	{
	    errmsg = "Cannot find object for '"; errmsg += objid; 
	    errmsg += "' ...";
	    return false;
	}
    }

    mid = objid;
    return true;
}


#undef mErrRet
#define mErrRet(s) \
    { strm << '\n' << s << '\n' << std::endl; mDestroyWorkers ; return false; }

#define mPIDMsg(s) { strm << "\n["<< getPID() <<"]: " << s << std::endl; }

bool BatchProgram::go( std::ostream& strm )
{
    if ( cmdLineOpts().size() )
    {
	BufferString opt = *cmdLineOpts()[0];
	bool ismaxstepout = opt == "maxstepout";
	if ( ismaxstepout || opt == "validate" )
	    return attribSetQuery( strm, pars(), ismaxstepout );
    }

    showHostName( strm );

    AttribDescSetProcessor* proc = 0;
    AttribOutputExecutor* exec = 0;

    BufferString errmsg;
    MultiID mid;
    if ( !prepare( strm, pars(), "Surface ID", mid, errmsg ) )
	mErrRet( errmsg );

    strm << "Loading: " << mid.buf() << "\n\n";
    PtrMan<Executor> loader = EM::EMM().objectLoader( mid, 0 );
    if ( !loader || !loader->execute(&strm) ) mErrRet( "Cannot load surface" );

    AttribDescSet attribset;
    PtrMan<IOPar> attribs = pars().subselect( "Attributes" );
    if ( !attribset.usePar(*attribs) )
	mErrRet( attribset.errMsg() )

    PtrMan<IOPar> output = pars().subselect( "Output.1" );
    if ( !output ) mErrRet( "No output specified" );
    
    PtrMan<IOPar> attribsiopar = 
			    output->subselect(CubeAttribOutput::attribkey);
    if ( !attribsiopar ) mErrRet( "No output specified" );

    TypeSet<int> attribids;
    int nrattribs = 1;
    attribsiopar->get( "MaxNrKeys", nrattribs );
    for ( int idx=0; idx<nrattribs; idx++ )
    {
	BufferString key = idx;
	int id_;
	if ( attribsiopar->get(key,id_) )
	    attribids += id_;
    }

    if ( !attribids.size() )
	mErrRet( "No attributes selected" );

    AttribEngMan aem;
    aem.setAttribSet( &attribset );

    BufferStringSet attribrefs;
    for ( int idx=0; idx<attribids.size(); idx++ )
    {
	const int id_ = attribids[idx];
	if ( !idx )
	{
	    AttribSelSpec selspec( 0, id_ );
	    aem.setAttribSpec( selspec );
	}
	else
	    aem.addOutputAttrib( id_ );

	AttribDesc& ad = attribset.getAttribDesc( attribset.descNr(id_) );
	attribrefs.add( ad.userRef() );
    }

    // TODO: make a targetvalue for each output
    BufferString newattrnm;
    pars().get( "Target value", newattrnm );
    if ( newattrnm != "" )
	attribrefs.get(0) = newattrnm;

    ObjectSet<BinIDValueSet> bivs;
    getPositions( strm, mid, bivs );

    exec = (AttribOutputExecutor*)aem.tableOutputCreator( errmsg, bivs );

    ProgressMeter progressmeter(strm);
    strm << std::endl;
    strm << "Estimated number of positions to be processed (regular survey): "
	 << exec->totalNr() << std::endl;
    strm << "Loading cube data ..." << std::endl;

    bool cont = true;
    bool loading = true;
    bool oldcalc = false;

    while ( 1 )
    {
	int res = exec->nextStep();
	if ( res > 0 )
	{
	    if ( loading )
	    {
		loading = false;
		mPIDMsg( "Processing started." );
	    }
	    if ( res > 1 )
		mPIDMsg( exec->message() );

	    bool newcalc = exec->calculating();
	    if ( oldcalc != newcalc )
	    {
		progressmeter.resetDist();
		oldcalc = newcalc;
	    }

	    ++progressmeter;
	}
	else
	{
	    if ( res == -1 )
		mErrRet( exec->errMsg() )
	    break;
	}
    }

    progressmeter.finish();
    mPIDMsg( "Processing done." );

    // It is VERY important workers are destroyed BEFORE the last writeStatus!!!
    mDestroyWorkers

    addSurfaceData( mid, attribrefs, bivs );
    EM::EMObject* obj = EM::EMM().getObject( EM::EMM().multiID2ObjectID(mid) );
    mDynamicCastGet(EM::Surface*,surface,obj)
    if ( !surface ) mErrRet( "Huh" );

    EM::SurfaceIOData sd; sd.use( *surface );
    EM::SurfaceIODataSelection sels( sd );
    PtrMan<Executor> saver = surface->auxdata.auxDataSaver( -1, -1 );
    if ( !saver || !saver->execute(&strm) )
	mErrRet( "Cannot save data" );

    strm << "Successfully saved data." << std::endl;

    return true;
}
