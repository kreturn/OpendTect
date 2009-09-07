/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Yuancheng Liu
 Date:		3-8-2008
________________________________________________________________________

-*/
static const char* rcsID = "$Id: vissplittextureseis2d.cc,v 1.10 2009-09-07 11:14:13 cvsnanne Exp $";

#include "vissplittextureseis2d.h"

#include "bendpointfinder.h"
#include "posinfo.h"
#include "simpnumer.h"
#include "SoTextureComposer.h"
#include "survinfo.h"
#include "viscoord.h"
#include "vistexturecoords.h"

#include <Inventor/nodes/SoIndexedTriangleStripSet.h>
#include <Inventor/nodes/SoSeparator.h>
#include <Inventor/nodes/SoShapeHints.h>
#include <Inventor/nodes/SoNormalBinding.h>
#include <Inventor/nodes/SoTextureCoordinate2.h>

#define mMaxHorSz 256
#define mMaxVerSz 256

mCreateFactoryEntry( visBase::SplitTextureSeis2D );

namespace visBase
{
   
SplitTextureSeis2D::SplitTextureSeis2D()
    : VisualObjectImpl( false )
    , zrg_( 0, 0 )
    , trcrg_( 0, 0 )
    , nrzpixels_( 0 )
{
    coords_ = visBase::Coordinates::create();
    coords_->ref();
    addChild( coords_->getInventorNode() );

    SoShapeHints* shapehint = new SoShapeHints;
    addChild( shapehint );
    shapehint->vertexOrdering = SoShapeHints::COUNTERCLOCKWISE;
    shapehint->shapeType = SoShapeHints::UNKNOWN_SHAPE_TYPE;

    SoNormalBinding* nb = new SoNormalBinding;
    addChild( nb );
    nb->value = SoNormalBinding::PER_FACE_INDEXED;

    setMaterial( 0 );
}


SplitTextureSeis2D::~SplitTextureSeis2D()
{
    coords_->unRef();
    for ( int idx=0; idx<separators_.size(); idx++ )
	separators_[idx]->unref();

    deepErase( horblocktrcindices_ );
}


void SplitTextureSeis2D::setPath( const TypeSet<PosInfo::Line2DPos>& path )
{
    path_.erase();
    trcnrs_.erase();
    
    const int nrtraces = path.size();
    if ( !nrtraces )
	return;
    
    trcnrs_ += path[0].nr_;
    path_ += path[0].coord_;
    
    for ( int idx=1; idx<nrtraces; idx++ )
    {
	int insert = idx;
	while ( path[idx].nr_<path[insert-1].nr_ && insert>=1 )
	    insert--;
	
	if ( insert==idx )
	{
	    path_ += path[idx].coord_;
	    trcnrs_ += path[idx].nr_;
	}
	else
	{
	    path_.insert( insert, path[idx].coord_ );
	    trcnrs_.insert( insert, path[idx].nr_ );
	}
    }    

    updateHorSplit();
}


void SplitTextureSeis2D::setDisplayedGeometry( const Interval<int>& trcrg,
					       const Interval<float>& zrg )
{
    if ( trcrg_ == trcrg && zrg_ == zrg )
	return;

    zrg_ = zrg;
    if ( trcrg_!=trcrg )
    {
	trcrg_ = trcrg;
	updateHorSplit();
    }
    else
	updateDisplay();
}


void SplitTextureSeis2D::setTextureZPixels( int zsize )
{
    if ( nrzpixels_ == zsize )
	return;

    nrzpixels_ = zsize;
    updateDisplay();
}


mVisTrans* SplitTextureSeis2D::getDisplayTransformation()
{
    return coords_->getDisplayTransformation(); 
}


void SplitTextureSeis2D::setDisplayTransformation( mVisTrans* nt )
{
    coords_->setDisplayTransformation( nt );
}


void SplitTextureSeis2D::updateHorSplit()
{
    const int geomsz = path_.size();
    if ( !trcrg_.width() || !geomsz ) 
	return;

    deepErase( horblocktrcindices_ );
    int diff = 0;
    while ( trcrg_.start>trcnrs_[diff] && diff<geomsz-1 )
	diff++;
    
    if ( diff>=geomsz-1 )
	return;

    const int nrtrcs = trcrg_.width()+1;
    const int nrhorblocks = nrBlocks( nrtrcs, mMaxHorSz, 1 );
    for ( int idx=0; idx<nrhorblocks; idx++ )
    {
	Interval<int> blockidxrg( idx*(mMaxHorSz-1), (idx+1)*(mMaxHorSz-1) );
	if ( blockidxrg.stop>=nrtrcs || nrhorblocks==1 ) 
	    blockidxrg.stop = nrtrcs-1;

	const int pathsize = blockidxrg.width()+1;
	const int offset = blockidxrg.start+diff;
	BendPointFinder2D finder( path_.arr()+offset, pathsize, 0.5 );
	finder.execute();

	const TypeSet<int>& totalbps = finder.bendPoints();

	TypeSet<int>* trcindices = new TypeSet<int>;
	trcindices->setCapacity( totalbps.size() );
	for ( int idy=0; idy<totalbps.size(); idy++ )
	    (*trcindices) += totalbps[idy] + offset;

	horblocktrcindices_ += trcindices;
    }
  
    updateDisplay();
}


void SplitTextureSeis2D::updateSeparator( SoSeparator* sep,
	SoIndexedTriangleStripSet*& tristrip, SoTextureCoordinate2*& tc,
	SoTextureComposer*& tcomp, bool hastexture ) const
{
    if ( sep->getNumChildren() )
    {
	tristrip = (SoIndexedTriangleStripSet*)
	    sep->getChild( sep->getNumChildren()-1 );
    }
    else
    {
	tristrip = new SoIndexedTriangleStripSet;
	sep->addChild( tristrip );
    }

    if ( hastexture )
    {
	if ( sep->getNumChildren()>1 )
	    tc = (SoTextureCoordinate2*) sep->getChild(sep->getNumChildren()-2);
	else
	{
	    tc = new SoTextureCoordinate2;
	    sep->insertChild( tc, 0 );
	    
	    if ( sep->findChild(tc)!=sep->findChild(tristrip)-1 )
	    {
		sep->removeChild( tc );
		sep->insertChild( tc, sep->findChild( tristrip ) );
	    }
	}

	if ( sep->getNumChildren()>2 )
	    tcomp = (SoTextureComposer*) sep->getChild(sep->getNumChildren()-3);
	else
	{
	    tcomp = new SoTextureComposer;
	    sep->insertChild( tcomp, 0 );
	    
	    if ( sep->findChild(tcomp)!=sep->findChild(tc)-1 )
	    {
		sep->removeChild( tcomp );
		sep->insertChild( tcomp, sep->findChild( tc ) );
	    }
	}
    }
    else 
    {
	while ( sep->getNumChildren()>1 )
    	    sep->removeChild( 0 );
    }
}


void SplitTextureSeis2D::updateDisplay( )
{
    if ( !zrg_.width() || !trcrg_.width() )
	return;

    int firstpathidx = 0;
    while ( trcrg_.start>trcnrs_[firstpathidx] )
    {
	firstpathidx++;
	if ( firstpathidx >= trcnrs_.size() )
	    return;
    }

    const int verblocks = nrBlocks( nrzpixels_, mMaxVerSz, 1 );
    ObjectSet<SoSeparator> unusedseparators = separators_;

    int coordidx = 0;
    const float inithorpos = (*horblocktrcindices_[0])[0];
    for ( int horidx=0; horidx<horblocktrcindices_.size(); horidx++ )
    {
	TypeSet<int>* horblockrg = horblocktrcindices_[ horidx ];
	for ( int idz=0; idz<verblocks; idz++ )
	{
	    SoSeparator* sep = 0;
	    SoTextureComposer* tcomp = 0;
	    SoTextureCoordinate2* tc = 0;
	    SoIndexedTriangleStripSet* tristrip = 0;

	    if ( unusedseparators.size() )
		sep = unusedseparators.remove( 0 );
	    else
	    {
		sep = new SoSeparator;
		sep->ref();
		addChild( sep );
		separators_ += sep;
	    }

	    updateSeparator( sep, tristrip, tc, tcomp, nrzpixels_ );
	    
	    const int startzpixel = idz * (mMaxVerSz-1);
	    int stopzpixel = startzpixel + mMaxVerSz-1;
	    if ( stopzpixel>=nrzpixels_ || verblocks==1 ) 
		stopzpixel = nrzpixels_-1;
	    
	    const int bpsz = (*horblockrg).size();
	    const int horsz = (*horblockrg)[bpsz-1]-(*horblockrg)[0]+1;
	    const int versz = stopzpixel-startzpixel+1;
	    const int texturehorsz = nextPower( horsz, 2 );
	    const int textureversz = nextPower( versz, 2 );
	    
	    if ( tcomp )
	    {
		tcomp->origin.setValue( 0, 
			(*horblockrg)[0]-firstpathidx, startzpixel );
		tcomp->size.setValue( 1, texturehorsz, textureversz );
	    }
	 
	    if ( tc )
	    {
		const float tcstart = 0.5/textureversz;
		const float tcstop = (versz-0.5)/textureversz;
		int tcidx = 0;
		for ( int idx=0; idx<bpsz; idx++ )
		{
		    const float dist = (*horblockrg)[idx]-(*horblockrg)[0];
		    const float tcrd = (0.5+dist)/texturehorsz;
	
		    tc->point.set1Value( tcidx, SbVec2f(tcstart,tcrd) );
		    tcidx++;
		    
		    tc->point.set1Value( tcidx, SbVec2f(tcstop,tcrd) );
		    tcidx++;
		}
		
		tc->point.deleteValues( tcidx, -1 );
	    }
	    
	    Interval<float> blockzrg = zrg_;
	    if ( nrzpixels_ )
	    {
		const float zstep = zrg_.width()/(nrzpixels_-1);
		blockzrg.start += zstep*startzpixel; 
		blockzrg.stop = (stopzpixel==nrzpixels_-1)
		    ? zrg_.stop : blockzrg.start+(versz-1)*zstep;
	    }
	   
	    int curknotidx=0;
	    for ( int idx=0; idx<bpsz; idx++ )
	    {
		const int ti = (*horblockrg)[idx];
		tristrip->textureCoordIndex.set1Value( curknotidx,curknotidx );
		tristrip->coordIndex.set1Value( curknotidx, coordidx ); 
		coords_->setPos( coordidx, Coord3(path_[ti], blockzrg.start) );
		curknotidx++; 
		coordidx++;
		
		tristrip->textureCoordIndex.set1Value( curknotidx,curknotidx );
		tristrip->coordIndex.set1Value( curknotidx, coordidx ); 
		coords_->setPos( coordidx, Coord3(path_[ti], blockzrg.stop) );
		curknotidx++;
		coordidx++;
	    }
	    
	    tristrip->coordIndex.deleteValues( curknotidx, -1 );
	    tristrip->textureCoordIndex.deleteValues( curknotidx, -1 );
	}
    }

    coords_->removeAfter( coordidx-1 );
    
    for ( int idx=unusedseparators.size()-1; idx>=0; idx-- ) 
    { 
    	separators_ -= unusedseparators[idx]; 
    	removeChild( unusedseparators[idx] );
    	unusedseparators[idx]->unref();
    	unusedseparators.remove( idx ); 
    }
}

}; // Namespace
