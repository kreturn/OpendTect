/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Oct 2007
________________________________________________________________________

-*/


#include "prog.h"
#include "uimsg.h"
#include "uimain.h"
#include "uistrings.h"
#include "commandlineparser.h"

#ifdef __msvc__
#include "winmain.h"
#endif


int main( int argc, char** argv )
{
    SetProgramArgs( argc, argv );
    CommandLineParser parser;

    if ( parser.nrArgs()<1 )
	return ExitProgram( 1 );

    int typ = 0; //Default is info
    if ( parser.hasKey( "warn" ) )
	typ = 1;
    else if ( parser.hasKey( "err" ) )
	typ = 2;
    else if ( parser.hasKey( "ask" ))
	typ = 3;

    BufferStringSet normalargs;
    parser.getNormalArguments( normalargs );

    uiString msg = uiString::emptyString();
    for ( int idx=0; idx<normalargs.size(); idx++ )
    {
	BufferString nextarg( normalargs[idx]->buf() );
	nextarg.replace( "-+-", "\n" );
	msg.append( nextarg );
	if ( idx<normalargs.size()-1 )
	    msg.append( toUiString(" ") );
    }
    if ( msg.isEmpty() )
	msg = typ == 1 ? od_static_tr("main", "Be careful!")
	    : (typ ==2 ? od_static_tr("main", "Problem found!")
		       : od_static_tr("main", "Your answer:"));

    uiMain app( argc, argv );
    if ( typ == 0 )
	uiMSG().message( msg );
    else if ( typ == 1 )
	uiMSG().warning( msg );
    else if ( typ == 2 )
	uiMSG().error( msg );
    else if ( typ == 3 )
    {
	msg = getUiYesNoString(uiMSG().askGoOn(msg));
	od_cout() << msg << od_endl;
    }

    return ExitProgram( 0 );
}
