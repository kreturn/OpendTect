#ifndef uibutton_h
#define uibutton_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          21/01/2000
 RCS:           $Id: uibutton.h,v 1.19 2007-05-09 16:52:40 cvsjaap Exp $
________________________________________________________________________

-*/

#include "uiobj.h"

class uiButtonGroup;
class uiButtonBody;

class uiPushButtonBody;
class uiRadioButtonBody;
class uiCheckBoxBody;
class uiToolButtonBody;
class ioPixmap;
class QEvent;


//!\brief Button Abstract Base class
class uiButton : public uiObject
{
public:
			uiButton(uiParent*,const char*,const CallBack*,
				 uiObjectBody&);
    virtual		~uiButton()		{}

    virtual void	setText(const char*);
    const char*		text();

    void		activate();
    bool		isActive() const;

    Notifier<uiButton>	activated;
};


/*!\brief Push button. By default, assumes immediate action, not a dialog
  when pushed. The button text will in that case get an added " ..." to the
  text. In principle, it could also get another appearance.
  */

class uiPushButton : public uiButton
{
public:
				uiPushButton(uiParent*,const char* nm,
					     bool immediate);
				uiPushButton(uiParent*,const char* nm,
					     const CallBack&,
					     bool immediate); 
				uiPushButton(uiParent*,const char* nm,
					     const ioPixmap&,
					     bool immediate);
				uiPushButton(uiParent*,const char* nm,
					     const ioPixmap&,const CallBack&,
					     bool immediate);
				~uiPushButton();

    void			setDefault(bool yn=true);
    void			setPixmap(const ioPixmap&);
    ioPixmap*			getPixmap()
    				{ return const_cast<ioPixmap*>(pixmap_); }
    const ioPixmap*		getPixmap() const	{ return pixmap_; }

private:

    uiPushButtonBody*		body_;
    uiPushButtonBody&		mkbody(uiParent*,const ioPixmap*,const char*,
	    				bool);

    const ioPixmap*		pixmap_;
};


class uiRadioButton : public uiButton
{                        
public:
				uiRadioButton(uiParent*,const char*);
				uiRadioButton(uiParent*,const char*,
					      const CallBack&);

    bool			isChecked() const;
    virtual void		setChecked(bool yn=true);

private:

    uiRadioButtonBody*		body_;
    uiRadioButtonBody&		mkbody(uiParent*,const char*);

};


class uiCheckBox: public uiButton
{
public:

				uiCheckBox(uiParent*,const char*);
				uiCheckBox(uiParent*,const char*,
					   const CallBack&);

    bool			isChecked() const;
    void			setChecked(bool yn=true);

    virtual void		setText(const char*);

private:

    uiCheckBoxBody*		body_;
    uiCheckBoxBody&		mkbody(uiParent*,const char*);

};


class uiToolButton : public uiButton
{
public:
				uiToolButton(uiParent*,const char*);
				uiToolButton(uiParent*,const char*,
					     const CallBack&);
				uiToolButton(uiParent*,const char*,
					     const ioPixmap&);
				uiToolButton(uiParent*,const char*,
					     const ioPixmap&,const CallBack&);

    bool			isOn();
    void			setOn(bool yn=true);

    void			setToggleButton(bool yn=true);
    bool			isToggleButton();

    void			setPixmap(const ioPixmap&);

private:

    uiToolButtonBody*		body_;
    uiToolButtonBody&		mkbody(uiParent*,const ioPixmap*, const char*); 

};


//! Button Abstract Base class
class uiButtonBody
{
    friend class        i_ButMessenger;

public:
			uiButtonBody()				{}
    virtual		~uiButtonBody()				{}

    virtual void	activate()				=0;
    virtual bool	isActive() const			=0;

    //! Button signals emitted by Qt.
    enum notifyTp       { clicked, pressed, released, toggled };
    
protected:
    virtual bool	handleEvent(const QEvent*)		=0;

    //! Handler called from Qt.
    virtual void        notifyHandler(notifyTp)			=0;
};




#endif
