/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "emhorizon2d.h"

#include "arrayndimpl.h"
#include "emsurfacetr.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "horizon2dline.h"
#include "ioman.h"
#include "selector.h"
#include "toplist.h"
#include "survinfo.h"
#include "survgeom2d.h"
#include "tabledef.h"
#include "unitofmeasure.h"
#include "zaxistransform.h"

namespace EM
{

Horizon2DGeometry::Horizon2DGeometry( Surface& surface )
    : HorizonGeometry( surface )
{}


Geometry::Horizon2DLine*
Horizon2DGeometry::sectionGeometry( const SectionID& sid )
{
    return (Geometry::Horizon2DLine*)SurfaceGeometry::sectionGeometry( sid );
}


const Geometry::Horizon2DLine*
Horizon2DGeometry::sectionGeometry( const SectionID& sid ) const
{
    return (const Geometry::Horizon2DLine*)
	SurfaceGeometry::sectionGeometry( sid );
}


int Horizon2DGeometry::nrLines() const
{ return l2dkeys_.size(); }


int Horizon2DGeometry::lineIndex( const PosInfo::Line2DKey& l2dkey ) const
{ return l2dkeys_.indexOf( l2dkey ); }


int Horizon2DGeometry::lineIndex( Pos::GeomID geomid ) const
{ return geomids_.indexOf( geomid ); }


int Horizon2DGeometry::lineIndex( const char* linenm ) const
{
    for ( int idx=0; idx<l2dkeys_.size(); idx++ )
    {
	if( S2DPOS().curLineSetID() != l2dkeys_[idx].lsID() )
	    S2DPOS().setCurLineSet( l2dkeys_[idx].lsID() );
	BufferString lnm = S2DPOS().getLineName( l2dkeys_[idx].lineID() );
	if ( lnm == linenm )
	    return idx;
    }

    return -1;
}


const char* Horizon2DGeometry::lineName( int lid ) const
{
    const PosInfo::Line2DKey l2dkey = lineKey( lid );
    if ( !l2dkey.isOK() ) return 0;

    if( S2DPOS().curLineSetID() != l2dkey.lsID() )
	S2DPOS().setCurLineSet( l2dkey.lsID() );
    return S2DPOS().getLineName( l2dkey.lineID() );
}


const char* Horizon2DGeometry::lineSet( int lid ) const
{
    const PosInfo::Line2DKey l2dkey = lineKey( lid );
    return l2dkey.isOK() ? S2DPOS().getLineSet( l2dkey.lsID() ) : 0;
}


PosInfo::Line2DKey Horizon2DGeometry::lineKey( int idx ) const
{
    return l2dkeys_.validIdx(idx) ? l2dkeys_[idx] : PosInfo::Line2DKey(-1,-1);
}


Pos::GeomID Horizon2DGeometry::geomID( int idx ) const
{
    return geomids_.validIdx( idx ) ? geomids_[idx] : -1;
}


bool Horizon2DGeometry::includeLine( const PosInfo::Line2DKey& l2dkey,int step )
{ return doAddLine( l2dkey, StepInterval<int>(mUdf(int),mUdf(int),step), 
								    true ); }


bool Horizon2DGeometry::includeLine( Pos::GeomID geomid, int step )
{ return doAddLine( geomid, StepInterval<int>(mUdf(int),mUdf(int),step), 
								    true ); }


bool Horizon2DGeometry::addLine( const PosInfo::Line2DKey& l2dkey, int step )
{ return doAddLine(l2dkey, StepInterval<int>(mUdf(int),mUdf(int),step), 
								    false); }


bool Horizon2DGeometry::addLine( Pos::GeomID geomid, int step )
{ return doAddLine(geomid, StepInterval<int>(mUdf(int),mUdf(int),step), 
								    false); }


bool Horizon2DGeometry::addLine( const PosInfo::Line2DKey& l2dkey,
				 const StepInterval<int>& trg )
{ return doAddLine( l2dkey, trg, false ); }


bool Horizon2DGeometry::addLine( Pos::GeomID geomid,
				 const StepInterval<int>& trg )
{ return doAddLine( geomid, trg, false ); }


bool Horizon2DGeometry::doAddLine( Pos::GeomID geomid,
				   const StepInterval<int>& inptrg, 
				   bool mergewithdouble )
{
    if ( geomids_.isPresent(geomid) )
	return false;

    const PosInfo::Line2DData linegeom( ((Survey::Geometry2D*)Survey::
					GM().getGeometry(geomid))->data() );
    if ( linegeom.isEmpty() )
	return false;

    StepInterval<int> trcrg = inptrg.isUdf() ? linegeom.trcNrRange() : inptrg;
    Geometry::Horizon2DLine* h2dl =
		    reinterpret_cast<Geometry::Horizon2DLine*>( sections_[0] );
    int oldgeomidx = -1;
    for ( int geomidx=0; mergewithdouble && geomidx<geomids_.size(); geomidx++ )
    {
	const int currow = h2dl->getRowIndex( geomids_[geomidx] );
	StepInterval<int> trg = h2dl->colRange( currow );
	trg.limitTo( trcrg );

	const Coord cur0 = h2dl->getKnot( RowCol(currow,trg.start) );
	const Coord cur1 = h2dl->getKnot( RowCol(currow,trg.stop) );
	if ( !trg.width() || !cur0.isDefined() || !cur1.isDefined() )
	    continue;

	PosInfo::Line2DPos new0; linegeom.getPos( trg.start, new0 );
	PosInfo::Line2DPos new1; linegeom.getPos( trg.stop, new1 );
	if ( !new0.coord_.isDefined() || !new1.coord_.isDefined() )
	    continue;

	const float maxdist = (float) (0.1 * cur0.distTo(cur1) / trg.width());
	if ( cur0.distTo(new0.coord_)>maxdist ||
	     cur1.distTo(new1.coord_)>maxdist )
	    continue;

	oldgeomidx = geomidx;
    }

    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	h2dl = reinterpret_cast<Geometry::Horizon2DLine*>( sections_[idx] );

	if ( oldgeomidx < 0 )
	    h2dl->addUdfRow( geomid, trcrg.start, trcrg.stop, trcrg.step );
	else
	    h2dl->reassignRow( geomids_[oldgeomidx], geomid );

	h2dl->syncRow( geomid, linegeom );
    }

