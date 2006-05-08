/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H. Bril
 Date:		Jul 2005
 RCS:		$Id: picksettr.cc,v 1.5 2006-05-08 20:22:19 cvsnanne Exp $
________________________________________________________________________

-*/

#include "picksetfact.h"
#include "pickset.h"
#include "ctxtioobj.h"
#include "binidvalset.h"
#include "ascstream.h"
#include "ioobj.h"
#include "iopar.h"
#include "ptrman.h"
#include "survinfo.h"
#include "streamconn.h"
#include "ioman.h"
#include "errh.h"
#include "keystrs.h"


const IOObjContext& PickSetGroupTranslatorGroup::ioContext()
{
    static IOObjContext* ctxt = 0;

    if ( !ctxt )
    {
	ctxt = new IOObjContext( &theInst() );
	ctxt->crlink = false;
	ctxt->newonlevel = 1;
	ctxt->needparent = false;
	ctxt->maychdir = true;
	ctxt->stdseltype = IOObjContext::Loc;
    }

    return *ctxt;
}


int PickSetGroupTranslatorGroup::selector( const char* key )
{
    int retval = defaultSelector( theInst().userName(), key );
    if ( retval ) return retval;

    if ( defaultSelector("Miscellaneous directory",key)
      || defaultSelector("Locations directory",key) ) return 1;

    return 0;
}


bool PickSetGroupTranslator::retrieve( Pick::SetGroup& psg, const IOObj* ioobj,
				  BufferString& bs, const bool* selarr )
{
    if ( !ioobj ) { bs = "Cannot find object in data base"; return false; }
    mDynamicCastGet(PickSetGroupTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a PickSet Group"; return false; }
    PtrMan<PickSetGroupTranslator> tr = t;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Read );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(true); return false; }
    bs = tr->read( psg, *conn, selarr );
    return bs == "";
}


bool PickSetGroupTranslator::store( const Pick::SetGroup& inppsg,
				    const IOObj* ioobj,
				    BufferString& bs, const bool* selarr,
				    bool domrg )
{
    if ( !ioobj ) { bs = "No object to store set in data base"; return false; }
    mDynamicCastGet(PickSetGroupTranslator*,t,ioobj->getTranslator())
    if ( !t ) { bs = "Selected object is not a PickSet Group"; return false; }

    PtrMan<PickSetGroupTranslator> tr = t;
    Pick::SetGroup mrgd;
    if ( domrg )
    {
	Conn* conn = ioobj->getConn( Conn::Read );
	if ( conn && !conn->bad() )
	{
	    delete conn;
	    if ( !retrieve(mrgd,ioobj,bs) )
		return false;
	}

	const int orgsz = mrgd.nrSets();
	for ( int idx=0; idx<inppsg.nrSets(); idx++ )
	{
	    const Pick::Set& ps = *inppsg.get( idx );
	    const UserIDString& nm = ps.name();
	    bool found = false;
	    for ( int iorg=0; iorg<orgsz; iorg++ )
	    {
		Pick::Set& mrgdps = *mrgd.get( iorg );
		if ( nm == mrgdps.name() )
		{
		    found = true;
		    mrgdps.copy( ps );
		    mrgdps.pixsize_ = ps.pixsize_;
		    mrgdps.color_ = ps.color_;
		    mrgdps.color_.setTransparency( 0 );
		    break;
		}
	    }
	    if ( !found )
	    {
		Pick::Set* newps = new Pick::Set( ps.name() );
		newps->copy( ps );
		newps->pixsize_ = ps.pixsize_;
		newps->color_ = ps.color_;
		newps->color_.setTransparency( 0 );
		mrgd.add( newps );
	    }
	}
    }

    bs = "";
    const Pick::SetGroup& wrgrp = domrg ? mrgd : inppsg;
    PtrMan<Conn> conn = ioobj->getConn( Conn::Write );
    if ( !conn )
        { bs = "Cannot open "; bs += ioobj->fullUserExpr(false); return false; }
    bs = tr->write( wrgrp, *conn, selarr );
    return bs == "";
}


