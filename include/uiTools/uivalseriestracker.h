#ifndef uivalseriestracker_h
#define uivalseriestracker_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        K. Tingdahl
 Date:          March 2005
 RCS:           $Id: uivalseriestracker.h,v 1.1 2007-06-25 19:30:09 cvskris Exp $
________________________________________________________________________

-*/

#include "uidlggroup.h"

class EventTracker;
class uiGenInput;

/*!User interface for EventTracker. */


class uiEventTracker : public uiDlgGroup
{
public:
    			uiEventTracker(uiParent*,EventTracker&);
			~uiEventTracker();

    bool			acceptOK();
protected:
    void			selEventType(CallBacker*);
    void			selAmpThresholdType(CallBacker*);
    void			selUseSimilarity(CallBacker*);

    uiGenInput*			evfld_;
    uiGenInput*			srchgatefld_;
    uiGenInput*			thresholdtypefld_;
    uiGenInput*			ampthresholdfld_;
    uiGenInput*			alloweddifffld_;

    uiGenInput*			usesimifld_;
    uiGenInput*			simithresholdfld_;
    uiGenInput*			compwinfld_;

    EventTracker&		tracker_;
};
			       
#endif
