/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          October 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprintscenedlg.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uifileinput.h"
#include "uilabel.h"
#include "uimain.h"
#include "uimsg.h"
#include "uiobj.h"
#include "uipixmap.h"
#include "uirgbarray.h"
#include "uispinbox.h"
#include "ui3dviewer.h"

#include "filepath.h"
#include "keystrs.h"
#include "mousecursor.h"
#include "settings.h"
#include "visscene.h"
#include "viscamera.h"

#include <osgViewer/View>
#include <osgGeo/TiledOffScreenRenderer>

static bool prevbuttonstate = true;
#define mAttachToAbove( fld ) \
	if ( fldabove ) fld->attach( alignedBelow, fldabove ); \
	fldabove = fld

uiPrintSceneDlg::uiPrintSceneDlg( uiParent* p,
				  const ObjectSet<ui3DViewer>& vwrs )
    : uiSaveImageDlg(p,false)
    , viewers_(vwrs)
    , scenefld_(0)
{
    screendpi_ = uiMain::getDPI();

    uiObject* fldabove = 0;
    if ( viewers_.size() > 1 )
    {
	BufferStringSet scenenms;
	for ( int idx=0; idx<viewers_.size(); idx++ )
	    scenenms.add( viewers_[idx]->getScene()->name() );

	scenefld_ = new uiLabeledComboBox( this, scenenms,
                    tr("Make snapshot of") );
	scenefld_->box()->selectionChanged.notify(
					mCB(this,uiPrintSceneDlg,sceneSel) );
	mAttachToAbove( scenefld_->attachObj() );
    }


    if ( scenefld_ )
	createGeomInpFlds( scenefld_->attachObj() );
    else
	createGeomInpFlds( fldabove );

    fileinputfld_->attach( alignedBelow, dpifld_ );

    sceneSel(0);
    PtrMan<IOPar> ctiopar;
    getSettingsPar( ctiopar, BufferString("3D") );

    if ( ctiopar.ptr() )
    {
	if ( !usePar(*ctiopar) )
	    useparsfld_->setValue( false );
    }
    else
    {
	useparsfld_->setValue( false );
	setFldVals( 0 );
    }

    updateFilter();
    setSaveButtonChecked( prevbuttonstate );
    unitChg( 0 );
}


void uiPrintSceneDlg::getSupportedFormats( const char** imagefrmt,
					   const char** frmtdesc,
					   BufferString& filters )
{
    BufferStringSet supportedimageformats;
    uiPixmap::supportedImageFormats( supportedimageformats );

    int idx = 0;
    while ( imagefrmt[idx] )
    {
	for ( int idxfmt = 0; idxfmt<supportedimageformats.size(); idxfmt++ )
	{
	    if ( supportedimageformats.get(idxfmt) == imagefrmt[idx] )
	    {
		if ( !filters.isEmpty() ) filters += ";;";
		filters += frmtdesc[idx];
		break;
	    }
	}
	idx++;
    }

}


void uiPrintSceneDlg::setFldVals( CallBacker* )
{
    if ( useparsfld_->getBoolValue() )
    {
	lockfld_->setChecked( false );
	lockfld_->setSensitive( true );
	PtrMan<IOPar> ctiopar;
	getSettingsPar( ctiopar, BufferString("3D") );

	if ( ctiopar.ptr() )
	{
	    if ( !usePar(*ctiopar) )
		useparsfld_->setValue( false );
	}
    }
    else
    {
	dpifld_->box()->setValue( screendpi_ );
	sceneSel( 0 );
	lockfld_->setChecked( true );
	lockfld_->setSensitive( false );
    }
}


void uiPrintSceneDlg::sceneSel( CallBacker* )
{
    const int vwridx = scenefld_ ? scenefld_->box()->currentItem() : 0;
    const ui3DViewer* vwr = viewers_[vwridx];
    const Geom::Size2D<int> winsz = vwr->getViewportSizePixels();
    aspectratio_ = (float)winsz.width() / winsz.height();

    if ( useparsfld_->getBoolValue() )
	return;

    setSizeInPix( winsz.width(), winsz.height() );
}


