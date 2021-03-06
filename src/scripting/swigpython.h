/*!
	Copyright (c) 2006-2007, Matevž Jekovec, Canorus development team
	All Rights Reserved. See AUTHORS for a complete list of authors.

	Licensed under the GNU GENERAL PUBLIC LICENSE. See COPYING for details.
*/

#ifdef USE_PYTHON
#ifndef SWIGPYTHON_H_
#define SWIGPYTHON_H_

#include <Python.h>

#include <QList>
#include <QString>

class CASwigPython {
public:
    enum CAClassType {
        // Qt objects
        String,

        // Canorus objects
        Document,
        Sheet,
        Resource,
        Context,
        Voice,
        MusElement,
        PlayableLength,

        // Console
        PyConsoleInterface,

        // Plugins
        Plugin
    };

    static void init();
    static PyObject* callFunction(QString fileName, QString function, QList<PyObject*> args, bool autoReload = false);
    static void* callPycli(void*);
    static PyObject* toPythonObject(void* object, CAClassType type); // defined in scripting/canoruspython.i

    static PyThreadState *mainThreadState, *pycliThreadState;
};

#endif /*SWIGPYTHON_H_*/
#endif
