/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          July 2011
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";


#include "stratsynth.h"

#include "attribsel.h"
#include "attribengman.h"
#include "attribdesc.h"
#include "attribdescset.h"
#include "attribparam.h"
#include "attribprocessor.h"
#include "attribfactory.h"
#include "attribsel.h"
#include "attribstorprovider.h"
#include "binidvalset.h"
#include "datapackbase.h"
#include "elasticpropsel.h"
#include "flatposdata.h"
#include "ioman.h"
#include "prestackattrib.h"
#include "prestackgather.h"
#include "prestackanglecomputer.h"
#include "propertyref.h"
#include "raytracerrunner.h"
#include "separstr.h"
#include "seisbufadapters.h"
#include "seistrc.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "stratlayermodel.h"
#include "stratlayersequence.h"
#include "synthseis.h"
#include "timeser.h"
#include "wavelet.h"

static const char* sKeyIsPreStack()		{ return "Is Pre Stack"; }
static const char* sKeySynthType()		{ return "Synthetic Type"; }
static const char* sKeyWaveLetName()		{ return "Wavelet Name"; }
static const char* sKeyRayPar() 		{ return "Ray Parameter"; } 
static const char* sKeyInput()	 		{ return "Input Synthetic"; } 


DefineEnumNames(SynthGenParams,SynthType,0,"Synthetic Type")
{ "Pre Stack", "Zero Offset Stack", "Angle Stack", 0 };

SynthGenParams::SynthGenParams()
{
    const BufferStringSet& facnms = RayTracer1D::factory().getNames( false );
    if ( !facnms.isEmpty() )
	raypars_.set( sKey::Type(), facnms.get( facnms.size()-1 ) );

    RayTracer1D::setIOParsToZeroOffset( raypars_ );
}


bool SynthGenParams::hasOffsets() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return offsets.size()>1;
}


void SynthGenParams::fillPar( IOPar& par ) const
{
    par.set( sKey::Name(), name_ );
    par.set( sKeySynthType(), SynthGenParams::toString(synthtype_) );
    if ( synthtype_ == SynthGenParams::AngleStack )
	par.set( sKeyInput(), inpsynthnm_ );
    par.set( sKeyWaveLetName(), wvltnm_ );
    IOPar raypar;
    raypar.mergeComp( raypars_, sKeyRayPar() );
    par.merge( raypar );
}


void SynthGenParams::usePar( const IOPar& par ) 
{
    par.get( sKey::Name(), name_ );
    par.get( sKeyWaveLetName(), wvltnm_ );
    PtrMan<IOPar> raypar = par.subselect( sKeyRayPar() );
    raypars_ = *raypar;
    if ( par.hasKey( sKeyIsPreStack()) )
    {
	bool isps = false;
	par.getYN( sKeyIsPreStack(), isps );
	if ( !isps && hasOffsets() )
	    synthtype_ = SynthGenParams::AngleStack;
	else if ( !isps )
	    synthtype_ = SynthGenParams::ZeroOffset;
	else
	    synthtype_ = SynthGenParams::PreStack;
    }
    else
    {
	BufferString typestr;
	parseEnum( par, sKeySynthType(), synthtype_ );
	if ( synthtype_ == SynthGenParams::AngleStack )
	    par.get( sKeyInput(), inpsynthnm_ );
    }
}


void SynthGenParams::createName( BufferString& nm ) const
{
    nm = wvltnm_;
    TypeSet<float> offset; 
    raypars_.get( RayTracer1D::sKeyOffset(), offset );
    const int offsz = offset.size();
    if ( offsz )
    {
	nm += " ";
	nm += "Offset ";
	nm += ::toString( offset[0] );
	if ( offsz > 1 )
	    nm += "-"; nm += offset[offsz-1];
    }
}




StratSynth::StratSynth( const Strat::LayerModel& lm )
    : lm_(lm)
    , level_(0)  
    , tr_(0)
    , wvlt_(0)
    , lastsyntheticid_(0)
{}


StratSynth::~StratSynth()
{
    deepErase( synthetics_ );
    setLevel( 0 );
}