const char* dgbPickSetGroupTranslator::read( Pick::SetGroup& psg, Conn& conn,
					     const bool* selarr )
{
    if ( !conn.forRead() || !conn.isStream() )
	return "Internal error: bad connection";

    ascistream astrm( ((StreamConn&)conn).iStream() );
    std::istream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot read from input file";
    if ( !astrm.isOfFileType(mTranslGroupName(PickSetGroup)) )
	return "Input file is not a Pick Set Group";
    if ( atEndOfSection(astrm) ) astrm.next();

    float zfac = 1;
    if ( astrm.hasKeyword("Z Factor") )
    {
	zfac = astrm.getValue();
	if ( mIsZero(zfac,mDefEps) || mIsUdf(zfac) )
	    zfac = 1;
	astrm.next();
	if ( atEndOfSection(astrm) ) astrm.next();
    }

    if ( atEndOfSection(astrm) )
	return "Input file contains no pick sets";

    psg.setName( conn.ioobj ? (const char*)conn.ioobj->name() : "" );

    for ( int ips=0; !atEndOfSection(astrm); ips++ )
    {
	Pick::Set* newps = selarr && !selarr[ips] ? 0
			 : new Pick::Set( astrm.value() );
	astrm.next();
	if ( astrm.hasKeyword(sKey::Color) )
	{
	    if ( newps ) newps->color_.use( astrm.value() );
	    astrm.next();
	}
	if ( astrm.hasKeyword(sKey::Size) )
	{
	    if ( newps ) newps->pixsize_ = astrm.getVal();
	    astrm.next();
	}
	Pick::Location loc;
	while ( !atEndOfSection(astrm) )
	{
	    if ( !loc.fromString( astrm.keyWord() ) )
		break;
	    loc.z *= zfac;
	    if ( newps ) *newps += loc;
	    astrm.next();
	}
	while ( !atEndOfSection(astrm) ) astrm.next();
	astrm.next();

	if ( newps ) psg.add( newps );
    }


    return psg.nrSets() ? 0 : "No valid picks found";
}


const char* dgbPickSetGroupTranslator::write( const Pick::SetGroup& psg,
						Conn& conn, const bool* selarr )
{
    if ( !conn.forWrite() || !conn.isStream() )
	return "Internal error: bad connection";

    ascostream astrm( ((StreamConn&)conn).oStream() );
    astrm.putHeader( mTranslGroupName(PickSetGroup) );
    std::ostream& strm = astrm.stream();
    if ( !strm.good() )
	return "Cannot write to output Pick Set file";

    for ( int iset=0; iset<psg.nrSets(); iset++ )
    {
	if ( selarr && !selarr[iset] ) continue;

	const Pick::Set& ps = *psg.get( iset );
	astrm.put( "Ref", ps.name() );
	char buf[80];
	if ( ps.color_ != Color::NoColor )
	{
	    ps.color_.fill( buf );
	    astrm.put( sKey::Color, buf );
	}
	if ( ps.pixsize_ != 0 )
	    astrm.put( sKey::Size, ps.pixsize_ );

	for ( int iloc=0; iloc<ps.size(); iloc++ )
	{
	    ps[iloc].toString( buf );
	    strm << buf << '\n';
	}
	astrm.newParagraph();
	if ( !strm.good() )
	    return "Error during write to output Pick Set file";
    }

    return 0;
}


void PickSetGroupTranslator::createBinIDValueSets(
			const BufferStringSet& ioobjids,
			ObjectSet<BinIDValueSet>& bivsets )
{
    for ( int idx=0; idx<ioobjids.size(); idx++ )
    {
	MultiID key( ioobjids.get( idx ) );
	PtrMan<IOObj>ioobj = IOM().get( key );
	BufferString msg;
	if ( !ioobj )
	{
	    msg = "Cannot find PickSet Group with key "; msg += key;
	    ErrMsg( msg ); continue;
	}
	Pick::SetGroup psg;
	if ( !retrieve(psg,ioobj,msg) )
	    { ErrMsg( msg ); continue; }

	const int nrsets = psg.nrSets();
	if ( !nrsets ) continue;

	BinIDValueSet* bs = new BinIDValueSet( 1, true );
	for ( int ips=0; ips<psg.nrSets(); ips++ )
	{
	    const Pick::Set& ps = *psg.get( ips );
	    const int nrpicks = ps.size();
	    if ( !nrpicks ) continue;

	    for ( int ipck=0; ipck<nrpicks; ipck++ )
	    {
		Pick::Location pl( ps[ipck] );
		bs->add( SI().transform(pl.pos), pl.z );
	    }
	}
	if ( bs->isEmpty() )
	    delete bs;
	else
	    bivsets += bs;
    }
}
