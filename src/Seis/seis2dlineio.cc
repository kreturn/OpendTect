/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : Dec 2009
-*/


#include "seis2dlineio.h"
#include "seis2ddata.h"
#include "seis2dlinemerge.h"
#include "seisselection.h"
#include "seisioobjinfo.h"
#include "seispacketinfo.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survgeom2d.h"
#include "posinfo2dsurv.h"
#include "bufstringset.h"
#include "trckeyzsampling.h"
#include "dirlist.h"
#include "file.h"
#include "filepath.h"
#include "dbman.h"
#include "ioobj.h"
#include "posinfo2d.h"
#include "ptrman.h"
#include "seisbuf.h"
#include "sorting.h"
#include "uistrings.h"
#include "keystrs.h"

Seis2DLineGetter::Seis2DLineGetter( SeisTrcBuf& trcbuf, int trcsperstep,
				    const Seis::SelData& sd )
    : Executor("Reading 2D Traces")
    , tbuf_(trcbuf)
    , seldata_(sd.clone())
{}

const char*
SeisTrc2DTranslatorGroup::getSurveyDefaultKey(const IOObj* ioobj) const
{ return IOPar::compKey( sKey::Default(), sKeyDefault() ); }


class Seis2DLineIOProviderSet : public ObjectSet<Seis2DLineIOProvider>
{
public:
    ~Seis2DLineIOProviderSet() { deepErase( *this ); }
};


ObjectSet<Seis2DLineIOProvider>& S2DLIOPs()
{
    mDefineStaticLocalObject( PtrMan<Seis2DLineIOProviderSet>, theinst,
			      = new Seis2DLineIOProviderSet );
    return *theinst.ptr();
}


bool TwoDSeisTrcTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return true;
    BufferString fnm( ioobj->fullUserExpr(true) );
    BufferString bakfnm( fnm ); bakfnm += ".bak";
    if ( File::exists(bakfnm) )
	File::remove( bakfnm );

    return File::remove( fnm );
}


bool TwoDSeisTrcTranslator::implRename( const IOObj* ioobj, const char* newnm,
					const CallBack* cb ) const
{
    if ( !ioobj )
	return false;

    PtrMan<IOObj> oldioobj = DBM().get( ioobj->key() );
    if ( !oldioobj ) return false;

    const bool isro = implReadOnly( ioobj );
    BufferString oldname( oldioobj->name() );
    PosInfo::POS2DAdmin().renameLineSet( oldname, ioobj->name() );
    implSetReadOnly( ioobj, isro );

    return Translator::implRename( ioobj, newnm, cb );
}


bool TwoDSeisTrcTranslator::initRead_()
{
    errmsg_.setEmpty();
    PtrMan<IOObj> ioobj = DBM().get( conn_ ? conn_->linkedTo() : DBKey() );
    if ( !ioobj )
	{ errmsg_ = tr( "Cannot reconstruct 2D filename" ); return false; }
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( !File::exists(fnm) )
    { errmsg_ = uiStrings::phrDoesntExist(toUiString(fnm)); return false; }

    return true;
}


bool SeisTrc2DTranslator::implRemove( const IOObj* ioobj ) const
{
    if ( !ioobj ) return true;
    BufferString fnm( ioobj->fullUserExpr(true) );
    Seis2DDataSet ds( *ioobj );
    const int nrlines = ds.nrLines();
    TypeSet<int> geomids;
    for ( int iln=0; iln<nrlines; iln++ )
	geomids.add( ds.geomID(iln) );

    for ( int iln=0; iln<nrlines; iln++ )
	ds.remove( geomids[iln] );

    DirList dl( fnm );
    return dl.isEmpty() ? File::remove( fnm ) : true;
}