void StratSynth::setWavelet( const Wavelet* wvlt )
{
    if ( !wvlt ) 
	return;

    delete wvlt_; 
    wvlt_ = wvlt;
    genparams_.wvltnm_ = wvlt->name();
} 


void StratSynth::clearSynthetics()
{
    deepErase( synthetics_ );
}


#define mErrRet( msg, act )\
{\
    errmsg_ = "Can not generate synthetics ";\
    errmsg_ += synthgenpar.name_;\
    errmsg_ += " :\n";\
    errmsg_ += msg;\
    act;\
}

bool StratSynth::removeSynthetic( const char* nm )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	if ( synthetics_[idx]->name() == nm )
	{
	    delete synthetics_.removeSingle( idx );
	    return true;
	}
    }

    return false;
}


SyntheticData* StratSynth::addSynthetic()
{
    SyntheticData* sd = generateSD( lm_,tr_ );
    if ( sd )
	synthetics_ += sd;
    return sd;
}


SyntheticData* StratSynth::addSynthetic( const SynthGenParams& synthgen )
{
    SyntheticData* sd = generateSD( lm_, synthgen, tr_ );
    if ( sd )
	synthetics_ += sd;
    return sd;
}



SyntheticData* StratSynth::replaceSynthetic( int id )
{
    SyntheticData* sd = getSynthetic( id );
    if ( !sd ) return 0;

    const int sdidx = synthetics_.indexOf( sd );
    sd = generateSD( lm_, tr_ );
    if ( sd )
    {
	sd->setName( synthetics_[sdidx]->name() );
	delete synthetics_.replace( sdidx, sd );
    }

    return sd;
}


SyntheticData* StratSynth::addDefaultSynthetic()
{
    genparams_.synthtype_ = SynthGenParams::ZeroOffset;
    genparams_.createName( genparams_.name_ );
    SyntheticData* sd = addSynthetic();

    mDynamicCastGet(PostStackSyntheticData*,psd,sd);
    if ( psd )  generateOtherQuantities( *psd, lm_ );

    return sd;
}


SyntheticData* StratSynth::getSynthetic( const char* nm ) 
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( !strcmp( synthetics_[idx]->name(), nm ) )
	    return synthetics_[idx]; 
    }
    return 0;
}


SyntheticData* StratSynth::getSynthetic( int id ) 
{
    for ( int idx=0; idx<synthetics().size(); idx ++ )
    {
	if ( synthetics_[idx]->id_ == id )
	    return synthetics_[ idx ];
    }
    return 0;
}


SyntheticData* StratSynth::getSyntheticByIdx( int idx ) 
{
    return synthetics_.validIdx( idx ) ?  synthetics_[idx] : 0;
}



SyntheticData* StratSynth::getSynthetic( const  PropertyRef& pr )
{
    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	mDynamicCastGet(PropertyRefSyntheticData*,pssd,synthetics_[idx]);
	if ( !pssd ) continue;
	if ( pr == pssd->propRef() )
	    return pssd;
    }
    return 0;
}


int StratSynth::nrSynthetics() const 
{
    return synthetics_.size();
}


bool StratSynth::generate( const Strat::LayerModel& lm, SeisTrcBuf& trcbuf )
{
    SyntheticData* dummysd = generateSD( lm );
    if ( !dummysd ) 
	return false;

    for ( int idx=0; idx<lm.size(); idx ++ )
    {
	const SeisTrc* trc = dummysd->getTrace( idx );
	if ( trc )
	    trcbuf.add( new SeisTrc( *trc ) );
    }
    snapLevelTimes( trcbuf, dummysd->d2tmodels_ );

    delete dummysd;
    return !trcbuf.isEmpty();
}


float StratSynth::cMaximumVpWaterVel()
{
    return 1510.f;
}


SyntheticData* StratSynth::generateSD( const Strat::LayerModel& lm,
				       TaskRunner* tr )
{ return generateSD( lm, genparams_, tr ); }


#define mSetEnum( str, newval ) \
{ \
    mDynamicCastGet(Attrib::EnumParam*,param,psdesc->getValParam(str)) \
    param->setValue( newval ); \
}

