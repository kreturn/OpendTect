#ifndef horizonsorter_h
#define horizonsorter_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	N. Hemstra
 Date:		April 2006
 RCS:		$Id$
________________________________________________________________________

-*/

#include "earthmodelmod.h"
#include "executor.h"

#include "cubesampling.h"
#include "multiid.h"
#include "binid.h"
#include "posinfo2dsurv.h"


namespace EM { class Horizon; }
template <class T> class Array3D;
class HorSamplingIterator;

/*!
\brief Executor to sort horizons.
*/

mExpClass(EarthModel) HorizonSorter : public Executor
{
public:

				HorizonSorter(const TypeSet<MultiID>&,
					      bool is2d=false);
				~HorizonSorter();

    void			getSortedList(TypeSet<MultiID>&);
    const HorSampling&		getBoundingBox() const	{ return hrg_; }
    int				getNrCrossings(const MultiID&,
	    				       const MultiID&) const;

    uiStringCopy		uiMessage() const;
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    uiStringCopy		uiNrDoneText() const;

protected:

    int				nextStep();
    void			calcBoundingBox();
    void			init();
    void			sort();

    int				totalnr_;
    int				nrdone_;

    bool			is2d_;
    TypeSet<Pos::GeomID>	geomids_;
    TypeSet<StepInterval<int> >	trcrgs_;

    HorSamplingIterator*	iterator_;
    BinID			binid_;
    HorSampling			hrg_;
    ObjectSet<EM::Horizon>	horizons_;
    Array3D<int>*		result_;
    TypeSet<MultiID>		unsortedids_;
    TypeSet<MultiID>		sortedids_;

    uiString			message_;
};


#endif