#define m32BitSizeLimit 4294967294UL  //4GB osg image size limitation
bool uiPrintSceneDlg::acceptOK( CallBacker* )
{
    const od_int64 size = mNINT64(sizepix_.width()*sizepix_.height())*4 ;
    if( size>m32BitSizeLimit )
    {
	pErrMsg( "The output image size is too big, please reduce image size\
		  or resolution" );
	return false;
    }
    
    if ( !filenameOK() )
	return false;

    bool ret = false;
    MouseCursorChanger cursorchanger( MouseCursor::Wait );

    const int vwridx = scenefld_ ? scenefld_->box()->currentItem() : 0;
    ui3DViewer* vwr = const_cast<ui3DViewer*>(viewers_[vwridx]);

    const float scenesDPI =  vwr->getScenesPixelDensity();
    const float renderDPI =  dpifld_->box()->getValue();

    const bool changedpi = scenesDPI != renderDPI;

    if ( changedpi )
        vwr->setScenesPixelDensity( renderDPI );

    osgViewer::View* mainview =
	const_cast<osgViewer::View*>( vwr->getOsgViewerMainView() );

    osgViewer::View* hudview =
	const_cast<osgViewer::View*>( vwr->getOsgViewerHudView() );

    ui3DViewer::WheelMode curmode = vwr->getWheelDisplayMode();
    vwr->setWheelDisplayMode( ui3DViewer::Never );
    osg::ref_ptr<osg::Image> hudimage = offScreenRenderViewToImage( hudview );
    vwr->setWheelDisplayMode( curmode );

    osg::ref_ptr<osg::Image> mainviewimage =
					offScreenRenderViewToImage( mainview );

    if ( changedpi )
        vwr->setScenesPixelDensity( scenesDPI );

    const int validresult = validateImages( mainviewimage, hudimage );

    if ( validresult == InvalidImages )
	return false;
    else if ( validresult == OnlyMainViewImage )
    {
	flipImageVertical( mainviewimage );
	ret = saveImages( mainviewimage, 0 );
    }
    else
    {
	flipImageVertical( hudimage );
	flipImageVertical( mainviewimage );
	ret = saveImages( mainviewimage, hudimage );
    }

    prevbuttonstate = saveButtonChecked();
    if ( prevbuttonstate )
	writeToSettings();

   return ret;

}


int uiPrintSceneDlg::validateImages(const osg::Image* mainimage,
				    const osg::Image* hudimage)
{
    if ( !mainimage || !hasImageValidFormat(mainimage) )
    {
	uiMSG().error( tr(" Image creation is failed. ") );
	return InvalidImages;
    }

    bool validsize (false);
    if ( hudimage && mainimage )
    {
	validsize = mainimage->s() == hudimage->s() &&
		    mainimage->t() == hudimage->t();
	if ( !validsize )
	{
	    pErrMsg("The size of main view image is different from hud image.");
	    return OnlyMainViewImage;
	}
    }

    if ( !hudimage || !validsize || !hasImageValidFormat( hudimage) )
    {
	uiMSG().warning(
	   tr("Cannot include HUD items (color bar and axis) in screen shot."));
	return OnlyMainViewImage;
    }

    return MainAndHudImages;
}


bool uiPrintSceneDlg::saveImages( const osg::Image* mainimg,
				  const osg::Image* hudimg )
{
    if ( !filenameOK() || !widthfld_ || !mainimg )
	return false;

    FilePath filepath( fileinputfld_->fileName() );
    setDirName( filepath.pathOnly() );

    const char* fmt = uiSaveImageDlg::getExtension();

    uiRGBArray rgbhudimage( true );
    if ( hudimg && hudimg->getPixelFormat() == GL_RGBA )
    {
	rgbhudimage.setSize( hudimg->s(), hudimg->t() );
	rgbhudimage.put( hudimg->data(),false,true );
    }

    uiRGBArray rgbmainimage( mainimg->getPixelFormat()==GL_RGBA );
    rgbmainimage.setSize( mainimg->s(), mainimg->t() );

    if ( !rgbmainimage.put( mainimg->data(), false, true ) )
	return false;

    if ( rgbhudimage.bufferSize()>0 )
	rgbmainimage.blendWith( rgbhudimage, false, true );

    rgbmainimage.save( filepath.fullPath().buf(),fmt );

    return true;

}


osg::Image* uiPrintSceneDlg::offScreenRenderViewToImage(
			     osgViewer::View* view )
{
    osg::Image* outputimage = 0;

    osgGeo::TiledOffScreenRenderer tileoffrenderer( view,
	visBase::DataObject::getCommonViewer() );
    tileoffrenderer.setOutputSize( mNINT32( sizepix_.width() ),
	mNINT32( sizepix_.height() ) );

    tileoffrenderer.setOutputBackgroundTransparency( 0 );

    if ( tileoffrenderer.createOutput() )
    {
	const osg::Image* outimg = tileoffrenderer.getOutput();

	if ( outimg )
	    outputimage = (osg::Image*)outimg->clone(
	    osg::CopyOp::DEEP_COPY_ALL );

    }

    return outputimage;
}


bool uiPrintSceneDlg::hasImageValidFormat(const osg::Image* image)
{
    bool ret = true;

    if ( ( image->getPixelFormat() != GL_RGBA &&
	 image->getPixelFormat() !=GL_RGB ) ||
	 !image->isDataContiguous() )
    {
	pErrMsg( "Image is in the wrong format." ) ;
	ret = false;
    }

    return ret;
}


void uiPrintSceneDlg::flipImageVertical(osg::Image* image)
{
    if ( !image ) return;

    if ( image->getOrigin()==osg::Image::BOTTOM_LEFT )
    {
	image->flipVertical();
    }

}


void uiPrintSceneDlg::writeToSettings()
{
    IOPar iopar;
    fillPar( iopar, false );
    settings_.mergeComp( iopar, "3D" );
    if ( !settings_.write() )
	uiMSG().error( tr("Cannot write settings") );
}


const char* uiPrintSceneDlg::getExtension()
{
    return uiSaveImageDlg::getExtension();
}