#define mSetFloat( str, newval ) \
{ \
    Attrib::ValParam* param  = psdesc->getValParam( str ); \
    param->setValue( newval ); \
}


#define mSetString( str, newval ) \
{ \
    Attrib::ValParam* param = psdesc->getValParam( str ); \
    param->setValue( newval ); \
}

SyntheticData* StratSynth::createAngleStack( SyntheticData* sd,
					     const CubeSampling& cs,
       					     const SynthGenParams& synthgenpar,
       					     TaskRunner* tr )
{
    mDynamicCastGet(PreStackSyntheticData*,presd,sd);
    if ( !presd ) return 0;
    BufferString dpidstr( "#" );
    SeparString fullidstr( toString(DataPackMgr::CubeID()), '.' );
    const PreStack::GatherSetDataPack& gdp = presd->preStackPack();
    fullidstr.add( toString(gdp.id()) );
    dpidstr.add( fullidstr.buf() );

    Attrib::Desc* psdesc =
	Attrib::PF().createDescCopy(Attrib::PSAttrib::attribName());

    mSetString(Attrib::StorageProvider::keyStr(),dpidstr.buf());
    mSetFloat( Attrib::PSAttrib::offStartStr(),
	       presd->offsetRange().start );
    mSetFloat( Attrib::PSAttrib::offStopStr(),
	       presd->offsetRange().stop );
    mSetEnum(Attrib::PSAttrib::calctypeStr(),PreStack::PropCalc::Stats);
    mSetEnum(Attrib::PSAttrib::stattypeStr(), Stats::Average );
    psdesc->setUserRef( synthgenpar.name_ );
    psdesc->updateParams();

    PtrMan<Attrib::DescSet> descset = new Attrib::DescSet( false );
    if ( !descset ) return 0;

    Attrib::DescID attribid = descset->addDesc( psdesc );
    PtrMan<Attrib::EngineMan> aem = new Attrib::EngineMan;

    TypeSet<Attrib::SelSpec> attribspecs;
    Attrib::SelSpec sp( 0, attribid );
    sp.set( *psdesc );
    attribspecs += sp;

    aem->setAttribSet( descset );
    aem->setAttribSpecs( attribspecs );
    aem->setCubeSampling( cs );
    
    BinIDValueSet bidvals( 0, false );
    const ObjectSet<PreStack::Gather>& gathers = gdp.getGathers();
    for ( int idx=0; idx<gathers.size(); idx++ )
	bidvals.add( gathers[idx]->getBinID() );

    SeisTrcBuf* dptrcbufs = new SeisTrcBuf( true );
    Interval<float> zrg( cs.zrg );
    PtrMan<Attrib::Processor> proc =
	aem->createTrcSelOutput( errmsg_, bidvals, *dptrcbufs, 0, &zrg);
    if ( !proc ) 
	mErrRet( errmsg_, return 0 ) ;

    proc->getProvider()->setDesiredVolume( cs );
    proc->getProvider()->setPossibleVolume( cs );
    if ( !TaskRunner::execute(tr,*proc) )
	mErrRet( proc->message(), return 0 ) ;

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );
    for ( int trcidx=0; trcidx<dptrcbufs->size(); trcidx++ )
    {
	BinID bid = dptrcbufs->get( trcidx )->info().binid;
	dptrcbufs->get( trcidx )->info().nr =(bid.crl-bid0.crl)/crlstep;
    }

    SeisTrcBufDataPack* angledp =
	new SeisTrcBufDataPack( dptrcbufs, Seis::Line,
				SeisTrcInfo::TrcNr, synthgenpar.name_ );
    delete sd;
    return new AngleStackSyntheticData( synthgenpar, *angledp );
}

