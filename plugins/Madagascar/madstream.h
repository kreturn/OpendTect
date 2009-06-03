#ifndef madstream_h
#define madstream_h
/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : R. K. Singh
 * DATE     : March 2008
 * ID       : $Id: madstream.h,v 1.8 2009-06-03 14:25:58 cvshelene Exp $
-*/


#include "position.h"
#include "strmdata.h"

class BufferString;
class CubeSampling;
class IOPar;
class SeisTrcReader;
class SeisTrcWriter;
class SeisTrcBuf;
class SeisPSReader;
class SeisPSWriter;

namespace PosInfo { class CubeDataIterator; class Line2DData; }
namespace Seis { class SelData; }

namespace ODMad
{

class MadStream
{
public:
    				MadStream(IOPar&);
				~MadStream();

    const IOPar*		getHeaderPars()		{ return headerpars_; }
    bool			getNextTrace(float*);
    int				getNrSamples() const;
    bool			putHeader(std::ostream&);
    bool			writeTraces(bool writetofile=true);
    SeisTrcBuf*			getTrcBuf() const	{ return stortrcbuf_; }

    bool			isOK() const;
    const char*			errMsg() const;
    bool			is2D() const		{ return is2d_; }
    bool			isPS() const		{ return isps_; }
    void			setStorBufMine( bool yn )
				{ stortrcbufismine_ = yn; }

protected:

    bool			iswrite_;
    bool			is2d_;
    bool			isps_;
    IOPar&			pars_;
    IOPar*			headerpars_;
    BufferString&		errmsg_;

    std::istream*		istrm_;
    std::ostream*		ostrm_;

    SeisTrcReader*		seisrdr_;
    SeisTrcWriter*		seiswrr_;
    SeisPSReader*		psrdr_;
    SeisPSWriter*		pswrr_;

    BinID			curbid_;
    SeisTrcBuf*			trcbuf_;
    SeisTrcBuf*			stortrcbuf_;
    bool			stortrcbufismine_;
    PosInfo::CubeDataIterator*	iter_;
    PosInfo::Line2DData*	l2ddata_;
    int				nroffsets_;
    int				curtrcidx_;		// For PS

    void			initRead(IOPar*);
    void			initWrite(IOPar*);
    void			fillHeaderParsFromStream();
    void			fillHeaderParsFromSeis();
    void			fillHeaderParsFromPS(const Seis::SelData*);
    bool			write2DTraces(bool);
    bool			getNextPos(BinID&);
    BufferString		getPosFileName(bool forread=false) const;
};


} // namespace ODMad

#endif
