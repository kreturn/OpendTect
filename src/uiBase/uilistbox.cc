/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          16/05/2000
 RCS:           $Id: uilistbox.cc,v 1.80 2007-07-11 06:48:43 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uilistbox.h"
#include "uifont.h"
#include "uilabel.h"
#include "uiobjbody.h"
#include "bufstringset.h"
#include "color.h"
#include "pixmap.h"

#include "i_q4listbox.h"
#include <QMouseEvent>

#define mNoSelection QAbstractItemView::NoSelection
#define mExtended QAbstractItemView::ExtendedSelection
#define mSingle QAbstractItemView::SingleSelection


class uiListBoxBody : public uiObjBodyImpl<uiListBox,QListWidget>
{

public:

                        uiListBoxBody(uiListBox& handle, 
				  uiParent* parnt=0, 
				  const char* nm="uiListBoxBody",
				  bool isMultiSelect=false,
				  int preferredNrLines=0,
				  int preferredFieldWidth=0);

    virtual 		~uiListBoxBody()		{ delete &messenger_; }

    void 		setLines( int prefNrLines, bool adaptVStretch )
			{ 
			    if( prefNrLines >= 0 ) prefnrlines_=prefNrLines;

			    if( adaptVStretch )
			    {
				int hs = stretch(true,true);
				setStretch( hs, (nrTxtLines()== 1) ? 0 : 2 );
			    }
			}

    virtual uiSize	minimumsize() const; //!< \reimp
    virtual int 	nrTxtLines() const
			    { return prefnrlines_ ? prefnrlines_ : 7; }

    int 		fieldwidth_;
    int 		prefnrlines_;

protected:
    void		mouseReleaseEvent(QMouseEvent*);

private:

    i_listMessenger&    messenger_;

};


uiListBoxBody::uiListBoxBody( uiListBox& handle, uiParent* parnt, 
			const char* nm, bool ismultiselect,
			int preferrednrlines, int preferredfieldwidth )
    : uiObjBodyImpl<uiListBox,QListWidget>( handle, parnt, nm, true )
    , messenger_(*new i_listMessenger(this,&handle))
    , fieldwidth_(preferredfieldwidth)
    , prefnrlines_(preferrednrlines)
{
    setObjectName( nm );
    setDragDropMode( QAbstractItemView::NoDragDrop );
    setAcceptDrops( false ); setDragEnabled( false );
    setSelectionBehavior( QAbstractItemView::SelectItems );
    if ( ismultiselect ) setSelectionMode( mExtended );

    setStretch( 2, (nrTxtLines()== 1) ? 0 : 2 );

    setHSzPol( uiObject::Medium );
}


uiSize uiListBoxBody::minimumsize() const
{ 
    const int totHeight = fontHgt() * prefnrlines_;
    const int totWidth  = fontWdt( true ) * fieldwidth_;
    return uiSize( totWidth, totHeight );
}


void uiListBoxBody::mouseReleaseEvent( QMouseEvent* event )
{
    if ( !event ) return;

    if ( event->button() == Qt::RightButton )
	handle_.buttonstate_ = OD::RightButton;
    else if ( event->button() == Qt::LeftButton )
	handle_.buttonstate_ = OD::LeftButton;
    else
	handle_.buttonstate_ = OD::NoButton;

    QListWidget::mouseReleaseEvent( event );
    handle_.buttonstate_ = OD::NoButton;
}


// -------------- uiListBox ---------------
uiListBox::uiListBox( uiParent* p, const char* nm, bool ms, int nl, int pfw )
    : uiObject( p, nm, mkbody(p,nm,ms,nl,pfw) )
    , buttonstate_(OD::NoButton)
    , selectionChanged(this)
    , doubleClicked(this)
    , rightButtonClicked(this)
    , leftButtonClicked(this)
{
}


uiListBox::uiListBox( uiParent* p, const BufferStringSet& items,
		      const char* txt, bool ms, int nl, int pfw )
    : uiObject( p, txt, mkbody(p,txt,ms,nl,pfw))
    , buttonstate_(OD::NoButton)
    , selectionChanged(this)
    , doubleClicked(this)
    , rightButtonClicked(this)
    , leftButtonClicked(this)
{
    addItems( items );
}


static const int sIconSz = 16;

uiListBoxBody& uiListBox::mkbody( uiParent* p, const char* nm, bool ms,
				  int nl, int pfw )
{
    body_ = new uiListBoxBody(*this,p,nm,ms,nl,pfw);
    body_->setIconSize( QSize(sIconSz,sIconSz) );
    return *body_;
}


uiListBox::~uiListBox()
{}


void uiListBox::setLines( int prefnrlines, bool adaptvstretch )
{ body_->setLines( prefnrlines, adaptvstretch ); }

void uiListBox::setNotSelectable()
{ body_->setSelectionMode( mNoSelection ); }

void uiListBox::setMultiSelect( bool yn )
{ body_->setSelectionMode( yn ? mExtended : mSingle ); }

int uiListBox::size() const
{ return body_->count(); }

