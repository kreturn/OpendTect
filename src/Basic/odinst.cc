/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2012
________________________________________________________________________

-*/


#include "odinst.h"
#include "file.h"
#include "filepath.h"
#include "oddirs.h"
#include "odplatform.h"
#include "envvars.h"
#include "od_istream.h"
#include "oscommand.h"
#include "settings.h"
#include "perthreadrepos.h"
#include "bufstringset.h"

#define mDeclEnvVarVal const char* envvarval = GetEnvVar("OD_INSTALLER_POLICY")
#define mRelRootDir GetSoftwareDir(1)

#ifdef __win__
#include <Windows.h>
#include <direct.h>
#include "winutils.h"
static BufferString getInstDir()
{
    BufferString dirnm( _getcwd(NULL,0) );
    char* termchar = 0;
    termchar = dirnm.find( "\\bin\\win" );
    if ( !termchar )
	termchar = dirnm.find( "\\bin\\Win" );

    if ( termchar )
	*termchar = '\0';
    return dirnm;
}
#undef mRelRootDir
# define mRelRootDir getInstDir()
#else
# include "unistd.h"
# ifndef OD_NO_QT
#  include <QProcess>
# endif
#endif

mDefineNameSpaceEnumUtils(ODInst,AutoInstType,"Auto update")
{ "Manager", "Inform", "Full", "None", 0 };


mDefineNameSpaceEnumUtils(ODInst,RelType,"Release type")
{
	"Stable",
	"Development",
	"Pre-Release Stable",
	"Pre-Release Development",
	"Old Version",
	"Other",
	0
};


BufferString ODInst::GetRelInfoDir()
{
#ifdef __mac__
    return File::Path( GetSoftwareDir(true), "Resources", "relinfo" ).fullPath();
#else
    return File::Path( GetSoftwareDir(true), "relinfo" ).fullPath();
#endif
}


ODInst::RelType ODInst::getRelType()
{
    File::Path relinfofp( GetRelInfoDir(), "README.txt" );
    const BufferString reltxtfnm( relinfofp.fullPath() );
    od_istream strm( reltxtfnm );
    if ( !strm.isOK() )
	return ODInst::Other;

    BufferString appnm, relstr;
    strm.getWord( appnm, false );
    strm.getWord( relstr, false );
    const int relsz = relstr.size();
    if ( appnm[0] != '[' || relsz < 4 || relstr[0] != '('
	|| relstr[relsz-1] != ']' || relstr[relsz-2] != ')' )
	return ODInst::Other;

    relstr[relsz-2] = '\0';
    return ODInst::RelTypeDef().parse( relstr.buf()+1 );
}


const char* sAutoInstTypeUserMsgs[] = {
    "[&Manager] Start the Installation Manager when updates are available",
    "[&Inform] When new updates are present, show this in OpendTect title bar",
    "[&Auto] Automatically download and install new updates "
	"(requires sufficient administrator rights)",
    "[&None] Never check for updates", 0 };


const BufferStringSet& ODInst::autoInstTypeUserMsgs()
{
    mDefineStaticLocalObject( BufferStringSet, ret, (sAutoInstTypeUserMsgs) );
    return ret;
}

const char* ODInst::sKeyAutoInst() { return ODInst::AutoInstTypeDef().name(); }


bool ODInst::canInstall()
{
    return File::isWritable( mRelRootDir );
}


#define mDefCmd(errretval) \
    File::Path installerdir( getInstallerPlfDir() ); \
    if ( !File::isDirectory(installerdir.fullPath()) ) \
	return errretval; \
    if ( __iswin__ ) \
	installerdir.add( "od_instmgr.exe" ); \
    else if ( __islinux__ ) \
	installerdir.add( "run_installer" ); \
    BufferString cmd( installerdir.fullPath() ); \
    if ( !File::isExecutable(cmd) ) \
        return errretval; \
    cmd.add( " --instdir " ).add( "\"" ).add( mRelRootDir ).add( "\"" ); \


BufferString ODInst::GetInstallerDir()
{
    BufferString appldir( GetSoftwareDir(0) );
    if ( File::isLink(appldir) )
	appldir = File::linkEnd( appldir );

    File::Path installerdir( appldir );
    installerdir.setFileName( mInstallerDirNm );
    return installerdir.fullPath();
}