bool SeisTrc2DTranslator::implRename( const IOObj* ioobj,
				    const char* newnm,const CallBack* cb ) const
{
    if ( !ioobj )
	return false;

    PtrMan<IOObj> oldioobj = DBM().get( ioobj->key() );
    if ( !oldioobj ) return false;

    const bool isro = implReadOnly( ioobj );
    BufferString oldname( oldioobj->name() );
    Seis2DDataSet ds( *ioobj );
    if ( !ds.rename(File::Path(newnm).fileName()) )
	return false;

    implSetReadOnly( ioobj, isro );

    return Translator::implRename( ioobj, newnm, cb );
}


bool SeisTrc2DTranslator::initRead_()
{
    errmsg_.setEmpty();
    PtrMan<IOObj> ioobj = DBM().get( conn_ ? conn_->linkedTo() : DBKey() );
    if ( !ioobj )
	{ errmsg_ = tr("Cannot reconstruct 2D filename"); return false; }
    BufferString fnm( ioobj->fullUserExpr(true) );
    if ( !File::exists(fnm) ) return false;

    Seis2DDataSet dset( *ioobj );
    if ( dset.nrLines() < 1 )
	{ errmsg_ = tr("Data set is empty"); return false; }
    dset.getTxtInfo( dset.geomID(0), pinfo_.usrinfo, pinfo_.stdinfo );
    addComp( DataCharacteristics(), pinfo_.stdinfo, Seis::UnknowData );

    if ( seldata_ )
	geomid_ = seldata_->geomID();

    if ( geomid_!=mUdfGeomID && dset.indexOf(geomid_)<0 )
	{ errmsg_ = tr( "Cannot find GeomID %1" ).arg(geomid_); return false; }

    TrcKeyZSampling cs( true );
    insd_.start = cs.zsamp_.start; insd_.step = cs.zsamp_.step;
    innrsamples_ = (int)((cs.zsamp_.stop-cs.zsamp_.start) /
			  cs.zsamp_.step + 1.5);
    pinfo_.inlrg.start = cs.hsamp_.start_.inl();
    pinfo_.inlrg.stop = cs.hsamp_.stop_.inl();
    pinfo_.inlrg.step = cs.hsamp_.step_.inl();
    pinfo_.crlrg.step = cs.hsamp_.step_.crl();
    pinfo_.crlrg.start = cs.hsamp_.start_.crl();
    pinfo_.crlrg.stop = cs.hsamp_.stop_.crl();
    return true;
}


#define mStdInit \
    , getter_(0) \
    , putter_(0) \
    , outbuf_(*new SeisTrcBuf(false)) \
    , tbuf1_(*new SeisTrcBuf(false)) \
    , tbuf2_(*new SeisTrcBuf(false)) \
    , l2dd1_(*new PosInfo::Line2DData) \
    , l2dd2_(*new PosInfo::Line2DData) \
    , outl2dd_(*new PosInfo::Line2DData) \
    , attrnms_(*new BufferStringSet) \
    , opt_(MatchTrcNr) \
    , numbering_(1,1) \
    , renumber_(false) \
    , stckdupl_(false) \
    , snapdist_(0.01) \
    , nrdone_(0) \
    , totnr_(-1) \
    , curattridx_(-1) \
    , currentlyreading_(0) \




Seis2DLineMerger::Seis2DLineMerger( const BufferStringSet& attrnms,
				    const Pos::GeomID& outgid )
    : Executor("Merging linens")
    , ds_(0)
    , outgeomid_(outgid)
    , msg_(tr("Opening files"))
    , nrdonemsg_(tr("Files opened"))
    mStdInit
{
    for ( int atridx=0; atridx<attrnms.size(); atridx++ )
	attrnms_.add( attrnms.get(atridx) );
}


Seis2DLineMerger::~Seis2DLineMerger()
{
    delete getter_;
    delete putter_;
    delete ds_;
    tbuf1_.deepErase();
    tbuf2_.deepErase();
    outbuf_.deepErase();

    delete &tbuf1_;
    delete &tbuf2_;
    delete &outbuf_;
    delete &attrnms_;
}