    if ( oldgeomidx < 0 )
	geomids_ += geomid;
    else
	geomids_[oldgeomidx] = geomid;

    return true;
}


bool Horizon2DGeometry::doAddLine( const PosInfo::Line2DKey& l2dkey,
				 const StepInterval<int>& inptrg,
				 bool mergewithdouble )
{
    if ( !l2dkey.isOK() || l2dkeys_.isPresent(l2dkey) )
	return false;

    if( S2DPOS().curLineSetID() != l2dkey.lsID() )
	S2DPOS().setCurLineSet( l2dkey.lsID() );
    PosInfo::Line2DData linegeom( S2DPOS().getLineName(l2dkey.lineID()) );
    if ( !S2DPOS().getGeometry(linegeom) )
	return false;

    StepInterval<int> trcrg = inptrg.isUdf() ? linegeom.trcNrRange() : inptrg;
    Geometry::Horizon2DLine* h2dl =
		    reinterpret_cast<Geometry::Horizon2DLine*>( sections_[0] );

    int oldgeomidx = -1;
    for ( int keyidx=0; mergewithdouble && keyidx<l2dkeys_.size(); keyidx++ )
    {
	const int currow = h2dl->getRowIndex( l2dkeys_[keyidx] );
	StepInterval<int> trg = h2dl->colRange( currow );
	trg.limitTo( trcrg );

	const Coord cur0 = h2dl->getKnot( RowCol(currow,trg.start) );
	const Coord cur1 = h2dl->getKnot( RowCol(currow,trg.stop) );
	if ( !trg.width() || !cur0.isDefined() || !cur1.isDefined() )
	    continue;

	PosInfo::Line2DPos new0; linegeom.getPos( trg.start, new0 );
	PosInfo::Line2DPos new1; linegeom.getPos( trg.stop, new1 );
	if ( !new0.coord_.isDefined() || !new1.coord_.isDefined() )
	    continue;

	const float maxdist = (float) (0.1 * cur0.distTo(cur1) / trg.width());
	if ( cur0.distTo(new0.coord_)>maxdist ||
	     cur1.distTo(new1.coord_)>maxdist )
	    continue;

	oldgeomidx = keyidx;
    }

    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	h2dl = reinterpret_cast<Geometry::Horizon2DLine*>( sections_[idx] );

	if ( oldgeomidx < 0 )
	    h2dl->addUdfRow( l2dkey, trcrg.start, trcrg.stop, trcrg.step );
	else
	    h2dl->reassignRow( l2dkeys_[oldgeomidx], l2dkey );

        h2dl->syncRow( l2dkey, linegeom );
    }

    if ( oldgeomidx < 0 )
	l2dkeys_ += l2dkey;
    else
	l2dkeys_[oldgeomidx] = l2dkey;

    return true;
}


