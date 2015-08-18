#ifndef prestackattrib_h
#define prestackattrib_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        B.Bril & H.Huck
 Date:          14-01-2008
 RCS:           $Id$
________________________________________________________________________

-*/


#include "attributesmod.h"
#include "attribprovider.h"
#include "prestackprop.h"
#include "multiid.h"

class SeisPSReader;
class IOObj;

namespace PreStack { class ProcessManager; class Gather;
		     class AngleComputer; }


namespace Attrib
{

/*!
\brief "Prestack Attribute"

  Outputs a standard attribute from prestack data.
  Classname should really be PreStack, but the compiler complains and mixes up
  with PreStack namespace.

<pre>
  PreStack calctype= axistype= lsqtype= offsaxis= valaxis= useazim= comp=
  aperture= preprocessor=


  Input:
  0		Prestack Data

  Output:
  0		Attribute
</pre>
*/

mExpClass(Attributes) PSAttrib : public Provider
{
public:

    static void		initClass();

			PSAttrib(Desc&);

    static const char*  attribName()		{ return "PreStack"; }
    static const char*  offStartStr()		{ return "offstart"; }
    static const char*  offStopStr()		{ return "offstop"; }
    static const char*  preProcessStr()		{ return "preprocessor"; }
    static const char*  calctypeStr()		{ return "calctype"; }
    static const char*  stattypeStr()		{ return "stattype"; }
    static const char*  lsqtypeStr()		{ return "lsqtype"; }
    static const char*  offsaxisStr()		{ return "offsaxis"; }
    static const char*  valaxisStr()		{ return "valaxis"; }
    static const char*  componentStr()		{ return "comp"; }
    static const char*  apertureStr()		{ return "aperture"; }
    static const char*  velocityIDStr()		{ return "velocityid"; }
    static const char*  useangleStr()		{ return "useangle"; }
    static const char*  angleStartStr()		{ return "anglestart"; }
    static const char*  angleStopStr()		{ return "anglestop"; }
    static const char*	rayTracerParamStr()	{ return "raytracerparam"; }
    static const char*	gathertypeStr()		{ return "gathertype"; }
    static const char*	xaxisunitStr()		{ return "xaxisunit"; }

    enum GatherType	{ Off, Ang };
			DeclareEnumUtils(GatherType)
    enum XaxisUnit	{ Deg, Rad };
			DeclareEnumUtils(XaxisUnit)

    const PreStack::PropCalc::Setup&	setup() const	{ return setup_; }
    const MultiID&			psID() const	{ return psid_; }
    const MultiID&			preProcID() const { return preprocid_; }
    const MultiID&			velocityID() const
					{ return velocityid_; }

    void                updateCSIfNeeded(CubeSampling&) const;
    void		setAngleComp(PreStack::AngleComputer*);
    void		setAngleData(DataPack::ID);

protected:

			~PSAttrib();
    static Provider*    createInstance(Desc&);
    static void		updateDesc(Desc&);

    bool		allowParallelComputation() const	{ return true;}
    bool		getInputOutput(int input,TypeSet<int>& res) const;
    bool		getInputData(const BinID&, int idx);
    bool		getAngleInputData();
    bool		computeData(const DataHolder&,const BinID& relpos,
				    int t0,int nrsamples,int threadid) const;
    void		prepPriorToBoundsCalc();
    void		setSmootheningPar();
    float		getXscaler(bool isoffset, bool isindegrees) const;

    MultiID			psid_;
    IOObj*			psioobj_;
    SeisPSReader*		psrdr_;
    int				component_;
    PreStack::ProcessManager*	preprocessor_;
    PreStack::PropCalc*		propcalc_;
    PreStack::PropCalc::Setup	setup_;
    PreStack::AngleComputer*    anglecomp_;

    MultiID			preprocid_;
    int				dataidx_;
    const DataHolder*		inputdata_;
    MultiID			velocityid_;

    ObjectSet<PreStack::Gather>    gatherset_;

public:
    static const char*		angleDPIDStr()	{ return "angleid"; }
};

}; // namespace Attrib

#endif