bool Seis2DLineMerger::getLineID( const char* lnm, int& lid ) const
{ return true; }


bool Seis2DLineMerger::nextAttr()
{
    curattridx_++;
    if ( !attrnms_.validIdx(curattridx_) )
	return false;

    PtrMan<IOObj> seisobj = DBM().getByName( mIOObjContext(SeisTrc2D),
					     attrnms_.get(curattridx_) );
    if ( !seisobj )
	return false;

    SeisIOObjInfo seisdatainfo( *seisobj );
    delete ds_;
    ds_ = new Seis2DDataSet( *seisdatainfo.ioObj() );
    currentlyreading_ = 0;
    return true;
}



#define mErrRet(s) \
    { \
	msg_ = s; \
	return false; \
    }

bool Seis2DLineMerger::nextGetter()
{
    if ( !ds_ )
	mErrRet(tr("Cannot find the Data Set"))
    if ( ds_->nrLines() < 2 )
	mErrRet(tr("Cannot find 2 lines in Line Set"));
    delete getter_; getter_ = 0;
    currentlyreading_++;
    if ( currentlyreading_ > 2 )
	return true;

    BufferString lnm( currentlyreading_ == 1 ? lnm1_ : lnm2_ );
    Pos::GeomID lid = Survey::GM().getGeomID( lnm );
    mDynamicCastGet(const Survey::Geometry2D*,geom2d,
		    Survey::GM().getGeometry(lid))
    if ( !geom2d )
	mErrRet( tr("Cannot find 2D Geometry of Line '%1'.").arg(lnm) )
    const PosInfo::Line2DData& l2dd( geom2d->data() );
    SeisTrcBuf& tbuf = currentlyreading_==1 ? tbuf1_ : tbuf2_;
    tbuf.deepErase();

    nrdone_ = 0;
    totnr_ = l2dd.positions().size();
    if ( totnr_ < 0 )
	mErrRet( tr("No data in %1").arg(geom2d->getName()) )
    const int dslineidx = ds_->indexOf( lid );
    if ( dslineidx<0 )
	mErrRet( tr("Cannot find line in %1 dataset" ).arg(geom2d->getName()) )
    getter_ = ds_->lineGetter( dslineidx, tbuf, 1 );
    if ( !getter_ )
	mErrRet(
	    uiStrings::phrCannotCreate(tr("a reader for %1.")
				       .arg(geom2d->getName()) ) )

    nrdonemsg_ = tr("Traces read");
    return true;
}


#undef mErrRet
#define mErrRet(s) \
{ \
    if ( !s.isEmpty() ) \
	msg_ = s; \
    return Executor::ErrorOccurred(); \
}

int Seis2DLineMerger::nextStep()
{
    return doWork();
}


