#ifndef uisegydef_h
#define uisegydef_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Sep 2008
 RCS:           $Id: uisegydef.h,v 1.19 2011-04-15 12:02:58 cvsbert Exp $
________________________________________________________________________

-*/

#include "seistype.h"
#include "uigroup.h"
#include "uisegyread.h"
#include "segyfiledef.h"
class IOObj;
class uiLabel;
class uiButton;
class uiGenInput;
class uiTabStack;
class uiFileInput;
namespace SEGY { class TrcHeaderDef; class FileSpec; class FilePars; }


/*!\brief base class for specification of SEG-Y file stuff */

mClass uiSEGYDefGroup : public uiGroup
{
public:
    			uiSEGYDefGroup( uiParent* p, const char* grpnm,
					bool forread )
			    : uiGroup(p,grpnm)
			    , forread_(forread)				{}

    virtual bool	fillPar(IOPar&,bool permissive=false) const	= 0;
    virtual void	usePar(const IOPar&)				= 0;
    virtual void	use(const IOObj*,bool force)			= 0;

    virtual void	getReport( IOPar& iop ) const
			{ fillPar( iop, true ); }

protected:

    bool		forread_;
};


/*!\brief UI for Specification of SEG-Y in- or output file(s) */

mClass uiSEGYFileSpec : public uiSEGYDefGroup
{
public:
    mClass Setup
    {
    public:
			Setup( bool needmulti )
			    : forread_(true)
			    , canbe3d_(true)
			    , needmultifile_(needmulti)
			    , pars_(0)		{}

	mDefSetupMemb(bool,forread)
	mDefSetupMemb(bool,canbe3d)
	mDefSetupMemb(bool,needmultifile) //!< only used if for read
	mDefSetupMemb(const IOPar*,pars)
    };

    			uiSEGYFileSpec(uiParent*,const Setup&);

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);
    void		setFileName(const char*);

    SEGY::FileSpec	getSpec() const;
    void		setSpec(const SEGY::FileSpec&);

    void		setInp2D(bool);

    static const char*	sKeyLineNmToken()	{ return "#L"; }
    static const char*	fileFilter()
    			{ return  "SEG-Y files (*.sgy *.SGY *.segy)"; }


protected:

    uiFileInput*	fnmfld_;
    uiGenInput*		multifld_;
    uiButton*		manipbut_;
    bool		is2d_;
    bool		needmulti_;

    void		setMultiInput(const StepInterval<int>&,int);

    void		fileSel(CallBacker*);
    void		manipFile(CallBacker*);

};


/*!\brief UI for Specification of SEG-Y information needed to examine */

mClass uiSEGYFilePars : public uiSEGYDefGroup
{
public:
    			uiSEGYFilePars(uiParent*,bool forread,IOPar* iop=0);

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		use(const IOObj*,bool force);

    SEGY::FilePars	getPars() const;
    void		setPars(const SEGY::FilePars&);

    Notifier<uiSEGYFilePars> readParsReq;

protected:

    uiGenInput*		nrsamplesfld_;
    uiGenInput*		fmtfld_;
    uiGenInput*		byteswapfld_;

    void		readParsPush(CallBacker*);

};


/*!\brief UI for Specification of SEG-Y fields needed for proper import

  The idea is that you know beforehand whether the file is Rev.1 or not.
  If it's Rev. 1, the positioning part will not be present.
 
 */
class uiSEGYFOByteSpec;

mClass uiSEGYFileOpts : public uiSEGYDefGroup
{
public:

    mClass Setup
    {
    public:
				Setup( Seis::GeomType gt,
				    uiSEGYRead::Purpose pp=uiSEGYRead::Import,
				    uiSEGYRead::RevType rt=uiSEGYRead::Rev0 )
				    : geom_(gt)
				    , purpose_(pp)
				    , revtype_(rt)	{}

	mDefSetupMemb(Seis::GeomType,geom)
	mDefSetupMemb(uiSEGYRead::Purpose,purpose)
	mDefSetupMemb(uiSEGYRead::RevType,revtype)
    };

			uiSEGYFileOpts(uiParent*,const Setup&,const IOPar* i=0);
			~uiSEGYFileOpts();

    const Setup&	setup() const		{ return setup_; }
    bool		forScan() const
    			{ return setup_.purpose_ != uiSEGYRead::Import; }

    bool		fillPar(IOPar&,bool permissive=false) const;
    void		usePar(const IOPar&);
    void		getReport(IOPar&) const;
    void		use(const IOObj*,bool force);

    Notifier<uiSEGYFileOpts> readParsReq;
    Notifier<uiSEGYFileOpts> preScanReq;

protected:

    Setup		setup_;
    bool		is2d_;
    bool		isps_;
    SEGY::TrcHeaderDef&	thdef_;

    uiTabStack*		ts_;
    uiGroup*		posgrp_;
    uiGroup*		psgrp_;
    uiGroup*		orulegrp_;
    uiGroup*		coordgrp_;

    uiGenInput*		posfld_;
    uiGenInput*		psposfld_;
    uiSEGYFOByteSpec*	inldeffld_;
    uiSEGYFOByteSpec*	crldeffld_;
    uiSEGYFOByteSpec*	trnrdeffld_;
    uiSEGYFOByteSpec*	refnrdeffld_;
    uiSEGYFOByteSpec*	offsdeffld_;
    uiSEGYFOByteSpec*	azimdeffld_;
    uiSEGYFOByteSpec*	xcoorddeffld_;
    uiSEGYFOByteSpec*	ycoorddeffld_;
    uiGenInput*		regoffsfld_;
    uiGenInput*		readcoordsfld_;
    uiGenInput*		coordsstartfld_;
    uiGenInput*		coordsstepfld_;
    uiFileInput*	coordsfnmfld_;
    uiLabel*		ensurepsxylbl_;

    uiGenInput*		scalcofld_;
    uiGenInput*		timeshiftfld_;
    uiGenInput*		sampleratefld_;

    uiGroup*		mkORuleGrp(const IOPar&);
    uiGroup*		mkPosGrp(const IOPar&);
    uiGroup*		mkPSGrp(const IOPar&);
    uiGroup*		mkCoordGrp(const IOPar&);

    void		mkBinIDFlds(uiGroup*,const IOPar&);
    void		mkCoordFlds(uiGroup*,const IOPar&);

    void		initFlds(CallBacker*);
    void		psPosChg(CallBacker*);
    void		readParsPush(CallBacker*);
    void		preScanPush(CallBacker*);
    void		crdChk(CallBacker*);

    int			psPosType() const;
    void		toggledFldFillPar(uiGenInput*,const IOPar&,const char*,
					  bool isz=false);
    void		setToggled(IOPar&,const char*,uiGenInput*,
	    			   bool isz=false) const;

};


#endif