bool uiListBox::validIndex( int idx ) const
{ return idx>=0 && idx<body_->count(); }


bool uiListBox::isSelected ( int idx ) const
{
    if ( !validIndex(idx) ) return false;

    QListWidgetItem* itm = body_->item( idx );
    return itm ? itm->isSelected() : false;
}


int uiListBox::nrSelected() const
{
    int res = 0;
    for ( int idx=0; idx<size(); idx++ )
	{ if ( isSelected(idx) ) res++; }
    return res;
}


void uiListBox::setSelected( int idx, bool yn )
{
    if ( validIndex(idx) )
	body_->item( idx )->setSelected( yn );
}


void uiListBox::selectAll( bool yn )
{
    if ( yn && body_->selectionMode()!=mExtended ) return;

    if ( yn )
	body_->selectAll();
    else
	body_->clearSelection();
}


void uiListBox::createQString( QString& qs, const char* str, bool embed ) const
{
    if ( !str ) str = "";
    BufferString bs( str );
    if ( embed ) { bs = "["; bs += str; bs += "]"; }
    qs = bs.buf();
}


void uiListBox::addItem( const char* text, bool embed ) 
{
    QString qs;
    createQString( qs, text, embed );
    body_->addItem( qs );
}


void uiListBox::addItem( const char* text, const ioPixmap& pm )
{
    addItem( text, false );
    setPixmap( size()-1, pm );
}


void uiListBox::addItem( const char* text, const Color& col )
{
    ioPixmap pm( 64, 64); pm.fill( col );
    addItem( text, pm );
}


void uiListBox::addItems( const char** textList ) 
{
    int curidx = currentItem();
    const char* pt_cur = *textList;
    while ( pt_cur )
        addItem( pt_cur++ );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::addItems( const BufferStringSet& strs )
{
    int curidx = currentItem();
    for ( int idx=0; idx<strs.size(); idx++ )
	body_->addItem( QString( strs.get(idx) ) );
    setCurrentItem( curidx < 0 ? 0 : curidx );
}


void uiListBox::insertItem( const char* text, int index, bool embed )
{
    QString qs;
    createQString( qs, text, embed );
    if ( index<0 )
	body_->addItem( qs );
    else
	body_->insertItem( index, qs );
}


void uiListBox::insertItem( const char* text, const ioPixmap& pm, int index )
{
    if ( index < 0 )
	addItem( text, pm );
    else
    {
	insertItem( text, index, false );
	setPixmap( index, pm );
    }
}


void uiListBox::insertItem( const char* text, const Color& col, int index )
{
    ioPixmap pm( 64, 64 ); pm.fill( col );
    insertItem( text, pm, index );
}


void uiListBox::setPixmap( int index, const Color& col )
{
    if ( index<0 || index>=size() || !body_->item(index) )
	return;

    QSize sz = body_->iconSize();
    ioPixmap pm( sz.width(), sz.height() ); pm.fill( col );
    setPixmap( index, pm );
}


void uiListBox::setPixmap( int index, const ioPixmap& pm )
{
    if ( index<0 || index>=size() || 
	 !body_->item(index) || !pm.qpixmap() ) return;

    body_->item(index)->setIcon( *pm.qpixmap() );
}


ioPixmap uiListBox::pixmap( int index ) const
{
    if ( index<0 || index>=size() || !body_->item(index) )
	return ioPixmap();
    QIcon qicon = body_->item(index)->icon();
    return ioPixmap( qicon.pixmap(body_->iconSize()) );
}


void uiListBox::empty()
{
    body_->QListWidget::clear();
}


void uiListBox::clear()
{
    body_->clearSelection();
}


void uiListBox::sort( bool asc )
{
    if ( !asc )
	pErrMsg("Descending sort not possible");
    body_->setSortingEnabled( true );
}


void uiListBox::removeItem( int idx )
{
    delete body_->takeItem( idx );
}


int uiListBox::nextSelected( int prev ) const
{
    if ( prev<0 ) prev = -1;
    const int sz = size();
    for ( int idx=prev+1; idx<sz; idx++ )
    {
	if ( isSelected(idx) )
	    return idx;
    }

    return -1;
}


bool uiListBox::isPresent( const char* txt ) const
{
    const int sz = size();
    for ( int idx=0; idx<sz; idx++ )
    {
#ifdef USEQT3
	BufferString itmtxt( body_->text(idx) );
#else
	BufferString itmtxt( body_->item(idx)->text().toAscii().constData() );
#endif
	char* ptr = itmtxt.buf();
	skipLeadingBlanks( ptr );
	if ( !strcmp(txt,ptr) ) return true;
    }
    return false;
}


const char* uiListBox::textOfItem( int idx, bool disembed ) const
{
    if ( !validIndex(idx) )
	return "";

    rettxt = (const char*)body_->item(idx)->text().toAscii();
    if ( !disembed || rettxt[0] != '[' )
	return rettxt;

    const int sz = rettxt.size();
    if ( rettxt[sz-1] != ']' )
	return rettxt;

    rettxt[sz-1] = '\0';
    return ((const char*)rettxt) + 1;
}


bool uiListBox::isEmbedded( int idx ) const
{
    rettxt = (const char*)body_->item(idx)->text().toAscii();
    return rettxt[0] == '[' && rettxt[rettxt.size()-1] == ']';
}


int uiListBox::currentItem() const
{
    QListWidgetItem* itm1 = body_->currentItem();
    QListWidgetItem* itm2 = body_->item( body_->currentRow() );
    return body_->currentRow();
}


void uiListBox::setCurrentItem( const char* txt )
{
    if ( !txt ) return;

    const int sz = body_->count();
    for ( int idx=0; idx<sz; idx++ )
    {
	const char* ptr = textOfItem( idx );
	skipLeadingBlanks(ptr);
	if ( !strcmp(ptr,txt) )
	    { setCurrentItem( idx ); return; }
    }
}


void uiListBox::setCurrentItem( int idx )
{
    if ( !validIndex(idx) )
	return;

    body_->setCurrentRow( idx );
    if ( body_->selectionMode() != mExtended )
	setSelected( idx );
}


void uiListBox::setItemText( int idx, const char* txt )
{
    if ( validIndex(idx) )
	body_->item(idx)->setText( QString(txt) );
}


void uiListBox::getSelectedItems( BufferStringSet& items )
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isSelected(idx) ) items.add( textOfItem(idx) );
}


