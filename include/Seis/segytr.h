#ifndef segytrctr_h
#define segytrctr_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	A.H. Bril
 Date:		2-4-1996
 RCS:		$Id: segytr.h,v 1.36 2009-12-03 11:49:13 cvsbert Exp $
________________________________________________________________________

Translators for SEGY files traces.

-*/

#include "segyfiledef.h"
#include "seistrctr.h"
#include "tracedata.h"
#include "strmdata.h"
class LinScaler;
namespace SEGY { class TxtHeader; class BinHeader; class TrcHeader; }

#define mSEGYTraceHeaderBytes	240


mClass SEGYSeisTrcTranslator : public SeisTrcTranslator
{			      isTranslator(SEGY,SeisTrc)
public:

			SEGYSeisTrcTranslator(const char*,const char*);
			~SEGYSeisTrcTranslator();
    virtual const char*	defExtension() const	{ return "sgy"; }

    bool		readInfo(SeisTrcInfo&);
    bool		read(SeisTrc&);
    bool		skip(int);
    bool		goToTrace(int);

    bool		isRev1() const;
    int			numberFormat() const	{ return filepars_.fmt_; }
    int			estimatedNrTraces() const { return estnrtrcs_; }

    void		toSupported(DataCharacteristics&) const;
    void		usePar(const IOPar&);

    const SEGY::TxtHeader* txtHeader() const	{ return txthead_; }
    const SEGY::BinHeader& binHeader() const	{ return binhead_; }
    const SEGY::TrcHeader& trcHeader() const	{ return trchead_; }
    void		setTxtHeader(SEGY::TxtHeader*);	//!< write; becomes mine
    void		setForceRev0( bool yn )	{ forcerev0_ = yn; }

    int			dataBytes() const;
    bool		rev0Forced() const	{ return forcerev0_; }
    SEGY::FilePars&	filePars()		{ return filepars_; }
    SEGY::FileReadOpts&	fileReadOpts()		{ return fileopts_; }

    bool		implShouldRemove(const IOObj*) const { return false; }
    void		cleanUp();

protected:

    SEGY::FilePars	filepars_;
    SEGY::FileReadOpts	fileopts_;
    SEGY::TxtHeader*	txthead_;
    SEGY::BinHeader&	binhead_;
    SEGY::TrcHeader&	trchead_; // must be *after* fileopts_
    short		binhead_ns_;
    float		binhead_dpos_;
    LinScaler*		trcscale_;
    const LinScaler*	curtrcscale_;
    bool		forcerev0_;

    bool		useinpsd;
    TraceDataInterpreter* storinterp;
    unsigned char	headerbuf[mSEGYTraceHeaderBytes];
    bool		headerdone;
    bool		headerbufread;
    bool		bytesswapped;

    // Following variables are inited by commitSelections
    bool		userawdata;
    unsigned char*	blockbuf;
    ComponentData*	inpcd;
    TargetComponentData* outcd;
    StreamConn::Type	iotype;
    int			maxmbperfile;
    int			ic2xyopt;
    int			offsazimopt;
    SamplingData<int>	offsdef;

    inline StreamConn&	sConn()		{ return *(StreamConn*)conn; }

    bool		commitSelections_();
    virtual bool	initRead_();
    virtual bool	initWrite_(const SeisTrc&);
    virtual bool	writeTrc_(const SeisTrc&);

    bool		readTraceHeadBuffer();
    bool		readDataToBuf(unsigned char*);
    bool		readData(SeisTrc&);
    bool		writeData(const SeisTrc&);
    virtual bool	readTapeHeader();
    virtual void	updateCDFromBuf();
    int			nrSamplesRead() const;
    virtual void	interpretBuf(SeisTrcInfo&);
    virtual bool	writeTapeHeader();
    virtual void	fillHeaderBuf(const SeisTrc&);
    void		toPreSelected(DataCharacteristics&) const;
    virtual void	toPreferred(DataCharacteristics&) const;
    void		fillErrMsg(const char*,bool);
    bool		noErrMsg();

    DataCharacteristics	getDataChar(int) const;
    int			nrFormatFor(const DataCharacteristics&) const;
    void		addWarn(int,const char*);
    const char*		getTrcPosStr() const;
    bool		doInterpretBuf(SeisTrcInfo&);

    int			curtrcnr, prevtrcnr;
    BinID		curbinid, prevbinid;
    float		curoffs, prevoffs;
    Coord		curcoord;
    StreamData		coordsd;
    int			estnrtrcs_;
    bool		othdomain_;

};


#endif