void Horizon2DGeometry::removeLine( const PosInfo::Line2DKey& l2dkey )
{
    const int lidx = l2dkeys_.indexOf( l2dkey );
    if ( lidx < 0 )
	return;

    l2dkeys_.removeSingle( lidx );
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	section->removeRow( l2dkey );
    }
}


void Horizon2DGeometry::removeLine( Pos::GeomID geomid )
{
    const int lidx = geomids_.indexOf( geomid );
    if ( lidx < 0 )
	return;

    geomids_.removeSingle( lidx );
    for ( int idx=sections_.size()-1; idx>=0; idx-- )
    {
	Geometry::Horizon2DLine* section =
	    reinterpret_cast<Geometry::Horizon2DLine*>(sections_[idx]);
	section->removeRow( geomid );
    }
}


PosID Horizon2DGeometry::getNeighbor( const PosID& pid, bool nextcol,
				      bool retundef ) const
{
    TypeSet<PosID> aliases;
    getLinkedPos( pid, aliases );
    aliases += pid;

    const int nraliases = aliases.size();
    for ( int idx=0; idx<nraliases; idx++ )
    {
	const SectionID sid = aliases[idx].sectionID();
	const RowCol ownrc = aliases[idx].getRowCol();
	const int colstep = colRange( sid, ownrc.row() ).step;
	const RowCol neighborrc( ownrc.row(),
		nextcol ? ownrc.col()+colstep : ownrc.col()-colstep );

	if ( surface_.isDefined( sid, neighborrc.toInt64() ) ||
	     (!retundef && idx==nraliases-1) )
	    return PosID( surface_.id(), sid, neighborrc.toInt64() );
    }

    return PosID::udf();
}


int Horizon2DGeometry::getConnectedPos( const PosID& pid,
					TypeSet<PosID>* res ) const
{
    int nrres = 0;
    PosID neighborpid = getNeighbor( pid, true, true );
    if ( neighborpid.objectID()!=-1 )
    {
	nrres++;
	if ( res ) (*res) += neighborpid;
    }

    neighborpid = getNeighbor( pid, false, true );
    if ( neighborpid.objectID()!=-1 )
    {
	nrres++;
	if ( res ) (*res) += neighborpid;
    }

    return nrres;
}


bool Horizon2DGeometry::isAtEdge( const PosID& pid ) const
{
    return getConnectedPos( pid, 0 ) != 2;
}


Geometry::Horizon2DLine* Horizon2DGeometry::createSectionGeometry() const
{ return new Geometry::Horizon2DLine; }


StepInterval<int> Horizon2DGeometry::colRange( const SectionID& sid,
					const PosInfo::Line2DKey& l2dkey ) const
{
    const Geometry::Horizon2DLine* geom = sectionGeometry( sectionID(sid) );
    return geom ? geom->colRange( l2dkey ) : StepInterval<int>(0,0,0);
}


StepInterval<int> Horizon2DGeometry::colRange( const SectionID& sid,
					       Pos::GeomID geomid ) const
{
    const Geometry::Horizon2DLine* geom = sectionGeometry( sectionID(sid) );
    return geom ? geom->colRange( geomid ) : StepInterval<int>(0,0,0);
}


StepInterval<int> Horizon2DGeometry::colRange( const PosInfo::Line2DKey& gid) const
{
    StepInterval<int> res(0,0,0);
    bool isset = false;

    for ( int idx=0; idx<nrSections(); idx++ )
    {
	StepInterval<int> sectionrg = colRange( sids_[idx], gid );
	if ( sectionrg.start>sectionrg.stop )
	    continue;
	if ( !isset ) { res = sectionrg; isset=true; }
	else res.include( sectionrg );
    }

    return res;
}


