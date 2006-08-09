/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Jul 2006
-*/

static const char* rcsID = "$Id: tableconv.cc,v 1.8 2006-08-09 17:27:33 cvsbert Exp $";

#include "tableconvimpl.h"


void TableImportHandler::addToCol( char c )
{
    if ( colpos_ < col_.bufSize()-1 )
	*(col_.buf() + colpos_) = c;
    else
    {
	*(col_.buf() + colpos_) = '\0';
	char buf[3]; buf[0] = c; buf[1] = ' '; buf[2] = '\0';
	col_ += buf;
    }
    colpos_++;
}


bool TableExportHandler::isNumber( const char* str )
{
    return isNumberString( str, NO );
}


bool TableExportHandler::init( std::ostream& strm )
{
    if ( *prepend_.buf() )
	strm << prepend_;
    return strm.good();
}


void TableExportHandler::finish( std::ostream& strm )
{
    if ( *append_.buf() )
	strm << append_;
}


int TableConverter::nextStep()
{
    if ( selcolnr_ == -1 && !exphndlr_.init(ostrm_) )
    {
	msg_ = "Cannot write first output";
	return -1;
    }

    selcolnr_ = colnr_ = 0;

    while ( true )
    {
	char c = readNewChar();
	if ( istrm_.eof() )
	{
	    exphndlr_.finish( ostrm_ );
	    return 0;
	}

	TableImportHandler::State impstate = imphndlr_.add( c );
	if ( !handleImpState(impstate) )
	    return -1;
	if ( impstate == TableImportHandler::EndRow )
	    return 1;
    }
}


bool TableConverter::handleImpState( TableImportHandler::State impstate )
{
    switch ( impstate )
    {

    case TableImportHandler::Error:

	msg_ = imphndlr_.errMsg();

    return false;

    case TableImportHandler::EndCol: {

	if ( colSel() )
	{
	    row_.add( imphndlr_.getCol() );
	    selcolnr_++;
	}
	colnr_++;
	imphndlr_.newCol();

    return true; }

    case TableImportHandler::EndRow: {

	if ( !handleImpState(TableImportHandler::EndCol) )
	    return false;

	if ( !manipulator_ || manipulator_->accept(row_) )
	{
	    const char* msg = exphndlr_.putRow( row_, ostrm_ );
	    if ( msg && *msg )
	    {
		msg_ = msg;
		return false;
	    }

	    rowsdone_++;
	}
	row_.deepErase();
	imphndlr_.newRow();

    return true; }

    default:
    break;

    }

    return true;

}


TableImportHandler::State WSTableImportHandler::add( char c )
{
    if ( c == '"' && !insingqstring_ )
	{ indoubqstring_ = !indoubqstring_; return InCol; }
    else if ( c == '\'' && !indoubqstring_ )
	{ insingqstring_ = !insingqstring_; return InCol; }
    else if ( c == '\n' )
    {
	indoubqstring_ = insingqstring_ = false;
	addToCol( '\0' );
	return EndRow;
    }
    else if ( isspace(c) )
    {
	if ( *col_.buf() )
	{
	    addToCol( '\0' );
	    return EndCol;
	}
	return InCol;
    }

    addToCol( c );
    return InCol;
}


TableImportHandler::State CSVTableImportHandler::add( char c )
{
    if ( c == '"' )
	{ instring_ = !instring_; return InCol; }
    else if ( c == '\n' )
    {
	if ( instring_ )
	    c = nlreplace_;
	else
	{
	    addToCol( '\0' );
	    return EndRow;
	}
    }
    else if ( c == ',' && !instring_ )
    {
	addToCol( '\0' );
	return EndCol;
    }

    addToCol( c );
    return InCol;
}


void WSTableExportHandler::addVal( std::ostream& strm, int col,
				   const char* val )
{
    if ( col )
	strm << '\t';
    const bool needsquotes = !*val || strcspn( val, " \t" );
    const char quotechar = usesingquotes_ ? '\'' : '"';
    if ( strchr( val, quotechar ) )
	replaceCharacter( (char*)val, quotechar, '`' );

    if ( needsquotes )
	strm << quotechar;
    if ( *val )
	strm << val;
    if ( needsquotes )
	strm << quotechar;
}


