#ifndef uiseistransf_h
#define uiseistransf_h
/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          June 2002
 RCS:           $Id: uiseistransf.h,v 1.19 2005-06-02 14:11:52 cvsbert Exp $
________________________________________________________________________

-*/

#include "uigroup.h"
#include "seisioobjinfo.h"
class IOObj;
class Executor;
class uiGenInput;
class SeisSelData;
class uiSeisSubSel;
class SeisResampler;
class uiSeisFmtScale;


class uiSeisTransfer : public uiGroup
{
public:

			uiSeisTransfer(uiParent*,bool with_format,
				       bool for_new_entry,bool withstep=true,
				       bool multi2dlines=false);

    void		updateFrom(const IOObj&);

    Executor*		getTrcProc(const IOObj& from,const IOObj& to,
	    			   const char* executor_txt,
				   const char* work_txt,
				   const char* attr2dnm,
				   const char* linenm2d_overrule=0) const;

    uiSeisSubSel*	selfld;
    uiSeisFmtScale*	scfmtfld;
    uiGenInput*		remnullfld;

    void		setInput(const IOObj&);
    bool		is2D() const		{ return is2d; }
    void		set2D(bool);
    bool		isSteer() const		{ return issteer; }
    void		setSteering(bool);
    void		getSelData(SeisSelData&) const;
    SeisResampler*	getResampler() const; //!< may return null

    int			maxBytesPerSample() const;
    SeisIOObjInfo::SpaceInfo spaceInfo() const;

    bool		removeNull() const;

protected:

    bool		is2d;
    bool		issteer;

    void		updFldsForType(CallBacker*);

};


#endif
