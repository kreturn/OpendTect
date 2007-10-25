/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : Sep 2003
-*/


static const char* rcsID = "$Id: attribdesc.cc,v 1.59 2007-10-25 04:22:06 cvsraman Exp $";

#include "attribdesc.h"

#include "attribparam.h"
#include "attribstorprovider.h"
#include "seistrctr.h"
#include "ioman.h"
#include "ioobj.h"

namespace Attrib
{

bool InputSpec::operator==(const InputSpec& b) const
{
    if ( desc!=b.desc || enabled!=b.enabled || issteering!=b.issteering )
	return false;

    for ( int idx=0; idx<forbiddenDts.size(); idx++ )
    {
	if ( b.forbiddenDts.indexOf(forbiddenDts[idx])==-1 )
	    return false;
    }

    for ( int idx=0; idx<b.forbiddenDts.size(); idx++ )
    {
	if ( forbiddenDts.indexOf(b.forbiddenDts[idx])==-1 )
	    return false;
    }

    return true;
}


const char* Desc::sKeyInlDipComp() { return "inl_dip"; }
const char* Desc::sKeyCrlDipComp() { return "crl_dip"; }


Desc::Desc( const char* attribname, DescStatusUpdater updater )
    : descset_( 0 )
    , attribname_( attribname )
    , statusupdater_( updater )
    , issteering_( false )
    , seloutput_( 0 )
    , hidden_( false )
    , is2d_( false )
    , is2dset_( false )
    , needprovinit_( false )
    , is2ddetected_( false )
{
    if ( strchr( attribname, ' ' ) )
	pErrMsg("Space character is not permitted in attribute names");

    inputs_.allowNull(true);
}


Desc::Desc( const Desc& a )
    : descset_( a.descset_ )
    , attribname_( a.attribname_ )
    , statusupdater_( a.statusupdater_ )
    , hidden_( a.hidden_ )
    , issteering_( a.issteering_ )
    , seloutput_( a.seloutput_ )
    , userref_( a.userref_ )
    , needprovinit_( a.needprovinit_ )
    , is2d_(a.is2d_)
    , is2dset_(a.is2dset_)
    , is2ddetected_(a.is2ddetected_)
{
    inputs_.allowNull(true);

    for ( int idx=0; idx<a.params_.size(); idx++ )
	addParam( a.params_[idx]->clone() );

    for ( int idx=0; idx<a.inputs_.size(); idx++ )
    {
	addInput( a.inputSpec(idx) );
	if ( a.inputs_[idx] )
	    setInput( idx, a.inputs_[idx] );
    }

    for ( int idx=0; idx<a.nrOutputs(); idx++ )
	addOutputDataType( a.outputtypes_[idx] );
}


Desc::~Desc()
{
    deepErase( params_ );
    deepUnRef( inputs_ );
}


const char* Desc::attribName() const		{ return attribname_; }

void Desc::setDescSet( DescSet* nds )
{
    descset_ = nds;
    if ( !nds ) return;
    is2dset_ = nds->is2D();
    if ( isStored() ) return;
    is2d_ = nds->is2D();
}

DescSet* Desc::descSet() const			{ return descset_; }

DescID Desc::id() const
{ return descset_ ? descset_->getID(*this) : DescID(-1,true); }


bool Desc::getDefStr( BufferString& res ) const
{
    res = attribName();
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx]->isEnabled() ) continue;
	res += " ";

	params_[idx]->fillDefStr( res );
    }

    if ( seloutput_!=-1 )
    {
	res += " output=";
	res += seloutput_;
    }

    return true;
}


