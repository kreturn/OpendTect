#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/

#include "uiiocommon.h"
#include "uiposprovgroup.h"
class TrcKeyZSampling;
class uiGenInput;
class uiPickSetIOObjSel;
class uiSelSteps;
class uiSelHRange;
class uiSelZRange;
class uiSelNrRange;
class uiFileInput;


/*! \brief UI for RangePosProvider */

mExpClass(uiIo) uiRangePosProvGroup : public uiPosProvGroup
{
public:

			uiRangePosProvGroup(uiParent*,
					    const uiPosProvGroup::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(BufferString&) const;

    void		setExtractionDefaults();

    void		getTrcKeyZSampling(TrcKeyZSampling&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiRangePosProvGroup(p,s); }
    static void		initClass();

protected:

    uiSelHRange*	hrgfld_;
    uiSelZRange*	zrgfld_;
    uiSelNrRange*	nrrgfld_;

    uiPosProvGroup::Setup setup_;

};


/*! \brief UI for PolyPosProvider */

mExpClass(uiIo) uiPolyPosProvGroup : public uiPosProvGroup
{
public:
			uiPolyPosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(BufferString&) const;

    void		setExtractionDefaults();

    bool		getID(DBKey&) const;
    void		getZRange(StepInterval<float>&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiPolyPosProvGroup(p,s); }
    static void		initClass();

protected:

    uiPickSetIOObjSel*	polyfld_;
    uiSelSteps*		stepfld_;
    uiSelZRange*	zrgfld_;

};


/*! \brief UI for TablePosProvider */

mExpClass(uiIo) uiTablePosProvGroup : public uiPosProvGroup
{ mODTextTranslationClass(uiTablePosProvGroup)
public:
			uiTablePosProvGroup(uiParent*,
					   const uiPosProvGroup::Setup&);

    virtual void	usePar(const IOPar&);
    virtual bool	fillPar(IOPar&) const;
    void		getSummary(BufferString&) const;

    bool		getID(DBKey&) const;
    bool		getFileName(BufferString&) const;

    static uiPosProvGroup* create( uiParent* p, const uiPosProvGroup::Setup& s)
			{ return new uiTablePosProvGroup(p,s); }
    static void		initClass();

protected:

    uiGenInput*		selfld_;
    uiPickSetIOObjSel*	psfld_;
    uiFileInput*	tffld_;

    void		selChg(CallBacker*);

};