SyntheticData* StratSynth::generateSD( const Strat::LayerModel& lm,
				       const SynthGenParams& synthgenpar,
				       TaskRunner* tr )
{
    errmsg_.setEmpty(); 

    if ( lm.isEmpty() ) 
    {
	errmsg_ = "Empty layer model.";
	return 0;
    }

    Seis::RaySynthGenerator synthgen;
    BufferString capt( "Generating ", synthgenpar.name_ ); 
    synthgen.setName( capt.buf() );
    synthgen.setWavelet( wvlt_, OD::UsePtr );
    const IOPar& raypars = synthgenpar.raypars_;
    synthgen.usePar( raypars );

    const int nraimdls = lm.size();
    int maxsz = 0;
    for ( int idm=0; idm<nraimdls; idm++ )
    {
	ElasticModel aimod; 
	const Strat::LayerSequence& seq = lm.sequence( idm ); 
	const int sz = seq.size();
	if ( sz < 1 )
	    continue;

	if ( !fillElasticModel( lm, aimod, idm ) )
	{
	    BufferString msg( errmsg_ );
	    mErrRet( msg.buf(), return 0;) 
	}

	maxsz = mMAX( aimod.size(), maxsz );
	synthgen.addModel( aimod );
    }
    if ( maxsz == 0 )
	return 0;

    if ( maxsz == 1 )
	mErrRet( "Model has only one layer, please add another layer.", 
		return 0; );

    if ( !TaskRunner::execute( tr, synthgen) )
    {
	const char* errmsg = synthgen.errMsg();
	mErrRet( errmsg ? errmsg : "", return 0 ) ;
    }

    const int crlstep = SI().crlStep();
    const BinID bid0( SI().inlRange(false).stop + SI().inlStep(),
	    	      SI().crlRange(false).stop + crlstep );

    ObjectSet<SeisTrcBuf> tbufs;
    CubeSampling cs( false );
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	ObjectSet<SeisTrc> trcs; rm.getTraces( trcs, true );
	SeisTrcBuf* tbuf = new SeisTrcBuf( true );
	for ( int idx=0; idx<trcs.size(); idx++ )
	{
	    SeisTrc* trc = trcs[idx];
	    trc->info().binid = BinID( bid0.inl, bid0.crl + imdl * crlstep );
	    trc->info().nr = imdl;
	    cs.hrg.include( trc->info().binid );
	    if ( !trc->isEmpty() )
	    {
		SamplingData<float> sd = trc->info().sampling;
		StepInterval<float> zrg( sd.start,
					 sd.start+(sd.step*trc->size()),
					 sd.step );
		cs.zrg.include( zrg, false );
	    }

	    trc->info().coord = SI().transform( trc->info().binid );
	    tbuf->add( trc );
	}
	tbufs += tbuf;
    }

    SyntheticData* sd = 0;
    if ( synthgenpar.synthtype_ == SynthGenParams::PreStack ||
	 synthgenpar.synthtype_ == SynthGenParams::AngleStack )
    {
	ObjectSet<PreStack::Gather> gatherset;
	while ( tbufs.size() )
	{
	    SeisTrcBuf* tbuf = tbufs.removeSingle( 0 );
	    PreStack::Gather* gather = new PreStack::Gather();
	    if ( !gather->setFromTrcBuf( *tbuf, 0 ) )
		{ delete gather; continue; }

	    gatherset += gather;
	}

	PreStack::GatherSetDataPack* dp = 
	    new PreStack::GatherSetDataPack( synthgenpar.name_, gatherset );
	sd = new PreStackSyntheticData( synthgenpar, *dp );

	if ( synthgenpar.synthtype_ == SynthGenParams::AngleStack )
	    sd = createAngleStack( sd, cs, synthgenpar, tr );
	else
	{
	    mDynamicCastGet(PreStackSyntheticData*,presd,sd);
	    presd->createAngleData( synthgen.rayTracers(),
		    		    synthgen.elasticModels() );
	}
    }
    else if ( synthgenpar.synthtype_ == SynthGenParams::ZeroOffset )
    {
	SeisTrcBuf* dptrcbuf = new SeisTrcBuf( true );
	while ( tbufs.size() )
	{
	    SeisTrcBuf* tbuf = tbufs.removeSingle( 0 );
	    SeisTrcPropChg stpc( *tbuf->get( 0 ) );
	    while ( tbuf->size() > 1 )
	    {
		SeisTrc* trc = tbuf->remove( tbuf->size()-1 );
		stpc.stack( *trc );
		delete trc;
	    }
	    dptrcbuf->add( *tbuf );
	}
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( *dptrcbuf, Seis::Line,
				   SeisTrcInfo::TrcNr, synthgenpar.name_ );	
	sd = new PostStackSyntheticData( synthgenpar, *dp );
    }

    sd->id_ = ++lastsyntheticid_;

    ObjectSet<TimeDepthModel> tmpd2ts;
    for ( int imdl=0; imdl<nraimdls; imdl++ )
    {
	Seis::RaySynthGenerator::RayModel& rm = synthgen.result( imdl );
	rm.getD2T( tmpd2ts, true );
	if ( !tmpd2ts.isEmpty() )
	    sd->d2tmodels_ += tmpd2ts.removeSingle(0);
	deepErase( tmpd2ts );
    }

    return sd;
}