bool Desc::parseDefStr( const char* defstr )
{
    BufferString defstrnm;
    if ( !getAttribName(defstr,defstrnm) || defstrnm!=attribname_ )
	return false;

    BufferString outputstr;
    bool res = getParamString( defstr, "output", outputstr );
    selectOutput( res ? atoi(outputstr.buf()) : 0 );
  
    BufferStringSet keys, vals;
    getKeysVals( defstr, keys, vals );

    is2ddetected_ = false;
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	bool found = false;
	if ( !params_[idx]->isGroup() )
	{
	    BufferString paramval;
	    for ( int idy=0; idy<keys.size(); idy++ )
	    {
		if ( keys.get(idy) == params_[idx]->getKey() )
		{
		    paramval = vals.get(idy);
		    found = true;
		    break;
		}
	    }

	    if ( !found )
	    {
		if ( params_[idx]->isRequired() )
		     continue;
		else
		    paramval = params_[idx]->getDefaultValue();
	    }

	    if ( !params_[idx]->setCompositeValue(paramval) )
		return false;
	}
	else
	{
	    BufferStringSet paramvalset;
	    int valueidx = 0;
	    for ( int idy=0; idy<keys.size(); idy++ )
	    {
		BufferString keystring = params_[idx]->getKey();
		keystring += valueidx;
		if ( keys.get(idy) == keystring )
		{
		    paramvalset.add( vals.get(idy).buf() );
		    found =true;
		    valueidx++;
		}
	    }
	    if ( !found )
	    {
		if ( params_[idx]->isRequired() )
		    continue;
	    }

	    if ( !params_[idx]->setValues(paramvalset) )
		return false;
	}
    }

    if ( statusupdater_ )
     statusupdater_(*this);

    if ( errmsg_.size() )
	return false;

    for ( int idx=0; idx<params_.size(); idx++ )
    {
         if ( !params_[idx]->isOK() )
	     return false;
    }
     
    return true;
}


const char* Desc::userRef() const		{ return userref_; }
void Desc::setUserRef( const char* str )	{ userref_ = str; }
int Desc::nrOutputs() const			{ return outputtypes_.size(); }
void Desc::selectOutput( int outp )		{ seloutput_ = outp; }
int Desc::selectedOutput() const		{ return seloutput_; }
int Desc::nrInputs() const			{ return inputs_.size(); }


void Desc::getDependencies(TypeSet<Attrib::DescID>& deps) const
{
    for ( int idx=nrInputs()-1; idx>=0; idx-- )
    {
	if ( !inputs_[idx] )
	    continue;

	if ( deps.indexOf(inputs_[idx]->id())!=-1 )
	    continue;

	deps += inputs_[idx]->id();
	inputs_[idx]->getDependencies(deps);
    }
}


Seis::DataType Desc::dataType( int target ) const
{
    if ( seloutput_==-1 || outputtypes_.isEmpty() )
	return Seis::UnknowData;

    int outidx = target==-1 ? seloutput_ : target;
    if ( outidx >= outputtypes_.size() ) outidx = 0;

    const int link = outputtypelinks_[outidx];
    if ( link == -1 )
	return outputtypes_[outidx];

    return link < inputs_.size() && inputs_[link] ? inputs_[link]->dataType()
						: Seis::UnknowData;
}


bool Desc::setInput( int inp, const Desc* nd )
{
    return setInput_( inp, const_cast<Desc*>( nd ) );
}


bool Desc::setInput_( int input, Desc* nd )
{
    if ( nd && (inputspecs_[input].forbiddenDts.indexOf(nd->dataType())!=-1 ||
		inputspecs_[input].issteering!=nd->isSteering()) )
	return false;

    if ( inputs_[input] ) inputs_[input]->unRef();
    inputs_.replace( input, nd );
    if ( inputs_[input] ) inputs_[input]->ref();

    return true;
}


const Desc* Desc::getInput( int input ) const
{ return input>=0 && input<inputs_.size() ? inputs_[input] : 0; }

Desc* Desc::getInput( int input )
{ return input>=0 && input<inputs_.size() ? inputs_[input] : 0; }