StepInterval<int> Horizon2DGeometry::colRange( Pos::GeomID geomid) const
{
    StepInterval<int> res(0,0,0);
    bool isset = false;

    for ( int idx=0; idx<nrSections(); idx++ )
    {
	StepInterval<int> sectionrg = colRange( sids_[idx], geomid );
	if ( sectionrg.start>sectionrg.stop )
	    continue;
	if ( !isset ) { res = sectionrg; isset=true; }
	else res.include( sectionrg );
    }

    return res;
}


void Horizon2DGeometry::fillPar( IOPar& iopar ) const
{
    const Geometry::Horizon2DLine* cgeom = sectionGeometry( sectionID(0) );
    if ( !cgeom ) return;

    Geometry::Horizon2DLine* geom = cgeom->clone();
    geom->trimUndefParts();
    for ( int idx=0; idx<l2dkeys_.size(); idx++ )
    {
	BufferString key = IOPar::compKey( "Line", idx );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
		   l2dkeys_[idx].toString() );
	const int rowidx = geom->getRowIndex( l2dkeys_[idx] );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		   geom->colRange(rowidx) );
    }

    iopar.set( Horizon2DGeometry::sKeyNrLines(), l2dkeys_.size() );

    delete geom;
}


bool Horizon2DGeometry::usePar( const IOPar& par )
{
    l2dkeys_.erase();

    if ( par.find(sKey::GeomID()) )
    {
	int nrlines = 0;
	par.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( sKey::GeomID(), idx );

	    int geomid;
	    par.get( key, geomid );
	    geomids_ += geomid;
	    PosInfo::Line2DData linegeom( ((Survey::Geometry2D*)Survey::GM().
						getGeometry(geomid))->data() );
	    if ( linegeom.isEmpty() )
		continue;

	    for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	    {
		Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
		section->syncRow( geomid, linegeom );
	    }
	}

	return true;
    }

    if ( par.find(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	par.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );

	    BufferString idstr;
	    par.get( IOPar::compKey(key,Horizon2DGeometry::sKeyID()), idstr );
	    PosInfo::Line2DKey oldgeomid; oldgeomid.fromString( idstr );
	    l2dkeys_ += oldgeomid;
	    if( S2DPOS().curLineSetID() != oldgeomid.lsID() )
		S2DPOS().setCurLineSet( oldgeomid.lsID() );
	    PosInfo::Line2DData linegeom(
		    	S2DPOS().getLineName(oldgeomid.lineID()));
	    if ( !S2DPOS().getGeometry(linegeom) )
		continue;

	    for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	    {
		Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
		section->syncRow( oldgeomid, linegeom );
	    }
	}

	return true;
    }

    TypeSet<int> lineids;
    if ( !par.get(sKeyLineIDs(),lineids) )
	return false;
    BufferStringSet linenames;
    if ( !par.get(sKeyLineNames(),linenames)  )
     	return false;

    for ( int idx=0; idx<lineids.size(); idx++ )
    {
	BufferString linesetkey = sKeyLineSets();
	linesetkey += idx;

	MultiID mid;
	if ( !par.get(linesetkey.buf(),mid) ) continue;

	PtrMan<IOObj> ioobj = IOM().get( mid );
	if ( !ioobj ) continue;

	PosInfo::Line2DKey geomid = S2DPOS().getLine2DKey( ioobj->name(),
						     linenames[idx]->buf() );
	if ( !geomid.isOK() ) continue;
	l2dkeys_ += geomid;

	PosInfo::Line2DData linegeom( linenames[idx]->buf() );
	if ( !S2DPOS().getGeometry(linegeom) )
	    continue;

	for ( int secidx=sections_.size()-1; secidx>=0; secidx-- )
	{
	    Geometry::Horizon2DLine* section =
		reinterpret_cast<Geometry::Horizon2DLine*>( sections_[secidx] );
	    section->syncRow( geomid, linegeom );
	}
    }

    return true;
}


mImplementEMObjFuncs( Horizon2D, EMHorizon2DTranslatorGroup::keyword() )


Horizon2D::Horizon2D( EMManager& emm )
    : Horizon(emm)
    , geometry_(*this)
{
    geometry_.addSection( "", false );
}


Horizon2D::~Horizon2D()
{}


