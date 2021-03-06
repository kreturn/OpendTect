/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Mar 2004
-*/


#include "filepath.h"

#include "file.h"
#include "genc.h"
#include "msgh.h"
#include "winutils.h"
#include "fixedstring.h"
#include "oddirs.h"
#include <time.h>
#include <string.h>


const char* File::Path::sPrefSep = ":";

static const File::Path::Style cOther = __iswin__ ? File::Path::Unix
						  : File::Path::Windows;

File::Path::Path( const char* fnm )
{
    set( fnm );
}


File::Path::Path( const char* p1, const char* p2, const char* p3,
		    const char* p4, const char* p5 )
{
    set( p1 );
    addPart( p2 ); addPart( p3 ); addPart( p4 ); addPart( p5 );
    compress();
}


File::Path::Path( const Path& fp, const char* p2, const char* p3,
		    const char* p4, const char* p5 )
{
    *this = fp;
    addPart( p2 ); addPart( p3 ); addPart( p4 ); addPart( p5 );
    compress();
}


File::Path& File::Path::operator =( const Path& fp )
{
    lvls_ = fp.lvls_;
    prefix_ = fp.prefix_;
    isabs_ = fp.isabs_;
    isuri_ = fp.isuri_;
    return *this;
}


File::Path& File::Path::operator =( const char* fnm )
{ return (*this = Path(fnm)); }


bool File::Path::operator ==( const Path& fp ) const
{
    return lvls_ == fp.lvls_ && prefix_ == fp.prefix_ && isabs_ == fp.isabs_;
}


bool File::Path::operator ==( const char* fnm ) const
{ return *this == Path(fnm); }


bool File::Path::operator !=( const Path& fp ) const
{ return !(*this == fp); }


bool File::Path::operator != ( const char* fnm ) const
{ return !(*this == Path(fnm)); }


File::Path& File::Path::set( const char* _fnm )
{
    lvls_.erase(); prefix_.setEmpty(); isabs_ = isuri_ = false;
    if ( !_fnm || !*_fnm )
	return *this;

    const BufferString fnmbs( _fnm );
    const char* fnm = fnmbs.buf();
    mSkipBlanks( fnm );
    if ( !*fnm )
	return *this;

    isuri_ = File::isURI( fnm );
    if ( isuri_ )
    {
	BufferString hostpart( fnm );
	char* ptr = hostpart.find( "//" ) + 2;
	ptr = firstOcc( ptr, '/' );
	if ( ptr ) *ptr = '\0';
	prefix_ = hostpart;
	fnm += prefix_.size();
	if ( ptr ) fnm++;
	isabs_ = true;
    }
    else
    {
	const char* ptr = firstOcc( fnm, *sPrefSep );
	if ( ptr )
	{
	    const char* dsptr = firstOcc( fnm, *dirSep(Local) );
	    const char* otherdsptr = firstOcc( fnm, *dirSep(cOther) );
	    if ( otherdsptr && ( !dsptr || otherdsptr < dsptr ) )
		dsptr = otherdsptr;

	    if ( dsptr > ptr )
	    {
		prefix_ = fnm;
		*firstOcc( prefix_.getCStr(), *sPrefSep ) = '\0';
		fnm = ptr + 1;
	    }
	}

	isabs_ = *fnm == '\\' || *fnm == '/';
	if ( isabs_ ) fnm++;
    }

    addPart( fnm );
    compress();
    return *this;
}


File::Path& File::Path::add( const char* fnm )
{
    if ( !fnm || !*fnm )
	return *this;

    int sl = lvls_.size();
    addPart( fnm );
    compress( sl );

    return *this;
}


File::Path& File::Path::insert( const char* fnm )
{
    if ( !fnm || !*fnm )
	return *this;

    BufferStringSet oldlvls( lvls_ );
    lvls_.setEmpty();
    set( fnm );
    lvls_.append( oldlvls );
    return *this;
}


File::Path& File::Path::setFileName( const char* fnm )
{
    if ( !fnm || !*fnm )
    {
	if ( lvls_.size() )
	    lvls_.removeSingle( lvls_.size()-1 );
    }
    else if ( lvls_.isEmpty() )
	add( fnm );
    else
    {
	*lvls_[lvls_.size()-1] = fnm;
	compress( lvls_.size()-1 );
    }
    return *this;
}


