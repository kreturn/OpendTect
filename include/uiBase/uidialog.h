#ifndef uidialog_h
#define uidialog_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/08/2000
 RCS:           $Id: uidialog.h,v 1.42 2006-05-04 20:43:32 cvsnanne Exp $
________________________________________________________________________

-*/

#include "uimainwin.h"
#include "bufstring.h"

#include "errh.h"
class uiGroup;
class uiButton;

/*!\brief Stand-alone dialog window with optional 'Ok', 'Cancel' and
'Save defaults' button.

It is meant to be the base class for 'normal' dialog windows.

*/


class uiDialog : public uiMainWin
{ 	
    // impl: uimainwin.cc
    friend class	uiDialogBody;

public:

    /*!\brief description of properties of dialog.

	see general.h for background on Setup classes.
     */

    class Setup
    {
    public:

			Setup( const char* window_title,
			       const char* dialog_title =0,
			       const char* help_id =0 )
			: wintitle_(window_title)
			, dlgtitle_(dialog_title ? dialog_title : window_title)
			, helpid_(help_id), savetext_("Save defaults")
			, oktext_("&Ok"), canceltext_("&Cancel")
			, modal_(true) // if no parent given, always non-modal
			, savebutton_(false), savebutispush_(false)
			, separator_(true), menubar_(false), nrstatusflds_(0)
			, mainwidgcentered_(false), savechecked_(false)
			, fixedsize_(false)
			{}

	mDefSetupMemb(BufferString,wintitle)
	mDefSetupMemb(BufferString,dlgtitle)
	mDefSetupMemb(BufferString,helpid)
	mDefSetupMemb(BufferString,savetext)
	mDefSetupMemb(BufferString,oktext)
	mDefSetupMemb(BufferString,canceltext)
	mDefSetupMemb(bool,modal)
	mDefSetupMemb(bool,savebutton)
	mDefSetupMemb(bool,savebutispush)
	mDefSetupMemb(bool,separator)
	mDefSetupMemb(bool,savechecked)
	mDefSetupMemb(bool,menubar)
	mDefSetupMemb(bool,mainwidgcentered)
	mDefSetupMemb(bool,fixedsize)
	mDefSetupMemb(int,nrstatusflds)
	    //! nrstatusflds == -1: Make a statusbar, but don't add msg fields.

    };

    enum                Button { OK, SAVE, CANCEL, HELP };

			uiDialog(uiParent*,const Setup&);
    const Setup&	setup() const;

    int			go(); 

    void		reject( CallBacker* cb =0);
    void		accept( CallBacker* cb =0);
    void		done(int ret=0);
    			//!< 0=Cancel, 1=OK, other=user defined

    void		setHSpacing( int ); 
    void		setVSpacing( int ); 
    void		setBorder( int ); 

    void		setCaption( const char* txt );
    void		setModal(bool yn);
    bool		isModal() const;

    uiButton*		button( Button but );
    void		setButtonText( Button but, const char* txt )
			{
			    switch ( but )
			    {
			    case OK	: setOkText( txt ); break;
			    case CANCEL	: setCancelText( txt ); break;
			    case SAVE	: enableSaveButton( txt ); break;
			    case HELP	: pErrMsg("can't set txt on help but");
			    }
			}

    enum CtrlStyle	{ DoAndLeave, DoAndStay, LeaveOnly };
			//! On construction, it's (of course) DoAndLeave
    void		setCtrlStyle(CtrlStyle);
			//! OK button disabled when set to empty
    void		setOkText( const char* txt );
			//! cancel button disabled when set to empty
    void		setCancelText( const char* txt );
			//! Save button enabled when set to non-empty
    void		enableSaveButton( const char* txt="Save defaults" );
			//! 0: cancel; 1: OK
    int			uiResult() const;
    			

    void		setButtonSensitive(Button,bool);
    void		setSaveButtonChecked(bool);
    void		setTitleText(const char* txt);
    bool		saveButtonChecked() const;

    void		setSeparator(bool yn);
			//!< Separator between central dialog and Ok/Cancel bar?
    bool		separator() const;
    void		setHelpID(const char*);
    const char*		helpID() const;

protected:

    virtual bool        rejectOK(CallBacker*){ return true;}//!< confirm reject 
    virtual bool        acceptOK(CallBacker*){ return true;}//!< confirm accept 
    virtual bool        doneOK(int)	     { return true; } //!< confirm exit 

    bool		cancelpushed_;

};

#endif