float Horizon2D::getZValue( const Coord& c, bool allow_udf, int nr ) const
{
    const int sectionidx = nr;

    const EM::SectionID sectionid = sectionID( sectionidx );
    const Geometry::Horizon2DLine* line =
	geometry().sectionGeometry( sectionid );

    if ( !line )
	return allow_udf ? mUdf(float) : 0;

    TopList<double,double> closestpoints( 2 );

    const StepInterval<int> rowrg = line->rowRange();
    const int nrrows = rowrg.nrSteps()+1;
    for ( int rowidx=0; rowidx<nrrows; rowidx++ )
    {
	StepInterval<int> colrg = line->colRange( rowidx );
	RowCol rowcol( rowrg.atIndex( rowidx ), 0 );

	for ( rowcol.col()=colrg.start; rowcol.col()<=colrg.stop;
	      rowcol.col()+= colrg.step )
	{
	    const Coord3 knot = line->getKnot( rowcol );
	    if ( !knot.isDefined() )
		continue;

	    const double sqdist = c.sqDistTo( knot );
	    if ( mIsZero(sqdist,1e-3) )
		return (float) knot.z;

	    closestpoints.addValue( -sqdist, knot.z );
	}
    }

    if ( closestpoints.isEmpty() )
	return allow_udf ? mUdf(float) : 0;

    if ( closestpoints.size()==1 )
	return (float) closestpoints.getAssociatedValue( 0 );

    const double z0 = closestpoints.getAssociatedValue( 0 );
    const double dist0 = Math::Sqrt( -closestpoints.getValue( 0 ) );
    const double z1 = closestpoints.getAssociatedValue( 1 );
    const double dist1 = Math::Sqrt( -closestpoints.getValue( 1 ) );

    return (float) ((dist1*z0+dist0*z1)/(dist0+dist1));
}


void Horizon2D::removeAll()
{
    Surface::removeAll();
    geometry_.removeAll();
}


void Horizon2D::removeSelected( const Selector<Coord3>& selector,
				TaskRunner* tr )
{
    if ( !selector.isOK() )
	return;

    removebypolyposbox_.setEmpty();
    insideselremoval_ = true;

    for ( int idx=0; idx<nrSections(); idx++ )
    {
	const Geometry::Element* ge = sectionGeometry( sectionID(idx) );
	if ( !ge ) continue;

	TypeSet<EM::SubID> removallist;

	PtrMan<EM::EMObjectIterator> iterator = createIterator( -1 );
	while ( true )
	{
	    const EM::PosID pid = iterator->next();
	    if ( pid.objectID()==-1 )
		break;

	    const Coord3 pos = getPos(pid);
	    if ( selector.includes(pos) )
		removallist += pid.subID();
	}

	removeListOfSubIDs( removallist, sectionID(idx) );
    }
    insideselremoval_ = false;
}


bool Horizon2D::unSetPos( const PosID& pid, bool addtoundo )
{
    Coord3 pos = getPos( pid );
    pos.z = mUdf(float);
    return EMObject::setPos( pid, pos, addtoundo );
}


bool Horizon2D::unSetPos( const EM::SectionID& sid, const EM::SubID& subid,
			  bool addtoundo )
{
    Coord3 pos = getPos( sid, subid );
    pos.z = mUdf(float);
    return EMObject::setPos( sid, subid, pos, addtoundo );
}

Coord3 Horizon2D::getPos( EM::SectionID sid, const PosInfo::Line2DKey& geomid,
			  int trcnr ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return Coord3::udf();

    const int lineidx = geom->getRowIndex( geomid );
    RowCol rc( lineidx, trcnr );
    return geom->getKnot( rc );
}


Coord3 Horizon2D::getPosition( EM::SectionID sid, Pos::GeomID geomid,
			       int trcnr ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return Coord3::udf();

    const int lineidx = geom->getRowIndex( geomid );
    RowCol rc( lineidx, trcnr );
    return geom->getKnot( rc );
}


bool Horizon2D::setPos( const EM::PosID& posid, const Coord3& pos,
			bool addtohistory )
{
    return EMObject::setPos( posid, pos, addtohistory );
}


bool Horizon2D::setPos( const EM::SectionID& sid, const EM::SubID& subid,
			const Coord3& pos, bool addtohistory )
{
    return EMObject::setPos( sid, subid, pos, addtohistory );
}


