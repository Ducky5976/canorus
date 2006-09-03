/** @file scripting/clef.i
 * 
 * Copyright (c) 2006, Matevž Jekovec, Canorus development team
 * All Rights Reserved. See AUTHORS for a complete list of authors.
 * 
 * Licensed under the GNU GENERAL PUBLIC LICENSE. See COPYING for details.
 */

%{
#include "core/clef.h"
%}

/**
 * Swig implementation of CAClef.
 */
%rename(Clef) CAClef;
class CAClef : public CAMusElement {
	public:
		enum CAClefType {
			Treble,
			Bass,
			Alto,
			Tenor,
			Soprano,
			PercussionHigh,
			PercussionLow
		};

		CAClef(CAClefType type, CAStaff *staff, int time);
		
		/**
		 * This is an overloaded member function provided for convenience.
		 * 
		 * Constructor using the ML arguments.
		 */ 
		CAClef(const char* type, CAStaff *staff, int time); //TODO: Same constructors with above for Swig - changed QString to char*
		
		CAClefType clefType();
		const QString clefTypeML();
		const int c1();
		const int centerPitch();
};