/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne/Bert
 Date:          June 2001/Oct 2016
________________________________________________________________________

-*/

#include "uiusercreatesurvey.h"

#include "uisurvinfoed.h"

#include "uichecklist.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uibutton.h"
#include "uilabel.h"
#include "uimsg.h"
#include "ui2dsip.h"

#include "survinfo.h"
#include "dbman.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"


static BufferString basicSurveyFullPath()
{
    const File::Path fp( mGetSWDirDataDir(), SurveyInfo::sBasicSurveyName() );
    return fp.fullPath();
}


#define mDataRootDir() dr && *dr ? dr : GetBaseDataDir()


uiUserCreateSurvey::uiUserCreateSurvey( uiParent* p, const char* dr )
	: uiDialog(p,uiDialog::Setup(tr("Create New Survey"),
		    tr("Data root directory: %1").arg(mDataRootDir()),
		    mODHelpKey(mStartNewSurveySetupHelpID)))
	, survinfo_(0)
	, dataroot_(mDataRootDir())
	, sips_(uiSurveyInfoEditor::survInfoProvs())
{
    uiRetVal uirv;
    survinfo_ = SurveyInfo::read( basicSurveyFullPath(), uirv );
    if ( !survinfo_ )
	uirv.insert( tr("Cannot read default empty survey from installation.\n"
		      "This may be a permission problem,"
		      "\nor your installation may be incomplete.\n") );
    if ( !uirv.isOK() )
    {
	new uiLabel( this, uirv );
	setOkText( uiString::emptyString() );
	setCancelText( uiStrings::sClose() );
    }

    setOkText( uiStrings::sNext() );

    survnmfld_ = new uiGenInput( this, tr("Survey name") );
    survnmfld_->setElemSzPol( uiObject::Wide );

    pol2dfld_ = new uiCheckList( this, uiCheckList::OneMinimum, OD::Horizontal);
    pol2dfld_->setLabel( tr("Available data") );
    pol2dfld_->addItem( uiStrings::s3D() ).addItem( uiStrings::s2D() );
    pol2dfld_->setChecked( 0, true ).setChecked( 1, true );
    pol2dfld_->changed.notify( mCB(this,uiUserCreateSurvey,pol2dChg) );
    pol2dfld_->attach( alignedBelow, survnmfld_ );

    for ( int idx=0; idx<sips_.size(); idx++ )
    {
	if ( !sips_[idx]->isAvailable() )
	    { sips_.removeSingle( idx ); idx--; }
    }

    uiListBox::Setup su( OD::ChooseOnlyOne, tr("Initial setup") );
    sipfld_ = new uiListBox( this, su );
    sipfld_->attach( alignedBelow, pol2dfld_ );
    sipfld_->setPrefHeightInChar( sips_.size() + 1 );

    zistimefld_ = new uiGenInput( this, tr("Z Domain"),
		BoolInpSpec(true,uiStrings::sTime(),uiStrings::sDepth()) );
    zistimefld_->valuechanged.notify(
			mCB(this,uiUserCreateSurvey,zdomainChg) );
    zistimefld_->attach( alignedBelow, sipfld_ );

    zinfeetfld_ = new uiGenInput( this, tr("Depth unit"),
				BoolInpSpec(true,tr("Meter"),tr("Feet")) );
    zinfeetfld_->attach( alignedBelow, zistimefld_ );
    zinfeetfld_->display( !isTime() );

    fillSipsFld( has2D(), has3D() );
}


uiUserCreateSurvey::~uiUserCreateSurvey()
{
    delete survinfo_;
}


BufferString uiUserCreateSurvey::survName() const
{ return BufferString( survnmfld_->text() ); }
BufferString uiUserCreateSurvey::survDirName() const
{ return SurveyInfo::dirNameForName( survnmfld_->text() ); }
bool uiUserCreateSurvey::has3D() const
{ return pol2dfld_->isChecked(0); }
bool uiUserCreateSurvey::has2D() const
{ return pol2dfld_->isChecked(1); }
bool uiUserCreateSurvey::isTime() const
{ return zistimefld_->getBoolValue();}
bool uiUserCreateSurvey::isInFeet() const
{ return !zinfeetfld_->getBoolValue();}
void uiUserCreateSurvey::pol2dChg( CallBacker* cb )
{ fillSipsFld( has2D(), has3D() ); }
void uiUserCreateSurvey::zdomainChg( CallBacker* cb )
{ zinfeetfld_->display( !isTime() ); }


SurveyInfo::Pol2D uiUserCreateSurvey::pol2D() const
{
    return has3D() ? ( has2D() ? SurveyInfo::Both2DAnd3D
			       : SurveyInfo::No2D )
			       : SurveyInfo::Only2D;
}


