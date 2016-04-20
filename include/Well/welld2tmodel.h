#ifndef welld2tmodel_h
#define welld2tmodel_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert Bril
 Date:		Aug 2003
________________________________________________________________________


-*/

#include "welldahobj.h"
#include "uistring.h"

class TimeDepthModel;
class uiString;

namespace Well
{

/*!\brief Depth to time model. */

mExpClass(Well) D2TModel : public DahObj
{ mODTextTranslationClass(D2TModel);
public:

			D2TModel( const char* nm= 0 )
			: DahObj(nm)	{}
			D2TModel( const D2TModel& d2t )
			: DahObj("")	{ *this = d2t; }
    D2TModel&		operator =(const D2TModel&);
    bool		operator ==(const D2TModel&) const;
    bool		operator !=(const D2TModel&) const;

    float		getTime(float d_ah, const Track&) const;
    float		getDepth(float time, const Track&) const;
    float		getDah(float time, const Track&) const;
    double		getVelocityForDah(float d_ah,const Track&) const;
    double		getVelocityForDepth(float dpt,const Track&) const;
    double		getVelocityForTwt(float twt,const Track&) const;
    bool		getTimeDepthModel(const Well::Data&,
					  TimeDepthModel&) const;

    inline float	t( int idx ) const	{ return t_[idx]; }
    float		value( int idx ) const	{ return t(idx); }
    float*		valArr()		{ return t_.arr(); }
    const float*	valArr() const		{ return t_.arr(); }

    BufferString	desc;
    BufferString	datasource;

    static const char*	sKeyTimeWell(); //!< name of model for well that is only
				      //!< known in time
    static const char*	sKeyDataSrc();

    void		add( float d_ah, float tm )
						{ dah_ += d_ah; t_ += tm; }
    bool		insertAtDah(float d_ah,float t);

    void		makeFromTrack(const Track&, float cstvel,
				      float replvel);
			//!< cstvel: velocity of the TD model
			//!< replvel: Replacement velocity, above SRD
    bool		ensureValid(const Well::Data&,uiString& errmsg,
				    TypeSet<double>* zvals=0,
				    TypeSet<double>* tvals=0);
			//!< Returns corrected model if necessary
			//!< May eventually also correct info().replvel

protected:

    TypeSet<float>	t_;

    void		removeAux( int idx )	{ t_.removeSingle(idx); }
    void		eraseAux()		{ t_.erase(); }

    bool		getVelocityBoundsForDah(float d_ah,const Track&,
					  Interval<double>& depths,
					  Interval<float>& times) const;
    bool		getVelocityBoundsForTwt(float twt,const Track&,
					  Interval<double>& depths,
					  Interval<float>& times) const;
			/*!<Gives index of next dtpoint at or after dah.*/
    int			getVelocityIdx(float pos,const Track&,
				       bool posisdah=true) const;

protected:

    inline float	getDepth( float time ) const { return mUdf(float); }
			//!< Legacy, misleading name. Use getDah().
    bool		getOldVelocityBoundsForDah(float d_ah,const Track&,
					     Interval<double>& depths,
					     Interval<float>& times) const;
			//!<Read legacy incorrect time-depth model.
    bool		getOldVelocityBoundsForTwt(float twt,const Track&,
					     Interval<double>& depths,
					     Interval<float>& times) const;
			//!<Read legacy incorrect time-depth model.
    static bool getTVDD2TModel(Well::D2TModel&,const Well::Data&,
			  TypeSet<double>& zvals, TypeSet<double>& tvals,
			  uiString& errmsg, uiString& warnmsg);
    static void convertDepthsToMD(const Well::Track&,
				  const TypeSet<double>& zvals,
				  TypeSet<float>& dahs);
    static void shiftTimesIfNecessary(TypeSet<double>& tvals, double wllheadz,
				 double vrepl, double origintwtinfile,
				  uiString& msg);
    static void checkReplacementVelocity(Well::Info&,double vreplinfile,
					 uiString& msg);
};

}; // namespace Well


#endif