void ODInst::startInstManagement()
{
#ifndef __win__
    mDefCmd();
    const BufferString curpath = File::getCurrentPath();
    File::changeDir( installerdir.pathOnly() );
    OS::ExecCommand( cmd, OS::RunInBG );
    File::changeDir( curpath.buf() );
#else
    File::Path installerdir( getInstallerPlfDir() );
    if ( installerdir.isEmpty() )
	return;
    installerdir.add( "od_instmgr" );
    BufferString cmd( installerdir.fullPath() );
    BufferString parm( " --instdir "  );
    parm.add( "\"" ).add( mRelRootDir ).add( "\"" );

    executeWinProg( cmd, parm, installerdir.pathOnly() );
#endif
}


void ODInst::startInstManagementWithRelDir( const char* reldir )
{
#ifdef __win__
    File::Path installerdir( getInstallerPlfDir() );
    if ( installerdir.isEmpty() )
	return;
    installerdir.add( "od_instmgr" );
    BufferString cmd( installerdir.fullPath() );
    BufferString parm( " --instdir "  );
    parm.add( "\"" ).add( reldir ).add( "\"" );

    executeWinProg( cmd, parm, installerdir.pathOnly() );
#endif
}


BufferString ODInst::getInstallerPlfDir()
{
    File::Path installerbasedir( GetInstallerDir() );
    if ( !File::isDirectory(installerbasedir.fullPath()) )
	return "";
    File::Path installerdir ( installerbasedir, "bin", __plfsubdir__, "Release" );
    const BufferString path = installerdir.fullPath();
    if ( !File::exists(path) || !File::isDirectory(path) )
	return installerbasedir.fullPath();

    return installerdir.fullPath();
}


bool ODInst::runInstMgrForUpdt()
{
    mDefCmd(false);
    cmd.add( " --updcheck_report" );
    return OS::ExecCommand( cmd, OS::Wait4Finish );
}


bool ODInst::updatesAvailable()
{
#ifdef OD_NO_QT
    return false;
#else
    mDefCmd(false); cmd.add( " --updcheck_report" );
# ifndef __win__
    const BufferString curpath = File::getCurrentPath();
    File::changeDir( installerdir.pathOnly() );
    const int res = QProcess::execute( QString(cmd.buf()) );
    File::changeDir( curpath.buf() );
    return res == 1;
# else
    File::Path tmp( File::getTempPath(), "od_updt" );
    mDefineStaticLocalObject(const bool,ret, = File::exists(tmp.fullPath()));
    if ( ret )
	File::remove( tmp.fullPath() );
    return ret;
# endif
#endif
}


const char* ODInst::getPkgVersion( const char* file_pkg_basenm )
{
    mDeclStaticString( ret );
    const BufferString part1( "ver.", file_pkg_basenm );
    BufferString fnm = part1;
    fnm.add( "_" ).add( OD::Platform::local().shortName() );
    File::Path fp( GetRelInfoDir(), fnm );
    fp.setExtension( "txt", false );

    fnm = fp.fullPath();
    if ( !File::exists(fnm) )
    {
	fp.setFileName( part1 ); fp.setExtension( "txt", false );
	fnm = fp.fullPath();
	if ( !File::exists(fnm) )
	    { ret = "[error: version file not found]"; return ret.buf(); }
    }

    File::getContent( fnm, ret );
    if ( ret.isEmpty() )
	ret = "[error: empty version file]";
    return ret.buf();
}


bool ODInst::autoInstTypeIsFixed()
{
    mDeclEnvVarVal;
    return envvarval && *envvarval;
}


ODInst::AutoInstType ODInst::getAutoInstType()
{
    mDeclEnvVarVal;
    const char* res = envvarval && *envvarval ? envvarval
			: userSettings().find( sKeyAutoInst() );
    return res && *res ? AutoInstTypeDef().parse( res ) : ODInst::InformOnly;
}


void ODInst::setAutoInstType( ODInst::AutoInstType ait )
{
    userSettings().set( sKeyAutoInst(), ODInst::toString(ait) );
    userSettings().write();
}


Settings& ODInst::userSettings()
{
    return Settings::fetch( "instmgr" );
}