//keep it for backward compatibility with 2.4
bool Desc::is2D() const
{
    if ( is2dset_ || is2ddetected_ )
	return is2d_;
    
    const ValParam* keypar = getValParam( StorageProvider::keyStr() );
    if ( !keypar ) return false;

    MultiID key = keypar->getStringValue();
    PtrMan<IOObj> ioobj = IOM().get( key );
    is2d_ = ioobj && SeisTrcTranslator::is2D( *ioobj );
    is2ddetected_ = true;
    return is2d_;
}

#define mErrRet(msg) \
    const_cast<Desc*>(this)->errmsg_ = msg;\
    return Error;\



Desc::SatisfyLevel Desc::isSatisfied() const
{
    if ( seloutput_==-1 )
	    // || seloutput_>nrOutputs()  )
	    //TODO NN descs return only one output. Needs solution!
    {
	BufferString msg = "Selected output is not correct";
	mErrRet(msg)
    }

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !params_[idx]->isOK() )
	{
	    BufferString msg = "Parameter '"; msg += params_[idx]->getKey();
	    msg += "' is not correct";
	    mErrRet(msg)
	}
    }

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputspecs_[idx].enabled ) continue;
	if ( !inputs_[idx] )
	{
	    BufferString msg = "'"; msg += inputspecs_[idx].getDesc();
	    msg += "' is not correct";
	    mErrRet(msg)
	}
	else
	{
	    TypeSet<Attrib::DescID> deps( 1, inputs_[idx]->id() );
	    inputs_[idx]->getDependencies( deps );

	    if ( deps.indexOf( id() )!=-1 )
	    {
		BufferString msg = "'"; msg += inputspecs_[idx].getDesc();
		msg += "' is dependent on itself";
		mErrRet(msg);
	    }
	}
    }

    return AllOk;
}


const BufferString& Desc::errMsg() const
{ return errmsg_; }


bool Desc::isIdenticalTo( const Desc& desc, bool cmpoutput ) const
{
    if ( this==&desc ) return true;

    if ( params_.size() != desc.params_.size()
      || inputs_.size() != desc.inputs_.size() )
	return false;

    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( *params_[idx]!=*desc.params_[idx] )
	    return false;
    }

    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( inputs_[idx]==desc.inputs_[idx] ) continue;

	if ( !inputs_[idx] && !desc.inputs_[idx] ) continue;

	if ( !desc.inputs_[idx] || 
	     !inputs_[idx]->isIdenticalTo(*desc.inputs_[idx], true) )
	    return false;
    }

    return cmpoutput ? seloutput_==desc.seloutput_ : true;
}


DescID Desc::inputId( int idx ) const
{
    const bool valididx = idx >= 0 && idx < inputs_.size();
    return valididx && inputs_[idx] ? inputs_[idx]->id() : DescID(-1,true);
}


void Desc::addParam( Param* param )
{
    params_ += param;
    is2ddetected_ = false;
}


const Param* Desc::getParam( const char* key ) const
{ return const_cast<Desc*>(this)->getParam(key); }


Param* Desc::getParam( const char* key )
{
    return findParam(key);
}


const ValParam* Desc::getValParam( const char* key ) const
{
    mDynamicCastGet(const ValParam*,valpar,getParam(key))
    return valpar;
}


ValParam* Desc::getValParam( const char* key )
{
    mDynamicCastGet(ValParam*,valpar,getParam(key))
    return valpar;
}


void Desc::setParamEnabled( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( !param ) return;

    param->setEnabled(yn);
}


bool Desc::isParamEnabled( const char* key ) const
{
    const Param* param = getParam( key );
    if ( !param ) return false;

    return param->isEnabled();
}
	


void Desc::setParamRequired( const char* key, bool yn )
{
    Param* param = findParam( key );
    if ( !param ) return;

    param->setRequired(yn);
}


bool Desc::isParamRequired( const char* key ) const
{
    const Param* param = getParam( key );
    if ( !param ) return false;

    return param->isRequired();
}


