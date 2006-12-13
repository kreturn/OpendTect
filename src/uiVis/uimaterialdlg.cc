/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          April 2002
 RCS:           $Id: uimaterialdlg.cc,v 1.10 2006-12-13 09:30:45 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimaterialdlg.h"

#include "uicolor.h"
#include "uisellinest.h"
#include "uislider.h"
#include "uitabstack.h"
#include "vismaterial.h"
#include "visobject.h"
#include "vissurvobj.h"

static const StepInterval<float> sSliderInterval( 0, 100, 10 );

uiLineStyleGrp::uiLineStyleGrp( uiParent* p, visSurvey::SurveyObject* so )
    : uiPropertyGrp(p,"Line style")
    , survobj_(so)
    , backup_(*so->lineStyle())
{
    field_ = new uiSelLineStyle( this, backup_, "Line style", true,
				 so->hasSpecificLineColor(), true );
    field_->changed.notify( mCB(this,uiLineStyleGrp,changedCB) );
}


void uiLineStyleGrp::changedCB( CallBacker* )
{
    survobj_->setLineStyle( field_->getStyle() );
}


bool uiLineStyleGrp::rejectOK( CallBacker* )
{
    survobj_->setLineStyle( backup_ );
    return true;
}


uiPropertiesDlg::uiPropertiesDlg( uiParent* p, visSurvey::SurveyObject* so )
    : uiDialog(p,"Display properties")
    , survobj_(so)
    , visobj_(dynamic_cast<visBase::VisualObject*>(so))
    , tabstack_(new uiTabStack(this,"TabStack"))
{
    if ( survobj_->allowMaterialEdit() && visobj_->getMaterial() )
    {
	uiPropertyGrp* grp = new uiMaterialGrp( tabstack_->tabGroup(),
				survobj_,
				true, true, false, false, false, true,
				survobj_->hasColor() );
	tabstack_->addTab( grp );
	tabs_ += grp;
    }

    if ( survobj_->lineStyle() )
    {
	uiPropertyGrp* grp =
	    new uiLineStyleGrp( tabstack_->tabGroup(), survobj_ );
	tabstack_->addTab( grp );
	tabs_ += grp;
    }

    setCancelText( "" );
    finaliseStart.notify( mCB(this,uiPropertiesDlg,doFinalise) );
}


void uiPropertiesDlg::doFinalise( CallBacker* cb )
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
	tabs_[idx]->doFinalise( cb );
}


bool uiPropertiesDlg::acceptOK( CallBacker* cb )
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( !tabs_[idx]->acceptOK(cb) )
	    return false;
    }

    return true;
}


bool uiPropertiesDlg::rejectOK( CallBacker* cb )
{
    for ( int idx=0; idx<tabs_.size(); idx++ )
    {
	if ( !tabs_[idx]->rejectOK(cb) )
	    return false;
    }

    return true;
}


uiMaterialGrp::uiMaterialGrp( uiParent* p, visSurvey::SurveyObject* so,
       bool ambience, bool diffusecolor, bool specularcolor,
       bool emmissivecolor, bool shininess, bool transparency, bool color )
    : uiPropertyGrp(p,"Material")
    , material_(dynamic_cast<visBase::VisualObject*>(so)->getMaterial())
    , survobj_(so)
    , ambslider_(0)
    , diffslider_(0)
    , specslider_(0)
    , emisslider_(0)
    , shineslider_(0)
    , transslider_(0)
    , colinp_(0)
    , prevobj_(0)
{
    if ( so->hasColor() )
    {
	colinp_ = new uiColorInput(this,Color(0,0,0),"Base color");
	colinp_->colorchanged.notify( mCB(this,uiMaterialGrp,colorChangeCB) );
	colinp_->setSensitive( color );
	prevobj_ = colinp_;
    }

    createSlider( ambience, ambslider_, "Ambient reflectivity" );
    createSlider( diffusecolor, diffslider_, "Diffuse reflectivity" );
    createSlider( specularcolor, specslider_, "Specular reflectivity" );
    createSlider( emmissivecolor, emisslider_, "Emissive intensity" );
    createSlider( shininess, shineslider_, "Shininess" );
    createSlider( transparency, transslider_, "Transparency" );
}


void uiMaterialGrp::createSlider( bool domk, uiSlider*& slider,
				  const char* lbltxt )
{
    if ( !domk ) return;

    uiSliderExtra* se = new uiSliderExtra( this, lbltxt );
    slider = se->sldr();
    slider->valueChanged.notify( mCB(this,uiMaterialGrp,sliderMove) );
    if ( prevobj_ ) se->attach( alignedBelow, prevobj_ );
    prevobj_ = se;
}


#define mFinalise( sldr, fn ) \
if ( sldr ) \
{ \
    sldr->setInterval( sSliderInterval ); \
    sldr->setValue( material_->fn()*100 ); \
}

void uiMaterialGrp::doFinalise( CallBacker* )
{
    mFinalise( ambslider_, getAmbience )
    mFinalise( diffslider_, getDiffIntensity )
    mFinalise( specslider_, getSpecIntensity )
    mFinalise( emisslider_, getEmmIntensity )
    mFinalise( shineslider_, getShininess )
    mFinalise( transslider_, getTransparency )

    if ( colinp_ ) colinp_->setColor( material_->getColor() );
}


void uiMaterialGrp::sliderMove( CallBacker* cb )
{
    mDynamicCastGet(uiSlider*,sldr,cb)
    if ( !sldr ) return;

    if ( sldr == ambslider_ )
	material_->setAmbience( ambslider_->getValue()/100 );
    else if ( sldr == diffslider_ )
	material_->setDiffIntensity( diffslider_->getValue()/100 );
    else if ( sldr == specslider_ )
	material_->setSpecIntensity( specslider_->getValue()/100 );
    else if ( sldr == specslider_ )
	material_->setEmmIntensity( emisslider_->getValue()/100 );
    else if ( sldr == shineslider_ )
	material_->setShininess( shineslider_->getValue()/100 );
    else if ( sldr == transslider_ )
	material_->setTransparency( transslider_->getValue()/100 );
}

void uiMaterialGrp::colorChangeCB(CallBacker*)
{ if ( colinp_ ) survobj_->setColor( colinp_->color() ); }