File::Path& File::Path::setPath( const char* pth )
{
    BufferString fnm( lvls_.size() ?
	    lvls_.get(lvls_.size()-1).buf() : (const char*) 0 );
    set( pth );
    if ( !fnm.isEmpty() )
	add( fnm );
    return *this;
}


File::Path& File::Path::setExtension( const char* ext, bool replace )
{
    if ( !ext ) ext = "";
    mSkipBlanks( ext );

    if ( *ext == '.' )
	ext++;
    if ( lvls_.size() < 1 )
    {
	if ( *ext )
	    add( ext );
	return *this;
    }

    BufferString& fname = *lvls_[lvls_.size()-1];
    char* ptr = lastOcc( fname.getCStr(), '.' );
    if ( ptr && replace )
	strcpy( *ext ? ptr+1 : ptr, ext );
    else if ( *ext )
	{ fname += "."; fname += ext; }
    return *this;
}


bool File::Path::isAbsolute() const
{ return isabs_; }


bool File::Path::isSubDirOf( const Path& b, Path* relpath ) const
{
    if ( b.isAbsolute()!=isAbsolute() )
	return false;

    if ( FixedString(b.prefix())!=prefix() )
	return false;

    const int nrblevels = b.nrLevels();
    if ( nrblevels>=nrLevels() )
	return false;

    for ( int idx=0; idx<nrblevels; idx++ )
    {
	if ( *lvls_[idx]!=*b.lvls_[idx] )
	    return false;
    }

    if ( relpath )
    {
	BufferString rel;
	for ( int idx=nrblevels; idx<nrLevels(); idx++ )
	{
	    if ( idx>nrblevels )
		rel.add( dirSep() );
	    rel.add( dir( idx ) );
	}

	relpath->set( rel.buf() );
    }

    return true;
}


bool File::Path::makeCanonical()
{
    BufferString fullpath = fullPath();
#ifndef __win__
    set( File::getCanonicalPath( fullpath.buf() ) );
#else
    BufferString winpath = File::getCanonicalPath( fullpath.buf() );
    winpath.replace( '/', '\\' );
    set( winpath );
#endif
    return true;
}


bool File::Path::makeRelativeTo( const Path& oth )
{
    const BufferString file = fullPath();
    const BufferString path = oth.fullPath();
    set( File::getRelativePath( path.buf(), file.buf() ) );
    return true;
}



BufferString File::Path::fullPath( Style f, bool cleanup ) const
{
    const BufferString res = dirUpTo(-1);
    return cleanup ? mkCleanPath(res,f) : res;
}


const char* File::Path::prefix() const
{
    return prefix_.buf();
}


int File::Path::nrLevels() const
{
    return lvls_.size();
}


const char* File::Path::extension() const
{
    if ( lvls_.isEmpty() )
	return 0;

    const char* ret = lastOcc( fileName().buf(), '.' );
    if ( ret ) ret++;
    return ret;
}


const OD::String& File::Path::fileName() const
{ return dir(-1); }


BufferString File::Path::baseName() const
{
    BufferString ret = fileName();
    if ( ret.isEmpty() )
	return ret;

    char* ptr = ret.getCStr();
    char* lastdot = lastOcc( ptr, '.' );
    if ( lastdot ) *lastdot = '\0';
    return ret;
}


BufferString File::Path::getTimeStampFileName( const char* ext )
{
    BufferString tsfnm;
    BufferString datestr = Time::getDateTimeString();
    datestr.replace( ", ", "-" );
    datestr.replace( ':', '.' );
    datestr.replace( ' ', '_' );
    tsfnm += datestr.buf();
    tsfnm += ext;

    return tsfnm;
}


BufferString File::Path::pathOnly() const
{ return dirUpTo(lvls_.size()-2); }


const OD::String& File::Path::dir( int nr ) const
{
    if ( nr < 0 || nr >= lvls_.size() )
	nr = lvls_.size()-1;
    return nr < 0 ? BufferString::empty() : *lvls_[nr];
}