void StratSynth::generateOtherQuantities()
{
    if ( !synthetics_.size() ) return;

    for ( int idx=0; idx<synthetics_.size(); idx++ )
    {
	const SyntheticData* sd = synthetics_[idx];
	mDynamicCastGet(const PostStackSyntheticData*,pssd,sd);
	mDynamicCastGet(const PropertyRefSyntheticData*,prsd,sd);
	if ( !pssd || prsd ) continue;
	return generateOtherQuantities( *pssd, lm_ );
    }
}


void StratSynth::generateOtherQuantities( const PostStackSyntheticData& sd, 
					  const Strat::LayerModel& lm ) 
{
    const PropertyRefSelection& props = lm.propertyRefs();

    for ( int iprop=1; iprop<props.size(); iprop++ )
    {
	SeisTrcBufDataPack* dp = new SeisTrcBufDataPack( sd.postStackPack() );

	BufferString nm( "[" ); nm += props[iprop]->name(); nm += "]";

	const StepInterval<double>& zrg = dp->posData().range( false );

	SeisTrcBuf* trcbuf = new SeisTrcBuf( dp->trcBuf() );
	const int bufsz = trcbuf->size();

	for ( int iseq=0; iseq<lm.size(); iseq ++ )
	{
	    const Strat::LayerSequence& seq = lm.sequence( iseq ); 
	    const TimeDepthModel& t2d = *sd.d2tmodels_[iseq];

	    int layidx = 0;
	    float laydpt = seq.startDepth();
	    float val = mUdf(float);
	    TypeSet<float> vals; 
	    for ( int idz=0; idz<zrg.nrSteps()+1; idz++ )
	    {
		const float time = mCast( float, zrg.atIndex(idz) );
		const float dpt = t2d.getDepth( time );

		const Strat::Layer* lay = 0;
		while ( layidx < seq.size() - 1 )
		{
		    if ( dpt <= laydpt )
			break;

		    lay = seq.layers()[layidx];
		    laydpt += lay->thickness();
		    layidx ++;
		}
		if ( lay && iprop < lay->nrValues() )
		    val = lay->value( iprop );

		vals += val;
	    }
	    SeisTrc* trc = iseq < bufsz ? trcbuf->get( iseq ) : 0;
	    if ( !trc ) continue;
	    //Array1DImpl<float> outvals( vals.size() );
	    //AntiAlias( 1/(float)5, vals.size(), vals.arr(), outvals.arr() );
	    for ( int idz=0; idz<vals.size(); idz++ )
		trc->set( idz, vals[idz], 0 );
	}


	dp->setBuffer( trcbuf, Seis::Line, SeisTrcInfo::TrcNr );	
	dp->setName( nm );
	PropertyRefSyntheticData* prsd = 
	    new PropertyRefSyntheticData( genparams_, *dp, *props[iprop] );
	prsd->id_ = ++lastsyntheticid_;
	prsd->setName( nm );

	deepCopy( prsd->d2tmodels_, sd.d2tmodels_ );
	synthetics_ += prsd;
    }
}