const char* WSTableExportHandler::putRow( const BufferStringSet& row,
					  std::ostream& strm )
{
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( strm, idx, row.get(idx) );
    strm << std::endl;

    return strm.good() ? 0 : "Error writing to output";
}


void CSVTableExportHandler::addVal( std::ostream& strm, int col,
				    const char* val )
{
    if ( col )
	strm << ',';
    const bool needsquotes = *val && !isNumber( val );
    if ( needsquotes )
	strm << '"';
    if ( *val )
	strm << val;
    if ( needsquotes )
	strm << '"';
}


const char* CSVTableExportHandler::putRow( const BufferStringSet& row,
					   std::ostream& strm )
{
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( strm, idx, row.get(idx) );
    strm << std::endl;

    return strm.good() ? 0 : "Error writing to output";
}


void SQLInsertTableExportHandler::addVal( std::ostream& strm, int col,
					  const char* val )
{
    if ( col )
	strm << ',';
    const bool needsquotes = !*val || !isNumber( val );
    if ( needsquotes )
	strm << "'";
    if ( val && *val )
	strm << val;
    if ( needsquotes )
	strm << "'";
}


const char* SQLInsertTableExportHandler::putRow( const BufferStringSet& row,
						 std::ostream& strm )
{
    if ( nrrows_ == 0 )
    {
	if ( tblname_ == "" )
	    return "No table name provided";
	addindex_ = indexcolnm_ != "";
	nrextracols_ = extracolnms_.size();
    }

    strm << "INSERT INTO " << tblname_;
    if ( colnms_.size() )
    {
	strm << " (";

	if ( addindex_ )
	    strm << indexcolnm_;
	else
	    strm << (nrextracols_ ? extracolnms_ : colnms_).get(0).buf();

	int idx0 = addindex_ ? 0 : 1;
	for ( int idx=idx0; idx<nrextracols_; idx++ )
	    strm << ',' << extracolnms_.get(idx).buf();
	idx0 = addindex_ || nrextracols_ > 0 ? 0 : 1;
	for ( int idx=idx0; idx<colnms_.size(); idx++ )
	    strm << ',' << colnms_.get(idx).buf();

	strm << ')';
    }

    strm << " VALUES (";

    if ( addindex_ )
	strm << startindex_ + nrrows_ * stepindex_;
    int idxoffs = addindex_ ? 1 : 0;
    for ( int idx=0; idx<nrextracols_; idx++ )
	addVal( strm, idx+idxoffs, extracolvals_.get(idx) );
    idxoffs += nrextracols_;
    for ( int idx=0; idx<row.size(); idx++ )
	addVal( strm, idx+idxoffs, row.get(idx) );

    strm << ");" << std::endl;

    nrrows_++;
    return strm.good() ? 0 : "Error writing to output";
}


bool TCEmptyFieldRemover::accept( BufferStringSet& cols ) const
{
    for ( int idx=0; idx<ckcols_.size(); idx++ )
    {
	if ( cols.get(ckcols_[idx]) == "" )
	    return false;
    }
    return true;
}


void TCDuplicateKeyRemover::setPrevKeys( const BufferStringSet& cols ) const
{
    if ( prevkeys_.size() < 1 )
    {
	for ( int idx=0; idx<keycols_.size(); idx++ )
	    prevkeys_.add( "" );
    }

    for ( int idx=0; idx<keycols_.size(); idx++ )
	prevkeys_.get(idx) = cols.get( keycols_[idx] );
}


bool TCDuplicateKeyRemover::accept( BufferStringSet& cols ) const
{
    nrdone_++;
    if ( keycols_.size() < 1 )
	return true;

    if ( nrdone_ == 1 )
	{ setPrevKeys( cols ); return true; }

    for ( int idx=0; idx<keycols_.size(); idx++ )
    {
	if ( cols.get(keycols_[idx]) != prevkeys_.get(idx) )
	    { setPrevKeys( cols ); return true; }
    }

    nrremoved_++;
    return false;
}
