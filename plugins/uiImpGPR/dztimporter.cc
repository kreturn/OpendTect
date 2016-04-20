/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Oct 2003
-*/


#include "dztimporter.h"
#include "seistrc.h"
#include "seiswrite.h"
#include "seisselectionimpl.h"
#include "strmprov.h"
#include "datachar.h"
#include "survinfo.h"
#include "uistrings.h"
#include "od_istream.h"

static const float cNanoFac = 1e-9;


#define mRdVal(v) strm.getBin( v )

DZT::FileHeader::FileHeader()
    : nsamp(0)
    , nrdef_(1,1)
    , cstart_(SI().minCoord(true))
    , cstep_(SI().crlDistance(),0)
{
}


bool DZT::FileHeader::getFrom( od_istream& strm, BufferString& emsg )
{
    // From 0, (u)shorts:
    mRdVal(tag); mRdVal(data); mRdVal(nsamp); mRdVal(bits); mRdVal(zero);
    // From 10, floats:
    mRdVal(sps); mRdVal(spm); mRdVal(mpm); mRdVal(position); mRdVal(range);
    // From 30, dates (4 bytes each):
    mRdVal(created); mRdVal(modified);
    // From 38, unsigned shorts:
    mRdVal(npass); mRdVal(rgain); mRdVal(nrgain); mRdVal(text); mRdVal(ntext);
    mRdVal(proc); mRdVal(nproc); mRdVal(nchan);
    // From 54, floats:
    mRdVal(epsr); mRdVal(top); mRdVal(depth);
    // From 66, 1 byte dtype and 31 bytes reserved
    char buf[32]; strm.getBin( buf, 32 ); dtype = buf[31];
    // From 98, 14 bytes antenna
    strm.getBin( antname, 14 );
    // From 112, the rest
    mRdVal(chanmask); strm.getBin( buf, 12 );
    mRdVal(chksum);

    emsg.setEmpty();
#define mRetFalse nsamp = 0; return false
    if ( nsamp < 1 )
	{ emsg = "Zero Nr of samples found."; mRetFalse; }
    if ( sps < 1 )
	{ emsg = "Zero scans per second found."; mRetFalse; }
    if ( range < 1 )
	{ emsg = "Zero trace length found."; mRetFalse; }
    if ( data < 128 )
	{ emsg.add("Invalid data offset found: ").add(data); mRetFalse; }

    // dtype cannot be trusted, it seems
    if ( bits < 32 )
    {
	if ( dtype %2 )
	    dtype = bits == 16 ? 3 : 1;
	else
	    dtype = bits == 16 ? 2 : 0;
    }

    strm.setPosition( data, od_stream::Abs );
    if ( !strm.isOK() )
	{ emsg = "Cannot read first trace."; mRetFalse; }

    return true;
}


void DZT::FileHeader::fillInfo( SeisTrcInfo& ti, int trcidx ) const
{
    ti.nr_ = traceNr( trcidx );
    ti.sampling_.start = position;
    ti.sampling_.step = ((float)range) / (nsamp-1);
    ti.sampling_.scale( cNanoFac );
    ti.coord_.x = cstart_.x + cstep_.x * trcidx;
    ti.coord_.y = cstart_.y + cstep_.y * trcidx;
}


DZT::Importer::Importer( const char* fnm, const IOObj& ioobj, Pos::GeomID gid )
    : Executor("Importing DZT file")
    , nrdone_(0)
    , totalnr_(-1)
    , wrr_(0)
    , databuf_(0)
    , di_(DataCharacteristics())
    , trc_(*new SeisTrc)
    , istream_(*new od_istream(fnm))
    , zfac_(1)
{
    if ( !istream_.isOK() )
	return;

    BufferString msg;
    if ( !fh_.getFrom( istream_, msg ) )
    {
	msg_ = mToUiStringTodo(msg);
	return;
    }

    DataCharacteristics dc( fh_.dtype < 9, fh_.dtype %2 );
    dc.setNrBytes( fh_.dtype > 1 ? (fh_.dtype > 3 ? 4 : 2) : 1 );
    di_.set( dc );

    trc_.data().reSize( fh_.nsamp );
    for ( int ichan=1; ichan<fh_.nchan; ichan++ )
	trc_.data().addComponent( fh_.nsamp, DataCharacteristics() );

    wrr_ = new SeisTrcWriter( &ioobj );
    Seis::RangeSelData* rsd = new Seis::RangeSelData;
    rsd->setIsAll( true );
    rsd->setGeomID( gid );
    wrr_ = new SeisTrcWriter( &ioobj );
    wrr_->setSelData( rsd );

    databuf_ = new char [ fh_.nrBytesPerTrace() ];
    msg_ = tr("Handling traces");
}


DZT::Importer::~Importer()
{
    closeAll();
    delete [] databuf_;
    delete &trc_;
    delete &istream_;
}


int DZT::Importer::closeAll()
{
    istream_.close();

    deleteAndZeroPtr(wrr_);

    return Finished();
}


#define mErrRet(s) { msg_ = s; return ErrorOccurred(); }

int DZT::Importer::nextStep()
{
    if ( !fh_.isOK() )
    {
	if ( !istream_.isOK() )
	    mErrRet(uiStrings::sCantOpenInpFile())
	else
	    return ErrorOccurred();
    }

    const int trcbytes = fh_.nrBytesPerTrace();
    istream_.getBin( databuf_, trcbytes );
    if ( istream_.lastNrBytesRead() != trcbytes )
	return closeAll();

    fh_.fillInfo( trc_.info(), mCast(int,nrdone_) );
    trc_.info().sampling_.scale( zfac_ );

    for ( int ichan=0; ichan<fh_.nchan; ichan++ )
    {
	for ( int isamp=0; isamp<fh_.nsamp; isamp++ )
	    trc_.set( isamp, di_.get(databuf_,isamp)+fh_.zero, ichan );
	float firstrealval = trc_.get( 2, ichan );
	trc_.set( 0, firstrealval, ichan );
	trc_.set( 1, firstrealval, ichan );
    }

    if ( !wrr_->put(trc_) )
	mErrRet(wrr_->errMsg())

    nrdone_++;
    return istream_.isOK() ? MoreToDo() : closeAll();
}
