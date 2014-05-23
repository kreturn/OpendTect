#ifndef velocityfunctionascio_h
#define velocityfunctionascio_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Aug 2008
 RCS:		$Id$
________________________________________________________________________

-*/

#include "velocitymod.h"
#include "tableascio.h"
#include "task.h"

namespace Table { class FormatDesc; }
class BinIDValueSet;

namespace Vel
{

mExpClass(Velocity) FunctionAscIO : public Table::AscIO, public SequentialTask
{
public:
    				FunctionAscIO( const Table::FormatDesc& fd,
					       od_istream&,
				       	       od_int64 filesizeinkb=-1 );
   static Table::FormatDesc*	getDesc();
   static void			updateDesc(Table::FormatDesc&);

   float 			getUdfVal() const;
   bool				isXY() const;
   void				setOutput(BinIDValueSet& bvs)
       				{ output_ = &bvs; first_ = true; }

protected:

   int				nextStep();
   od_int64			nrDone() const { return nrdone_/1024; }
   uiStringCopy			uiNrDoneText() const { return "KBytes read"; }
   od_int64			totalNr() const { return nrkbytes_; }
   static void			createDescBody(Table::FormatDesc&);

   od_istream&			strm_;   
   BinIDValueSet*		output_;
   bool				first_;
   od_int64			nrdone_;
   od_int64			nrkbytes_;
};

} // namespace Vel


#endif