int Seis2DLineMerger::doWork()
{
    if ( !currentlyreading_ && !nextAttr() )
	    return Executor ::Finished();

    if ( getter_ || !currentlyreading_ )
    {
	if ( !currentlyreading_ )
	    return nextGetter() ? Executor::MoreToDo()
				: Executor::ErrorOccurred();
	const int res = getter_->doStep();
	if ( res < 0 )
	    { msg_ = getter_->message(); return res; }
	else if ( res == 1 )
	    { nrdone_++; return Executor::MoreToDo(); }

	return nextGetter() ? Executor::MoreToDo()
			    : Executor::ErrorOccurred();
    }
    else if ( putter_ )
    {
	if ( nrdone_ >= outbuf_.size() )
	{
	    outbuf_.deepErase();
	    if ( !putter_->close() )
		mErrRet(putter_->errMsg())
	    delete putter_; putter_ = 0;
	    Survey::Geometry* geom = Survey::GMAdmin().getGeometry(outgeomid_);
	    mDynamicCastGet(Survey::Geometry2D*,geom2d,geom);
	    if ( !geom2d || !Survey::GMAdmin().write(*geom2d, msg_) )
		return Executor::ErrorOccurred();
	    geom2d->touch();
	    currentlyreading_ = 0;
	    return Executor::MoreToDo();
	}

	const SeisTrc& trc = *outbuf_.get( mCast(int,nrdone_) );
	if ( !putter_->put(trc) )
	    mErrRet(putter_->errMsg())

	if ( !curattridx_ )
	{
	    PosInfo::Line2DPos pos( trc.info().trcNr() );
	    pos.coord_ = trc.info().coord_;
	    Survey::Geometry* geom = Survey::GMAdmin().getGeometry( outgeomid_);
	    mDynamicCastGet(Survey::Geometry2D*,geom2d,geom);
	    if ( !geom2d )
		mErrRet( tr("Output 2D Geometry not written properly") );

	    PosInfo::Line2DData& outl2dd = geom2d->dataAdmin();
	    outl2dd.add( pos );
	}
	nrdone_++;
	return Executor::MoreToDo();
    }

    if ( tbuf1_.isEmpty() && tbuf2_.isEmpty() )
	mErrRet( tr("No input traces found") )

    mergeBufs();

    nrdone_ = 0;
    totnr_ = outbuf_.size();

    IOPar* lineiopar = new IOPar;
    lineiopar->set( sKey::GeomID(), outgeomid_ );
    putter_ = ds_->linePutter( outgeomid_ );
    if ( !putter_ )
	mErrRet(tr("Cannot create writer for output line") );

    nrdonemsg_ = tr("Traces written");
    return Executor::MoreToDo();
}


void Seis2DLineMerger::mergeBufs()
{
    makeBufsCompat();

    if ( opt_ == MatchCoords && !tbuf1_.isEmpty() && !tbuf2_.isEmpty() )
	mergeOnCoords();
    else
    {
	outbuf_.stealTracesFrom( tbuf1_ );
	outbuf_.stealTracesFrom( tbuf2_ );
	if ( opt_ == MatchTrcNr )
	{
	    outbuf_.sort( true, SeisTrcInfo::TrcNr );
	    outbuf_.enforceNrTrcs( 1, SeisTrcInfo::TrcNr, stckdupl_ );
	}
    }

    if ( renumber_ )
    {
	for ( int idx=0; idx<outbuf_.size(); idx++ )
	    outbuf_.get( idx )->info().trckey_ =
				TrcKey( outgeomid_, numbering_.atIndex(idx) );
    }
}


void Seis2DLineMerger::makeBufsCompat()
{
    if ( tbuf1_.isEmpty() || tbuf2_.isEmpty() )
	return;

    const SeisTrc& trc10 = *tbuf1_.get( 0 );
    const int trcsz = trc10.size();
    const int trcnc = trc10.nrComponents();
    const SamplingData<float> trcsd = trc10.info().sampling_;

    const SeisTrc& trc20 = *tbuf2_.get( 0 );
    if ( trc20.size() == trcsz && trc20.info().sampling_ == trcsd
      && trc20.nrComponents() == trcnc )
	return;

    for ( int itrc=0; itrc<tbuf2_.size(); itrc++ )
    {
	SeisTrc& trc = *tbuf2_.get( itrc );
	SeisTrc cptrc( trc );		// Copy old data for values
	trc = trc10;			// Get entire structure as trc10
	trc.info() = cptrc.info();	// Yet, keep old info
	trc.info().sampling_ = trcsd;	// ... but do take the samplingdata

	for ( int icomp=0; icomp<trcnc; icomp++ )
	{
	    const bool havethiscomp = icomp < cptrc.nrComponents();
	    for ( int isamp=0; isamp<trcsz; isamp++ )
	    {
		if ( !havethiscomp )
		    trc.set( isamp, 0, icomp );
		else
		{
		    const float z = trc.samplePos( isamp );
		    trc.set( isamp, cptrc.getValue(z,icomp), icomp );
		}
	    }
	}
    }
}


/* Algo:

   1) Determine points furthest away from other line
   2) Project all points on that line, lpar is then a sorting key
   3) sort, make outbuf accoring to that, and snap if needed
*/