void Desc::updateParams()
{
    if ( statusupdater_ ) statusupdater_(*this);

    for ( int idx=0; idx<nrInputs(); idx++ )
    {
	Desc* dsc = getInput(idx); 
	if ( dsc && dsc->isHidden() )
	    dsc->updateParams();
    }
}


void Desc::addInput( const InputSpec& is )
{
    inputspecs_ += is;
    inputs_ += 0;
}


bool Desc::removeInput( int idx )
{
    inputspecs_.remove(idx);
    inputs_.remove(idx);
    return true;
}



InputSpec& Desc::inputSpec( int input ) { return inputspecs_[input]; }


const InputSpec& Desc::inputSpec( int input ) const
{ return const_cast<Desc*>(this)->inputSpec(input); }


void Desc::removeOutputs()
{
    outputtypes_.erase();
    outputtypelinks_.erase();
}


void Desc::setNrOutputs( Seis::DataType dt, int nroutp )
{
    removeOutputs();
    for ( int idx=0; idx<nroutp; idx++ )
	addOutputDataType( dt );
}
	

void Desc::addOutputDataType( Seis::DataType dt )
{ outputtypes_+=dt; outputtypelinks_+=-1; }


void Desc::addOutputDataTypeSameAs( int input )
{
    outputtypes_ += Seis::UnknowData;
    outputtypelinks_ += input;
}


void Desc::changeOutputDataType( int input, Seis::DataType ndt )
{
    if ( outputtypes_.size()<=input || input<0 ) return;
    outputtypes_[input] = ndt;
}
	

bool Desc::getAttribName( const char* defstr_, BufferString& res )
{
    ArrPtrMan<char> defstr = new char [strlen(defstr_)+1];
    strcpy( defstr, defstr_ );

    int start = 0;
    while ( start<strlen(defstr) && isspace(defstr[start]) ) start++;

    if ( start>=strlen(defstr) ) return false;

    int stop = start+1;
    while ( stop<strlen(defstr) && !isspace(defstr[stop]) ) stop++;
    defstr[stop] = 0;

    res = &defstr[start];
    return true;
}


bool Desc::getParamString( const char* defstr, const char* key,
			   BufferString& res )
{
    if ( !defstr || !key )
	return 0;

    const int inpsz = strlen(defstr);
    const int keysz = strlen(key);
    bool inquotes = false;
    for ( int idx=0; idx<inpsz; idx++ )
    {
	if ( !inquotes && defstr[idx] == '=' )
	{
	    int firstpos = idx - 1;

	    while ( isspace(defstr[firstpos]) && firstpos >= 0 ) firstpos --;
	    if ( firstpos < 0 ) continue;

	    int lastpos = firstpos;

	    while ( !isspace(defstr[firstpos]) && firstpos >= 0 ) firstpos --;
	    firstpos++;

	    if ( lastpos - firstpos + 1 == keysz )
	    {
		if ( !strncmp( &defstr[firstpos], key, keysz ) )
		{
		    firstpos = idx + 1;
		    while ( isspace(defstr[firstpos]) && firstpos < inpsz )
			firstpos ++;
		    if ( firstpos == inpsz ) continue;

		    bool hasquotes = false;
		    if (defstr[firstpos] == '"')
		    {
			hasquotes = true;
			if (firstpos == inpsz - 1)
			    continue;
			firstpos++;
		    }
		    lastpos = firstpos;

		    while (( (hasquotes && defstr[lastpos] != '"')
			    || (!hasquotes && !isspace(defstr[lastpos])) )
			    &&  lastpos < inpsz)
			lastpos ++;

		    lastpos --;

		    ArrPtrMan<char> tmpres = new char [lastpos-firstpos+2];
		    strncpy( tmpres, &defstr[firstpos], lastpos-firstpos+1 );
		    tmpres[lastpos-firstpos+1] = 0;

		    res = tmpres;
		    return true;
		}
	    }
	}
	else if ( defstr[idx] == '"' )
	    inquotes = !inquotes;
    }

    return false;
}


