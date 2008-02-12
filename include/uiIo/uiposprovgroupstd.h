#ifndef uiposprovgroupstd_h
#define uiposprovgroupstd_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: uiposprovgroupstd.h,v 1.5 2008-02-12 09:07:35 cvsbert Exp $
________________________________________________________________________

-*/

#include "uiposprovgroup.h"
class MultiID;
class CtxtIOObj;
class CubeSampling;
class uiGenInput;
class uiIOObjSel;
class uiIOFileSelect;


/*! \brief UI for RangePosProvider */

class uiRangePosProvGroup : public uiPosProvGroup
{
public:

    struct Setup : public uiPosProvider::Setup
    {
			Setup( const char* txt, bool with_z )
			    : uiPosProvider::Setup(txt,with_z)
			    , withstep_(true)
			    , useworksi_(true)		{}
	mDefSetupMemb(bool,withstep)
	mDefSetupMemb(bool,useworksi)
    };

			uiRangePosProvGroup(uiParent*,
					    const uiPosProvider::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    void		getCubeSampling(CubeSampling&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvider::Setup& s )
    			{ return new uiRangePosProvGroup(p,s); }
    static void		initClass();

protected:

    bool		wstep_;
    bool		wsi_;

    uiGenInput*		inlfld_;
    uiGenInput*		crlfld_;
    uiGenInput*		zfld_;

};


/*! \brief UI for PolyPosProvider */

class uiPolyPosProvGroup : public uiPosProvGroup
{
public:
			uiPolyPosProvGroup(uiParent*,
					   const uiPosProvider::Setup&);
			~uiPolyPosProvGroup();

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    bool		getID(MultiID&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvider::Setup& s )
    			{ return new uiPolyPosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio_;

    uiIOObjSel*		polyfld_;
    uiGenInput*		zfld_;

};


/*! \brief UI for TablePosProvider */

class uiTablePosProvGroup : public uiPosProvGroup
{
public:
			uiTablePosProvGroup(uiParent*,
					   const uiPosProvider::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;

    bool		getID(MultiID&) const;
    bool		getFileName(BufferString&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvider::Setup& s )
    			{ return new uiTablePosProvGroup(p,s); }
    static void		initClass();

protected:

    CtxtIOObj&		ctio_;

    uiGenInput*		selfld_;
    uiIOObjSel*		psfld_;
    uiIOFileSelect*	tffld_;

    void		selChg(CallBacker*);

};


#endif
