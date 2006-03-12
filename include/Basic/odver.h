#ifndef odver_h
#define odver_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	A.H.Bril
 Date:		Mar 2006
 RCS:		$Id: odver.h,v 1.1 2006-03-12 13:39:09 cvsbert Exp $
________________________________________________________________________

-*/


#define mODMajorVersion		2
#define mODMinorVersion		3


/*!\mainpage Basic utilities
  \section Introduction Introduction

  This module handles all things that are so basic to all other modules that
  they can be seen as a layer of common services to the entire system. One of
  the tasks is to provide platform-independence of file, date&time, threads,
  and more.

  The difference with the 'General' module was, traditionally, that Basic
  utilities could in principle be insteresting outside OpendTect. This
  distinction is not enforced (see e.g. the survey info class). We place
  things in Basic that feel more basic than General. Note that General depends
  on Basic, not vice versa.

  You'll find that many of these tools can be found in other packages in some
  form or another. But, the problems of management of dependencies are usually
  much bigger than having to maintain a bunch of lean-and-mean specially made
  objects that do selected things in a way that fits our system.


  \section Content Content

  We'll just name a few groups of services. There are many more isolated
  useful objects, defines, functions and so forth.

<ul>
 <li>Sets/Lists
  <ul>
   <li>sets.h : 'The' classes for sets of objects and pointers to objects
   <li>sortedlist.h and sortedtable.h : sets that are sorted during build
   <li>toplist.h : holds a "top N" list
   <li>iopar.h : IOPar is a keyword-value lookup list that is used as 'generic'
       parameter list throughout OpendTect.
  </ul>
 <li>Strings
  <ul>
   <li>string2.h : things not in the standard <string.h>
   <li>bufstring.h and bufstringset.h : Variable length strings commonly used in
       OpendTect with a guaranteed minimum buffer size. That makes them ideal
       as bridge to C strings.
   <li>compoundkey.h and multiid.h : dot-separated keys.
   <li>fixstring.h: fixed length strings but with tools like '+=' .
  </ul>
 <li>System-wide service objects
  <ul>
   <li>msgh.h, errh.h and debug.h : Message pushing without user interface
   <li>settings.h : access to persistent user specific settings
   <li>survinfo.h : access to the survey setup (names, positions, ranges)
  </ul>
 <li>Positions (coordinates, inlines/crosslines=BinIDs)
  <ul>
   <li>position.h and posgeom.h : basic map position tools
   <li>binidvalset.h : A set of binids optionally accompanied by values.
       Searching a specific BinID is fast.
  </ul>
 <li>File and stream handling
  <ul>
   <li>filegen.h : basic file tools like existence, path operations, copy, etc.
   <li>ascstream.h : read and write of the typical OpendTect standard Ascii data
   <li>dirlist.h : list contents of a directory
   <li>strmprov.h and strmdata.h : access files, pipes, or devices for read
       or write and make sure they are closed correctly.
  </ul>
 <li>CallBacks
  <ul>
   <li>callback.h : our (simple but powerful) event system
  </ul>
 <li>Template floating-point algorithms
  <ul>
   <li>simpnumer.h, sorting.h, periodicvalue.h, genericnumer.h, extremefinder.h
  </ul>
</ul>

*/

#endif
