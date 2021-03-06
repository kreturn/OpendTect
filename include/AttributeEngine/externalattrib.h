#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		Nov 2004
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "factory.h"
#include "datapack.h"
#include "sets.h"
#include "uistring.h"

class BinIDValueSet;
class TrcKeyZSampling;
class SeisTrcBuf;
class TaskRunner;
class RegularSeisDataPack;


namespace Attrib
{

class SelSpec;

/*!
\brief Generic class for attribs that does not come from the attribute engine.
*/

mExpClass(AttributeEngine) ExtAttribCalc
{
public:
    virtual			~ExtAttribCalc()			{}
    virtual bool		setTargetSelSpec(const SelSpec&)	= 0;
				/*!<\returns if this object can
				     compute it or not. */

    virtual RefMan<RegularSeisDataPack>	createAttrib(const TrcKeyZSampling&,
					     DataPack::ID, TaskRunner*);

    virtual bool		createAttrib(ObjectSet<BinIDValueSet>&,
					     TaskRunner*);
    virtual bool		createAttrib(const BinIDValueSet&, SeisTrcBuf&,
					     TaskRunner*);

    virtual bool		isIndexes() const	{ return false; }

    uiString			errmsg_;
};


mDefineFactory1Param( AttributeEngine, ExtAttribCalc, const Attrib::SelSpec&,
		      ExtAttrFact );


} // namespace Attrib