const char* StratSynth::errMsg() const
{
    return errmsg_.isEmpty() ? 0 : errmsg_.buf();
}


bool StratSynth::fillElasticModel( const Strat::LayerModel& lm, 
				ElasticModel& aimodel, int seqidx )
{
    const Strat::LayerSequence& seq = lm.sequence( seqidx ); 
    const ElasticPropSelection& eps = lm.elasticPropSel();
    const PropertyRefSelection& props = lm.propertyRefs();
    if ( !eps.isValidInput(&errmsg_) )
	return false; 

    ElasticPropGen elpgen( eps, props );
    const float srddepth = -1*mCast(float,SI().seismicReferenceDatum() );
    int firstidx = 0;
    if ( seq.startDepth() < srddepth )
	firstidx = seq.nearestLayerIdxAtZ( srddepth );

    for ( int idx=firstidx; idx<seq.size(); idx++ )
    {
	const Strat::Layer* lay = seq.layers()[idx];
	float thickness = lay->thickness();
	if ( idx == firstidx )
	    thickness -= srddepth - lay->zTop();
	if ( thickness < 1e-4 )
	    continue;

	float dval, pval, sval;
	elpgen.getVals( dval, pval, sval, lay->values(), props.size() );
	const bool dudf = mIsUdf(dval); const bool pudf = mIsUdf(dval);
	const bool sudf = mIsUdf(dval);
	const int nrudf = (dudf?1:0) + (pudf?1:0) + (sudf?1:0);
	if ( nrudf > 0 )
	{
	    BufferString msg(
		    "Cannot derive layer propert", nrudf>1?"ies:":"y:" );
#	    define mAddUdfProp(t,s) if ( t##udf ) msg.add( " " ).add( s )
	    mAddUdfProp(d,"'Density'"); mAddUdfProp(p,"'P-wave velocity'");
	    mAddUdfProp(s,"'S-wave velocity'");
	    msg.add( ".\nPlease check the definition" );
	    if ( nrudf > 1 ) msg.add( "s" );
	    errmsg_ = msg; return false;
	}

	// Detect water - reset Vs
	if ( pval < cMaximumVpWaterVel() )
	    sval = 0;

	aimodel += ElasticLayer( thickness, pval, sval, dval );
    }

    return true;
}


void StratSynth::snapLevelTimes( SeisTrcBuf& trcs, 
			const ObjectSet<const TimeDepthModel>& d2ts ) 
{
    if ( !level_ ) return;

    TypeSet<float> times = level_->zvals_;
    for ( int imdl=0; imdl<times.size(); imdl++ )
	times[imdl] = d2ts.validIdx(imdl) && !mIsUdf(times[imdl]) ? 
	    	d2ts[imdl]->getTime( times[imdl] ) : mUdf(float);

    for ( int idx=0; idx<trcs.size(); idx++ )
    {
	const SeisTrc& trc = *trcs.get( idx );
	SeisTrcPropCalc stp( trc );
	float z = times.validIdx( idx ) ? times[idx] : mUdf( float );
	trcs.get( idx )->info().zref = z;
	if ( !mIsUdf( z ) && level_->snapev_ != VSEvent::None )
	{
	    Interval<float> tg( z, trc.startPos() );
	    mFlValSerEv ev1 = stp.find( level_->snapev_, tg, 1 );
	    tg.start = z; tg.stop = trc.endPos();
	    mFlValSerEv ev2 = stp.find( level_->snapev_, tg, 1 );
	    float tmpz = ev2.pos;
	    const bool ev1invalid = mIsUdf(ev1.pos) || ev1.pos < 0;
	    const bool ev2invalid = mIsUdf(ev2.pos) || ev2.pos < 0;
	    if ( ev1invalid && ev2invalid )
		continue;
	    else if ( ev2invalid )
		tmpz = ev1.pos;
	    else if ( fabs(z-ev1.pos) < fabs(z-ev2.pos) )
		tmpz = ev1.pos;

	    z = tmpz;
	}
	trcs.get( idx )->info().pick = z;
    }
}


void StratSynth::setLevel( const Level* lvl )
{ delete level_; level_ = lvl; }



