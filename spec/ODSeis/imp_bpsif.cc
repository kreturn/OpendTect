/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : Bert
 * DATE     : July 2007
 * RCS      : $Id: imp_bpsif.cc,v 1.1 2008-01-10 13:12:35 cvsbert Exp $
-*/

#include "seisimpbpsif.h"
#include "multiid.h"

#include "prog.h"


static int doWork( int argc, char** argv )
{
    if ( argc < 3 )
    {
	std::cerr << "Usage: " << argv[0] << " inp_BPSIF_file output_id\n";
	return 1;
    }

    SeisImpBPSIF imp( argv[1], MultiID(argv[2]) );
    return imp.execute( &std::cout ) ? 0 : 1;
}


int main( int argc, char** argv )
{
    return ExitProgram( doWork(argc,argv) );
}