BufferString File::Path::dirUpTo( int lvl ) const
{
    if ( lvl < 0 || lvl >= lvls_.size() )
	lvl = lvls_.size() - 1;

    BufferString ret;
    if ( !prefix_.isEmpty() )
    {
	ret.set( prefix_ );
	if ( !isuri_ )
	    ret.add( sPrefSep );
    }
    if ( lvl < 0 )
	return ret;

    if ( isabs_ )
	ret.add( dirSep() );
    if ( lvls_.size() )
	ret += lvls_.get( 0 );

    for ( int idx=1; idx<=lvl; idx++ )
    {
	ret += dirSep();
	ret += lvls_.get( idx );
    }

    return ret;
}


BufferString File::Path::getTempDir()
{
    BufferString tmpdir = File::getTempPath();
    if ( !File::exists(tmpdir) )
    {
	BufferString msg( "Temporary directory '", tmpdir, "'does not exist" );
	UsrMsg( msg );
    }
    else if ( !File::isWritable(tmpdir) )
    {
	BufferString msg( "Temporary directory '", tmpdir, "'is read-only" );
	UsrMsg( msg );
    }

    return tmpdir;
}


BufferString File::Path::getTempName( const char* ext )
{
    Path fp( getTempDir() );

    BufferString fname( "od", GetPID() );
    mDefineStaticLocalObject( int, counter, = 0 );
    time_t time_stamp = time( (time_t*)0 ) + counter++;
    fname += (od_int64)time_stamp;

    if ( ext && *ext )
    {
	fname += ".";
	fname += ext;
    }

    fp.add( fname );
    return fp.fullPath();
}


BufferString File::Path::mkCleanPath( const char* path, Style stl )
{
    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    BufferString ret( path );
    if ( stl == Windows && !__iswin__ )
	ret = getCleanWinPath( path );
    if ( stl == Unix && __iswin__ )
	ret = getCleanUnxPath( path );

    return ret;
}

static const char* winds = "\\";
static const char* unixds = "/";

const char* File::Path::dirSep() const
{
    return isuri_ ? unixds : dirSep( Local );
}


const char* File::Path::dirSep( Style stl )
{
    if ( stl == Local )
	stl = __iswin__ ? Windows : Unix;

    return stl == Windows ? winds : unixds;
}


void File::Path::addPart( const char* fnm )
{
    if ( !fnm || !*fnm ) return;

    mSkipBlanks( fnm );
    const int maxlen = strLength( fnm );
    char prev = ' ';
    char* buf = new char [maxlen+1]; *buf = '\0';
    char* bufptr = buf;
    bool remdblsep = false;

    while ( *fnm )
    {
	char cur = *fnm;

	if ( cur != *dirSep(Local) && cur != *dirSep(cOther) )
	    remdblsep = true;
	else
	{
	    if ( (prev != *dirSep(Local) && prev != *dirSep(cOther))
		    || !remdblsep )
	    {
		*bufptr = '\0';
		if ( buf[0] ) lvls_.add( buf );
		bufptr = buf;
		*bufptr = '\0';
	    }
	    fnm++;
	    continue;
	}

	*bufptr++ = cur;
	fnm++;
	prev = cur;
    }
    *bufptr = '\0';
    if ( buf[0] ) lvls_.add( buf );
    delete [] buf;
    conv2TrueDirIfLink();
}


void File::Path::compress( int startlvl )
{
    for ( int idx=startlvl; idx<lvls_.size(); idx++ )
    {
	const BufferString& bs = *lvls_[idx];
	int remoffs = 99999;
	if ( bs == "." )
	    remoffs = 0;
	else if ( bs == ".." && idx > 0 && *lvls_[idx-1] != ".." )
	    remoffs = 1;

	if ( idx-remoffs >= 0 )
	{
	    lvls_.removeRange( idx-remoffs, idx );
	    idx -= remoffs + 1;
	}
    }
}


void File::Path::conv2TrueDirIfLink()
{
#ifdef __win__
    BufferString dirnm = dirUpTo( -1 );
    if ( File::exists(dirnm) )
	return;

    dirnm += ".lnk";
    if ( File::exists(dirnm) && File::isLink(dirnm) )
	set( File::linkEnd(dirnm) );
#endif
}


BufferString File::Path::winDrive() const
{
    BufferString windrive = File::getRootPath( fullPath() );
    return windrive;
}