void StratSynth::flattenTraces( SeisTrcBuf& tbuf ) const
{
    if ( tbuf.isEmpty() )
	return;

    float tmax = tbuf.get(0)->info().sampling.start;
    float tmin = tbuf.get(0)->info().sampling.atIndex( tbuf.get(0)->size() );
    for ( int idx=tbuf.size()-1; idx>=1; idx-- )
    {
	if ( mIsUdf(tbuf.get(idx)->info().pick) ) continue;
	tmin = mMIN(tmin,tbuf.get(idx)->info().pick);
	tmax = mMAX(tmax,tbuf.get(idx)->info().pick);
    }

    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	const float start = trc->info().sampling.start - tmax;
	const float stop  = trc->info().sampling.atIndex(trc->size()-1) -tmax;
	SeisTrc* newtrc = trc->getRelTrc( ZGate(start,stop) );
	if ( !newtrc )
	{
	    newtrc = new SeisTrc( *trc );
	    newtrc->zero();
	}

	delete tbuf.replace( idx, newtrc );
    }
}	


void StratSynth::decimateTraces( SeisTrcBuf& tbuf, int fac ) const
{
    for ( int idx=tbuf.size()-1; idx>=0; idx-- )
    {
	if ( idx%fac )
	    delete tbuf.remove( idx ); 
    }
}


SyntheticData::SyntheticData( const SynthGenParams& sgp, DataPack& dp )
    : NamedObject(sgp.name_)
    , datapack_(dp)
    , id_(-1) 
{
}


SyntheticData::~SyntheticData()
{
    deepErase( d2tmodels_ );
    removePack(); 
}


void SyntheticData::setName( const char* nm )
{
    NamedObject::setName( nm );
    datapack_.setName( nm );
}


void SyntheticData::removePack()
{
    const DataPack::FullID dpid = datapackid_;
    DataPackMgr::ID packmgrid = DataPackMgr::getID( dpid );
    const DataPack* dp = DPM(packmgrid).obtain( DataPack::getID(dpid) );
    if ( dp )
	DPM(packmgrid).release( dp->id() );
}


float SyntheticData::getTime( float dpt, int seqnr ) const
{
    return d2tmodels_.validIdx( seqnr ) ? d2tmodels_[seqnr]->getTime( dpt ) 
					: mUdf( float );
}


float SyntheticData::getDepth( float time, int seqnr ) const
{
    return d2tmodels_.validIdx( seqnr ) ? d2tmodels_[seqnr]->getDepth( time ) 
					: mUdf( float );
}
 

PostStackSyntheticData::PostStackSyntheticData( const SynthGenParams& sgp,
						SeisTrcBufDataPack& dp)
    : SyntheticData(sgp,dp)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::FlatID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PostStackSyntheticData::~PostStackSyntheticData()
{
}


const SeisTrc* PostStackSyntheticData::getTrace( int seqnr ) const
{ return postStackPack().trcBuf().get( seqnr ); }



PreStackSyntheticData::PreStackSyntheticData( const SynthGenParams& sgp,
					     PreStack::GatherSetDataPack& dp)
    : SyntheticData(sgp,dp)
    , angledp_(0)
{
    useGenParams( sgp );
    DataPackMgr::ID pmid = DataPackMgr::CubeID();
    DPM( pmid ).add( &dp );
    datapackid_ = DataPack::FullID( pmid, dp.id());
}


PreStackSyntheticData::~PreStackSyntheticData()
{
    if ( angledp_ )
	DPM( DataPackMgr::CubeID() ).release( angledp_->id() );
}


void PreStackSyntheticData::convertAngleDataToDegrees( PreStack::Gather* ag ) const
{
    Array2D<float>& agdata = ag->data();
    const int dim0sz = agdata.info().getSize(0);
    const int dim1sz = agdata.info().getSize(1);
    for ( int idx=0; idx<dim0sz; idx++ )
    {
	for ( int idy=0; idy<dim1sz; idy++ )
	{
	    const float radval = agdata.get( idx, idy );
	    if ( mIsUdf(radval) ) continue;
	    const float dval = Math::toDegrees(radval);
	    agdata.set( idx, idy, dval );
	}
    }
}


