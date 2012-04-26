#ifndef fftfilter_h
#define fftfilter_h

/*
________________________________________________________________________

(C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
Author:        Bruno
Date:          6-10-2009
RCS:           $Id: fftfilter.h,v 1.8 2012-04-26 14:37:33 cvsbruno Exp $
________________________________________________________________________

*/


#include <complex>
#include "enums.h"

namespace Fourier { class CC; }
class ArrayNDWindow;

template <class T> class Array1DImpl;
typedef std::complex<float> float_complex;

/*! brief classical FFT filter, use set to set up data step, min and max frequency and type of the filter (minfreq not required for highpass, maxfreq not required for lowpass) !*/ 

mClass FFTFilter
{

public:
			FFTFilter();
			~FFTFilter();	   

			enum Type		{ LowPass, HighPass, BandPass };
			DeclareEnumUtils(Type)

    void  		set(float d_f,float minf,float maxf,Type,bool zeropadd); 
    void		apply(const float*,float*,int sz) const;
    void		apply(const float_complex*,float_complex*,int sz) const;

    void		apply(const Array1DImpl<float>&,
	    			 Array1DImpl<float>&) const;
    void		apply(const Array1DImpl<float_complex>&,
	    			 Array1DImpl<float_complex>&) const;

			//will taper the array before apply
    void		setTaperWindow( float* samp, int sz )
			{ delete timewindow_; timewindow_=new Window(samp,sz); }

			//optional cut-off the frequency with a window
    void		setFreqBorderWindow(float* win,int sz,bool forlowpass);

protected:

    mStruct Window
    {
			Window(float* win,int sz)
			    : win_(win)
			    , size_(sz)		{}

			int size_;
			float* win_;
    };

    float		df_;
    Type		type_;
    float		maxfreq_;
    float		minfreq_;
    bool		iszeropadd_;

    Fourier::CC*	fft_; 
    Window*		timewindow_;
    Window*		hfreqwindow_;
    Window*		lfreqwindow_;

    void 		FFTFreqFilter(float,float,bool,
			    const float_complex*,float_complex*,int sz) const;
    void   		FFTBandPassFilter(float,float,float,
			    const float_complex*,float_complex*,int sz) const;
};

#endif