bool Horizon2D::setPos( EM::SectionID sid, const PosInfo::Line2DKey& geomid,
			int trcnr, float z, bool addtohistory )
{
    Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const int lineidx = geom->getRowIndex( geomid );
    if ( mIsUdf(lineidx) || lineidx<0 ) return false;

    EM::SubID subid = BinID( lineidx, trcnr ).toInt64();
    Coord3 newpos = EMObject::getPos( sid, subid );
    newpos.z = z;
    return EMObject::setPos( sid, subid, newpos, addtohistory );
}


bool Horizon2D::setPos( EM::SectionID sid, Pos::GeomID geomid, int trcnr,
			float z, bool addtohistory )
{
    Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const int lineidx = geom->getRowIndex( geomid );
    if ( mIsUdf(lineidx) || lineidx<0 ) return false;

    EM::SubID subid = BinID( lineidx, trcnr ).toInt64();
    Coord3 newpos = EMObject::getPos( sid, subid );
    newpos.z = z;
    return EMObject::setPos( sid, subid, newpos, addtohistory );
}


Coord3 Horizon2D::getPos( const EM::PosID& pid ) const
{ return EMObject::getPos(pid); }

Coord3 Horizon2D::getPos( const EM::SectionID& sid, const EM::SubID& sub ) const
{ return EMObject::getPos(sid,sub); }


Coord3 Horizon2D::getPos( EM::SectionID sid, int lineidx, int trcnr ) const
{
    return getPos( sid, RowCol(lineidx,trcnr).toInt64() );
}


TypeSet<Coord3> Horizon2D::getPositions( int lineidx, int trcnr ) const
{
    TypeSet<Coord3> crds;
    for ( int idx=0; idx<nrSections(); idx++ )
	crds += getPos( sectionID(idx), RowCol(lineidx,trcnr).toInt64() );
    return crds;
}


bool Horizon2D::setArray1D( const Array1D<float>& arr,
			    SectionID sid, const PosInfo::Line2DKey& geomid,
			    bool onlyfillundefs )
{
    const StepInterval<int> trcrg = geometry_.colRange( geomid );
    return setArray1D( arr, trcrg, sid, geomid, onlyfillundefs );
}


bool Horizon2D::setArray1D( const Array1D<float>& arr,
			    const StepInterval<int>& trcrg,
			    SectionID sid, const PosInfo::Line2DKey& geomid,
			    bool onlyfillundefs )
{
    Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const int lineidx = geom->getRowIndex( geomid );
//    const StepInterval<int> colrg = geom->colRange( lineidx );
//  Should we use this colrg?
    for ( int col=trcrg.start; col<=trcrg.stop; col+=trcrg.step )
    {
	RowCol rc( lineidx, col );
	Coord3 pos = geom->getKnot( rc );
	if ( pos.isDefined() && onlyfillundefs )
	    continue;

	if ( arr.info().validPos(trcrg.getIndex(col)) )
	{
	    float z = arr.get( trcrg.getIndex(col) );
	    pos.z = z;
	    geom->setKnot( rc, pos );
	}
    }

    return true;
}


bool Horizon2D::setArray1D( const Array1D<float>& arr, SectionID sid,
			    Pos::GeomID geomid, bool onlyfillundefs )
{
    Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    const int lineidx = geom->getRowIndex( geomid );
    const StepInterval<int> colrg = geom->colRange( lineidx );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	RowCol rc( lineidx, col );
	Coord3 pos = geom->getKnot( rc );
	if ( pos.isDefined() && onlyfillundefs )
	    continue;

	if ( arr.info().validPos(colrg.getIndex(col)) )
	{
	    float z = arr.get( colrg.getIndex(col) );
	    pos.z = z;
	    geom->setKnot( rc, pos );
	}
    }

    return true;
}


Array1D<float>* Horizon2D::createArray1D( SectionID sid,
					  const PosInfo::Line2DKey& geomid,
					  const ZAxisTransform* trans ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    Array1DImpl<float>* arr = 0;
    const int lineidx = geom->getRowIndex( geomid );
    arr = new Array1DImpl<float>( geom->colRange(lineidx).nrSteps() + 1 );

    if ( !arr && !arr->isOK() )
	return 0;

    const StepInterval<int> colrg = geom->colRange( lineidx );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	Coord3 pos = geom->getKnot( RowCol(lineidx,col) );
	if ( trans )
	    pos.z = trans->transform( pos );

	arr->set( colrg.getIndex(col), (float) pos.z );
    }

    return arr;
}