void PreStackSyntheticData::createAngleData( const ObjectSet<RayTracer1D>& rts,
					     const TypeSet<ElasticModel>& ems ) 
{
    if ( angledp_ ) DPM( DataPackMgr::CubeID() ).release( angledp_->id() );
    ObjectSet<PreStack::Gather> anglegathers;
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    PreStack::ModelBasedAngleComputer anglecomp;
    for ( int idx=0; idx<rts.size(); idx++ )
    {
	if ( !gathers.validIdx(idx) )
	    continue;
	const PreStack::Gather* gather = gathers[idx];
	anglecomp.setOutputSampling( gather->posData() );
	anglecomp.setElasticModel( ems[idx], false, false ); 
	anglecomp.setRayTracer( rts[idx] );
	PreStack::Gather* anglegather = anglecomp.computeAngles();
	convertAngleDataToDegrees( anglegather );
	TypeSet<float> azimuths;
	gather->getAzimuths( azimuths );
	anglegather->setAzimuths( azimuths );
	anglegathers += anglegather;
    }

    angledp_ = new PreStack::GatherSetDataPack( name(), anglegathers );
    DPM( DataPackMgr::CubeID() ).add( angledp_ );
}


const Interval<float> PreStackSyntheticData::offsetRange() const
{
    Interval<float> offrg( 0, 0 );
    const ObjectSet<PreStack::Gather>& gathers = preStackPack().getGathers();
    if ( !gathers.isEmpty() ) 
    {
	const PreStack::Gather& gather = *gathers[0];
	offrg.set(gather.getOffset(0),gather.getOffset( gather.size(true)-1));
    }
    return offrg;
}


bool PreStackSyntheticData::hasOffset() const
{ return offsetRange().width() > 0; }


const SeisTrc* PreStackSyntheticData::getTrace( int seqnr, int* offset ) const
{ return preStackPack().getTrace( seqnr, offset ? *offset : 0 ); }


SeisTrcBuf* PreStackSyntheticData::getTrcBuf( float offset, 
					const Interval<float>* stackrg ) const
{
    SeisTrcBuf* tbuf = new SeisTrcBuf( true );
    Interval<float> offrg = stackrg ? *stackrg : Interval<float>(offset,offset);
    preStackPack().fill( *tbuf, offrg );
    return tbuf;
}


AngleStackSyntheticData::AngleStackSyntheticData( const SynthGenParams& sgp,
						  SeisTrcBufDataPack& sdp )
    : PostStackSyntheticData(sgp,sdp)
{
    useGenParams( sgp );
}


AngleStackSyntheticData::~AngleStackSyntheticData()
{}


void AngleStackSyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    SyntheticData::fillGenParams( sgp );
    sgp.inpsynthnm_ = inpsynthnm_;
}


void AngleStackSyntheticData::useGenParams( const SynthGenParams& sgp )
{
    SyntheticData::useGenParams( sgp );
    inpsynthnm_ = sgp.inpsynthnm_;
}


PropertyRefSyntheticData::PropertyRefSyntheticData( const SynthGenParams& sgp,
						    SeisTrcBufDataPack& dp,
						    const PropertyRef& pr )
    : PostStackSyntheticData( sgp, dp ) 
    , prop_(pr)
{}


bool SyntheticData::isAngleStack() const
{
    TypeSet<float> offsets;
    raypars_.get( RayTracer1D::sKeyOffset(), offsets );
    return !isPS() && offsets.size()>1;
}


void SyntheticData::fillGenParams( SynthGenParams& sgp ) const
{
    sgp.raypars_ = raypars_;
    sgp.wvltnm_ = wvltnm_;
    sgp.name_ = name();
    sgp.synthtype_ = synthType();
}


void SyntheticData::useGenParams( const SynthGenParams& sgp )
{
    raypars_ = sgp.raypars_;
    wvltnm_ = sgp.wvltnm_;
    setName( sgp.name_ );
}