void uiListBox::getSelectedItems( TypeSet<int>& items )
{
    for ( int idx=0; idx<this->size(); idx++ )
	if ( isSelected(idx) ) items += idx;
}


void uiListBox::setFieldWidth( int fw )
{
    body_->fieldwidth_ = fw;
}


int uiListBox::optimumFieldWidth( int minwdth, int maxwdth ) const
{
    const int sz = size();
    int len = minwdth;
    for ( int idx=0; idx<sz; idx++ )
    {
	int itlen = strlen( textOfItem(idx) );
	if ( itlen >= maxwdth )
	    { len = maxwdth; break; }
	else if ( itlen > len )
	    len = itlen;
    }
    return len + 1;
}


// -------------- uiLabeledListBox ----------------
uiLabeledListBox::uiLabeledListBox( uiParent* p, const char* txt, bool multisel,
				    uiLabeledListBox::LblPos pos )
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, txt, multisel );
    mkRest( txt, pos );
}


uiLabeledListBox::uiLabeledListBox( uiParent* p, const BufferStringSet& s,
				    const char* txt,
				    bool multisel, uiLabeledListBox::LblPos pos)
	: uiGroup(p,"Labeled listbox")
{
    lb = new uiListBox( this, s, txt, multisel );
    mkRest( txt, pos );
}


void uiLabeledListBox::setLabelText( const char* txt, int nr )
{
    if ( nr >= lbls.size() ) return;
    lbls[nr]->setText( txt );
}


const char* uiLabeledListBox::labelText( int nr ) const
{
    if ( nr >= lbls.size() ) return "";
    return lbls[nr]->text();
}


void uiLabeledListBox::mkRest( const char* txt, uiLabeledListBox::LblPos pos )
{
    setHAlignObj( lb );

    BufferStringSet txts;
    BufferString s( txt );
    char* ptr = s.buf();
    if( !ptr || !*ptr ) return;
    while ( 1 )
    {
	char* nlptr = strchr( ptr, '\n' );
	if ( nlptr ) *nlptr = '\0';
	txts += new BufferString( ptr );
	if ( !nlptr ) break;

	ptr = nlptr + 1;
    }
    if ( txts.size() < 1 ) return;

    bool last1st = pos > RightTop && pos < BelowLeft;
    ptr = last1st ? txts[txts.size()-1]->buf() : txts[0]->buf();

    uiLabel* labl = new uiLabel( this, ptr );
    lbls += labl;
    constraintType lblct = alignedBelow;
    switch ( pos )
    {
    case LeftTop:
	lb->attach( rightOf, labl );		lblct = rightAlignedBelow;
    break;
    case RightTop:
	labl->attach( rightOf, lb );		lblct = alignedBelow;
    break;
    case AboveLeft:
	lb->attach( alignedBelow, labl );	lblct = alignedAbove;
    break;
    case AboveMid:
	lb->attach( centeredBelow, labl );	lblct = centeredAbove;
    break;
    case AboveRight:
	lb->attach( rightAlignedBelow, labl );	lblct = rightAlignedAbove;
    break;
    case BelowLeft:
	labl->attach( alignedBelow, lb );	lblct = alignedBelow;
    break;
    case BelowMid:
	labl->attach( centeredBelow, lb );	lblct = centeredBelow;
    break;
    case BelowRight:
	labl->attach( rightAlignedBelow, lb );	lblct = rightAlignedBelow;
    break;
    }

    int nrleft = txts.size() - 1;
    while ( nrleft )
    {
	uiLabel* cur = new uiLabel( this, (last1st
			? txts[nrleft-1] : txts[txts.size()-nrleft])->buf() );
	cur->attach( lblct, labl );
	lbls += cur;
	labl = cur;
	nrleft--;
    }

    deepErase( txts );
}
