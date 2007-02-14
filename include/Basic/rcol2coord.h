#ifndef rcol2coord_h
#define rcol2coord_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		9-4-1996
 Contents:	RowCol <-> Coord transform
 RCS:		$Id: rcol2coord.h,v 1.5 2007-02-14 17:31:19 cvskris Exp $
________________________________________________________________________

-*/
 
#include <position.h>
#include <rowcol.h>

template <class T> class StepInterval;


/*!\brief Encapsulates linear tranform from (i,j) index to (x,y) coordinates. */

class RCol2Coord
{
public:

			RCol2Coord()		{}

    bool		isValid() const		{ return xtr.valid(ytr); }
    Coord		rowDir() const		{ return Coord(xtr.b,ytr.b); }
    Coord		colDir() const		{ return Coord(xtr.c,ytr.c); }
    Coord		transform(const RCol&) const;
    Coord		transform(const Coord& rc) const;
    			/*!< transforms a rowcol stored in a coord.  The 
			     row is stored in the x-component, and the
			     col is stored in the y-component. */
    RowCol		transformBack(const Coord&,
	    			  const StepInterval<int>* inlrg=0,
	    			  const StepInterval<int>* crlrg=0 ) const;
			/*!< Transforms Coord to RowCol. If the ranges are
			     given, they are only used for snapping: the
			     actual range is not used */

    Coord		transformBackNoSnap(const Coord&) const;
    			/*!< transforms back, but does not snap. The 
			     row is stored in the x-component, and the
			     col is stored in the y-component. */


    bool		set3Pts(const Coord& c0,const Coord& c1,const Coord& c2,
	    			const RCol& rc0,const RCol& rc1,int32 col2 ); 
			/*!<Sets up the transform using three points.
			    \note that the third point is assumed to be on
			    the same row as the first point.
			*/
    struct RCTransform
    {
			RCTransform()	{ a = b = c = 0; }

	inline double	det( const RCTransform& bct ) const
			{ return b * bct.c - bct.b * c; }
	inline bool	valid( const RCTransform& bct ) const
			{ double d = det( bct ); return !mIsZero(d,mDefEps); }

	double		a, b, c;
    };

    void		setTransforms(	const RCTransform& x,
					const RCTransform& y )
			{ xtr = x; ytr = y; }
    const RCTransform&	getTransform( bool x ) const
			{ return x ? xtr : ytr; }

protected:

    RCTransform		xtr;
    RCTransform		ytr;

};


#endif