uiString uiUserCreateSurvey::sipName() const
{
    const int sipidx = sipfld_->currentItem();
    return sips_.validIdx(sipidx) ? sips_[sipidx]->usrText()
				  : uiString::emptyString();
}


void uiUserCreateSurvey::fillSipsFld( bool have2d, bool have3d )
{
    int preferredsel = sipfld_->isEmpty() ? -1 : sipfld_->currentItem();
    sipfld_->setEmpty();

    const int nrprovs = sips_.size();
    for ( int idx=0; idx<nrprovs; idx++ )
    {
	uiSurvInfoProvider& sip = *sips_[idx];
	const uiString sipnm = sip.usrText();
	mDynamicCastGet(const ui2DSurvInfoProvider*,sip2d,&sip);

	if ( preferredsel < 0 )
	{
	    const BufferString workstr = sipnm.getFullString();
	    if ( workstr.contains("etrel") )
		preferredsel = idx;
	    else
	    {
		if ( sip2d && !have3d )
		    preferredsel = idx;
	    }
	}

	sipfld_->addItem( sipnm );
	const char* icnm = sip.iconName();
	if ( !icnm || !*icnm )
	    icnm = "empty";
	sipfld_->setIcon( idx, icnm );
	if ( !have2d && sip2d )
	    sipfld_->setItemSelectable( sipfld_->size()-1, false );
    }

    sipfld_->addItem( tr("Enter by hand") ); // always last
    sipfld_->setIcon( sipfld_->size()-1, "manualenter" );
    sipfld_->setCurrentItem( preferredsel < 0 ? 0 : preferredsel );

    int maxlen = 0;
    for ( int idx=0; idx<sipfld_->size(); idx++ )
    {
	const int len = FixedString( sipfld_->itemText(idx) ).size();
	if ( len > maxlen ) maxlen = len;
    }
    sipfld_->setPrefWidthInChar( maxlen + 5 );
}


#define mErrRet(s) { uiMSG().error(s); return false; }


bool uiUserCreateSurvey::usrInputOK()
{
    const BufferString survdirnm = survDirName();
    if ( survdirnm.isEmpty() )
	mErrRet(uiStrings::phrEnter(tr("a name for the new survey")))

    const BufferString storagedir
		= File::Path( dataroot_ ).add( survdirnm ).fullPath();
    if ( File::exists(storagedir) )
    {
	uiString errmsg = tr("A survey called %1 already exists\nPlease "
			     "remove it first or use another survey name")
			.arg( survName() );
	mErrRet( errmsg )
    }

    return true;
}


bool uiUserCreateSurvey::acceptOK()
{
    if ( !survinfo_ )
	return true;
    else if ( !usrInputOK() )
	return false;

    const BufferString survnm = survName();
    survinfo_->setName( survnm );
    survinfo_->setDirName( SurveyInfo::dirNameForName(survnm) );
    survinfo_->setBasePath( dataroot_ );
    survinfo_->setSurvDataType( pol2D() );
    survinfo_->setZUnit( isTime(), isInFeet() );
    survinfo_->setSipName( sipName() );

    const BufferString fullsurvpath = survinfo_->getFullDirPath();
    uiString errmsg;
    if ( !File::copyDir(basicSurveyFullPath(),fullsurvpath,&errmsg) )
    {
	uiRetVal uirv( tr("Cannot make a copy of the default survey") );
	uirv.add( errmsg );
	uiMSG().error( uirv );
	return false;
    }
    if ( !File::makeWritable(fullsurvpath,true,true) )
    {
	uiMSG().error( tr("Cannot make the survey directory:\n%1\nwriteable.")
		    .arg(fullsurvpath) );
	return false;
    }

    if ( !survinfo_->write() )
    {
	uiMSG().error( tr("Cannot write the survey parameters into "
		    "the new survey directory:\n%1").arg(fullsurvpath) );
	File::removeDir( fullsurvpath );
	return false;
    }

    const uiRetVal uirv = DBM().setDataSource( survinfo_->getFullDirPath(),
	    					true );
    if ( uirv.isError() )
	{ uiMSG().error( uirv ); return false; }

    return doUsrDef();
}


bool uiUserCreateSurvey::doUsrDef()
{
    IOPar iop; iop.setStdCreationEntries();
    iop.set( uiSurvInfoProvider::sKeySIPName(), survinfo_->sipName() );
    survinfo_->setFreshSetupData( iop );

    const_cast<SurveyInfo&>( SI() ) = *survinfo_;
    uiSurveyInfoEditor dlg( this, true );
    if ( !dlg.go() )
	File::removeDir( survinfo_->getFullDirPath() );

    return true;
}


BufferString uiUserCreateSurvey::dirName() const
{
    if ( !survinfo_ )
	return BufferString::empty();
    else
	return survinfo_->getDirName();
}