Array1D<float>* Horizon2D::createArray1D( SectionID sid, Pos::GeomID geomid,
					  const ZAxisTransform* trans ) const
{
    const Geometry::Horizon2DLine* geom = geometry_.sectionGeometry( sid );
    if ( !geom || geom->isEmpty() )
	return 0;

    Array1DImpl<float>* arr = 0;
    const int lineidx = geom->getRowIndex( geomid );
    arr = new Array1DImpl<float>( geom->colRange(lineidx).nrSteps() + 1 );

    if ( !arr && !arr->isOK() )
	return 0;

    const StepInterval<int> colrg = geom->colRange( lineidx );
    for ( int col=colrg.start; col<=colrg.stop; col+=colrg.step )
    {
	Coord3 pos = geom->getKnot( RowCol(lineidx,col) );
	if ( trans )
	    pos.z = trans->transform( pos );

	arr->set( colrg.getIndex(col), (float) pos.z );
    }

    return arr;
}


const IOObjContext& Horizon2D::getIOObjContext() const
{ return EMHorizon2DTranslatorGroup::ioContext(); }


Table::FormatDesc* Horizon2DAscIO::getDesc()
{
    Table::FormatDesc* fd = new Table::FormatDesc( "Horizon2D" );
    fd->headerinfos_ += new Table::TargetInfo( "Undefined Value",
	    		StringInpSpec(sKey::FloatUdf()), Table::Required );
    BufferStringSet hornms;
    createDescBody( fd, hornms );
    return fd;
}


bool Horizon2DAscIO::isFormatOK(  const Table::FormatDesc& fd,
				  BufferString& msg )
{
    const bool trccoldefined = fd.bodyinfos_[2]->selection_.isInFile( 0 );
    const bool xycolsdefined = fd.bodyinfos_[1]->selection_.isInFile( 0 )
			       &&  fd.bodyinfos_[1]->selection_.isInFile( 1 );
     if ( trccoldefined || xycolsdefined )
	 return true;

     msg = "At least one of 'Trace Nr' and 'X Y' columns need to be defined";
     return false;
}


void Horizon2DAscIO::createDescBody( Table::FormatDesc* fd,
				     const BufferStringSet& hornms )
{
    fd->bodyinfos_ += new Table::TargetInfo( "Line name", Table::Required );
    Table::TargetInfo* ti = new Table::TargetInfo( "Position", DoubleInpSpec(),
	    				    Table::Optional );
    ti->form(0).add( DoubleInpSpec() ); ti->form(0).setName( "X Y" );
    fd->bodyinfos_ += ti;
    fd->bodyinfos_ += new Table::TargetInfo( "Trace nr", IntInpSpec(),
	    				     Table::Optional );
    for ( int idx=0; idx<hornms.size(); idx++ )
    {
	BufferString fldname = hornms.get( idx );
	ti = new Table::TargetInfo( fldname.buf(), FloatInpSpec(),
			Table::Required, PropertyRef::surveyZType() );
	ti->selection_.unit_ = UnitOfMeasure::surveyDefZUnit();
	fd->bodyinfos_ += ti;
    }
}


void Horizon2DAscIO::updateDesc( Table::FormatDesc& fd,
				 const BufferStringSet& hornms )
{
    fd.bodyinfos_.erase();
    createDescBody( &fd, hornms );
}


#define mErrRet(s) { if ( s ) errmsg_ = s; return 0; }

int Horizon2DAscIO::getNextLine( BufferString& lnm, Coord& crd, int& trcnr,
     				 TypeSet<float>& data )
{
    data.erase();
    if ( !finishedreadingheader_ )
    {
	if ( !getHdrVals(strm_) )
	    return -1;

	udfval_ = getfValue( 0 );
	finishedreadingheader_ = true;
    }

    int ret = getNextBodyVals( strm_ );
    if ( ret <= 0 ) return ret;

    lnm = text( 0 );
    crd.x = getdValue( 1 );
    crd.y = getdValue( 2 );
    trcnr = getIntValue( 3 );
    const int nrhors = vals_.size() - 4;
    for ( int idx=0; idx<nrhors; idx++ )
	data += getfValue( idx+4, udfval_ );

    return ret;
}

} // namespace EM