void Seis2DLineMerger::mergeOnCoords()
{
    const int nrtrcs1 = tbuf1_.size() - 1;
    const int nrtrcs2 = tbuf2_.size() - 1;
    const Coord c10( tbuf1_.get(0)->info().coord_ );
    const Coord c11( tbuf1_.get(nrtrcs1)->info().coord_ );
    const Coord c20( tbuf2_.get(0)->info().coord_ );
    const Coord c21( tbuf2_.get(nrtrcs2)->info().coord_ );
    const double sqd10 = c10.sqDistTo( c20 ) + c10.sqDistTo( c21 );
    const double sqd11 = c11.sqDistTo( c20 ) + c11.sqDistTo( c21 );
    const double sqd20 = c20.sqDistTo( c10 ) + c20.sqDistTo( c11 );
    const double sqd21 = c21.sqDistTo( c10 ) + c21.sqDistTo( c11 );
    const Coord lnstart( sqd11 > sqd10 ? c11 : c10 );
    const Coord lnend( sqd21 > sqd20 ? c21 : c20 );
    const Coord lndelta( lnend.x_ - lnstart.x_, lnend.y_ - lnstart.y_ );
    const Coord sqlndelta( lndelta.x_ * lndelta.x_, lndelta.y_ * lndelta.y_ );
    const double sqabs = sqlndelta.x_ + sqlndelta.y_;
    if ( sqabs < 0.001 )
	return;

    TypeSet<double> lpars; TypeSet<int> idxs;
    for ( int ibuf=0; ibuf<2; ibuf++ )
    {
	const int ntr = ibuf ? nrtrcs2 : nrtrcs1;
	for ( int idx=0; idx<ntr; idx++ )
	{
	    const SeisTrcBuf& tb( ibuf ? tbuf2_ : tbuf1_ );
	    const Coord& ctrc( tb.get(idx)->info().coord_ );
	    const Coord crel( ctrc.x_ - lnstart.x_, ctrc.y_ - lnstart.y_ );
	    const double lpar = (lndelta.x_ * crel.x_ + lndelta.y_ * crel.y_)
			      / sqabs;
	    // const Coord projrelpt( lpar * lndelta.x_, lpar.lndelta.y_ );

	    lpars += lpar;
	    idxs += ibuf ? nrtrcs1 + idx : idx;
	}
    }

    sort_coupled( lpars.arr(), idxs.arr(), lpars.size() );
    doMerge( idxs, true );
}


void Seis2DLineMerger::doMerge( const TypeSet<int>& idxs, bool snap )
{
    const int nrtrcs1 = tbuf1_.size() - 1;
    for ( int idx=0; idx<idxs.size(); idx++ )
    {
	const int globidx = idxs[idx];
	const bool is1 = globidx < nrtrcs1;
	const int bufidx = globidx - (is1 ? 0 : nrtrcs1);
	outbuf_.add( (is1 ? tbuf1_ : tbuf2_).get(bufidx) );
    }
    tbuf1_.erase(); tbuf2_.erase();
    if ( !snap ) return;

    const double sqsnapdist = snapdist_ * snapdist_;
    int nrsnapped = 0;
    for ( int itrc=1; itrc<outbuf_.size(); itrc++ )
    {
	SeisTrc* prvtrc = outbuf_.get( itrc-1 );
	SeisTrc* curtrc = outbuf_.get( itrc );
	const double sqdist = curtrc->info().coord_.sqDistTo(
			      prvtrc->info().coord_ );
	if ( sqdist > sqsnapdist )
	    nrsnapped = 0;
	else
	{
	    nrsnapped++;
	    if ( stckdupl_ )
		SeisTrcPropChg( *prvtrc )
		    .stack( *curtrc, false, 1.f / ((float)nrsnapped) );

	    delete outbuf_.remove( itrc );
	    itrc--;
	}
    }
}