Param* Desc::findParam( const char* key )
{
    for ( int idx=0; idx<params_.size(); idx++ )
    {
	if ( !strcmp(params_[idx]->getKey(),key) )
	    return params_[idx];
    }

    return 0;
}


bool Desc::isStored() const
{
    return !strcmp( attribName(), StorageProvider::attribName() );
}


bool Desc::getMultiID( MultiID& mid ) const
{
    if ( !isStored() ) return false;

    const ValParam* keypar = getValParam( StorageProvider::keyStr() );
    mid = keypar->getStringValue();
    return true;
}


bool Desc::isIdentifiedBy( const char* str ) const
{
    if ( userref_==str )
	return true;

    if ( isStored() )
    {
	LineKey lk( str );
	BufferString defstr; getDefStr(defstr);
	BufferString parstr;
	if ( !getParamString(defstr,params_[0]->getKey(),parstr) )
	    return false;

	const bool is2ddefstr = 
	    parstr == lk.lineName() && !strcmp(lk.attrName().buf(),"Seis");
	if ( parstr == str || is2ddefstr )
	    return true;
    }

    return false;
}


void Desc::getKeysVals( const char* defstr, BufferStringSet& keys,
			BufferStringSet& vals )
{
    const int len = strlen(defstr);
    int spacepos = 0;
    int equalpos = 0;
    for ( int idx=0; idx<len; idx++ )
    {
	if ( defstr[idx] != '=')
	    continue;

	equalpos = idx;
	spacepos = idx-1;
	while ( spacepos>=0 && isspace(defstr[spacepos]) ) spacepos--;
	if ( spacepos < 0 ) continue;
	int lastpos = spacepos;

	while ( !isspace(defstr[spacepos]) && spacepos >= 0 )
	    spacepos --;

	spacepos++;
	ArrPtrMan<char> tmpkey = new char [lastpos-spacepos+2];
	strncpy( tmpkey, &defstr[spacepos], lastpos-spacepos+1 );
	tmpkey[lastpos-spacepos+1] = 0;
	const char* tmp = tmpkey;
	keys.add(tmp);
	
	spacepos = idx+1;

	if ( defstr[spacepos] == '"' )
	{
	    spacepos++;
	    lastpos = spacepos;
	    while ( spacepos<len && (defstr[spacepos] != '"') ) spacepos++;

	    spacepos--;
	}
	else
	{
	    while ( spacepos<len && isspace(defstr[spacepos]) ) spacepos++;
	    if ( spacepos >= len ) continue;
	    lastpos = spacepos;

	    while ( !isspace(defstr[spacepos]) && spacepos<len ) spacepos++;
	    spacepos--;
	}

	ArrPtrMan<char> tmpval = new char [spacepos-lastpos+2];
	strncpy( tmpval, &defstr[lastpos], spacepos-lastpos+1 );
	tmpval[spacepos-lastpos+1] = 0;
	tmp = tmpval;
	vals.add(tmp);
	idx = spacepos;
    }
}


void Desc::changeStoredID( const char* newid )
{
    if ( !isStored() ) return;

    ValParam* keypar = getValParam( StorageProvider::keyStr() );
    keypar->setValue( newid );
}


void Desc::set2D( bool is2dstudy )
{
    is2dset_ = true;
    is2d_ = is2dstudy;
}


Desc* Desc::getStoredInput() const
{
    Desc* desc = 0;
    for ( int idx=0; idx<inputs_.size(); idx++ )
    {
	if ( !inputs_[idx] ) continue;

	if ( inputs_[idx]->isStored() )
	    return const_cast<Desc*>( inputs_[idx] );
	else
	    desc = inputs_[idx]->getStoredInput();
    }

    return desc;
}


}; // namespace Attrib
