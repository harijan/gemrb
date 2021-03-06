/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef PYTHON_HELPERS_H
#define PYTHON_HELPERS_H

// Python.h needs to be included first.
#include "GUIScript.h"

#include "Callback.h"
#include "Holder.h"
#include "Interface.h"

#include "GUI/Control.h"
#include "GUI/Window.h"

#include "System/Logging.h"

namespace GemRB {

bool CallPython(PyObject*, PyObject* args = NULL);
long CallPythonWithReturn(PyObject*, PyObject* args = NULL);

// could use an adapter pattern to reduce code duplication
// for the 2 callback types

struct PythonCallback : public VoidCallback {
public:
	PythonCallback(PyObject *Function);
	~PythonCallback();
	bool operator()();
private:
	PyObject *Function;
};

template <class T>
struct PythonObjectCallback : public Callback<T*> {
public:
	PythonObjectCallback(PyObject*);
	~PythonObjectCallback();

	bool operator()();
	bool operator()(T*);
private:
	PyObject *Function;
};

typedef PythonObjectCallback<Control> PythonControlCallback;

template <typename T>
class CObject : public Holder<T> {
private:
public:
	operator PyObject* () const
	{
		if (Holder<T>::ptr) {
			Holder<T>::ptr->acquire();
			GUIScript *gs = (GUIScript *) core->GetGUIScriptEngine();
			PyObject *obj = PyCapsule_New(Holder<T>::ptr, &T::ID, PyCapsuleRelease);
			PyCapsule_SetContext(obj, &T::ID);
			//PyObject *obj = PyCapsule_FromVoidPtrAndDesc(,,);
			PyObject *tuple = PyTuple_New(1);
			PyTuple_SET_ITEM(tuple, 0, obj);
			PyObject *ret = gs->ConstructObject(T::ID.description, tuple);
			Py_DECREF(tuple);
			return ret;
		} else {
			Py_INCREF( Py_None );
			return Py_None;
		}
	}
	CObject(PyObject *obj)
	{
		if (obj == Py_None)
			return;
		PyObject *id = PyObject_GetAttrString(obj, "ID");
		if (id)
			obj = id;
		else
			PyErr_Clear();
		if (!PyCapsule_CheckExact(obj) || PyCapsule_GetContext(obj) != const_cast<TypeID*>(&T::ID)) {
			Log(ERROR, "GUIScript", "Bad Capsule extracted.");
			Py_XDECREF(id);
			return;
		}
		Holder<T>::ptr = static_cast<T*>(PyCapsule_GetPointer(obj,"CObject.ptr"));
		Holder<T>::ptr->acquire();
		Py_XDECREF(id);
	}
	CObject(const Holder<T>& ptr)
		: Holder<T>(ptr)
	{
	}
	// This is here because of lookup order issues.
	operator bool () const
	{
		return Holder<T>::ptr;
	}
private:
	static void PyCapsuleRelease(PyObject *obj)
	{
		if (!PyCapsule_IsValid(obj, &T::ID)) {
			Log(ERROR, "GUIScript", "Bad CObject deleted.");
			return;
		}
		reinterpret_cast<T*>(obj)->release();
	}
	static void PyRelease(void *obj, void *desc)
	{
		if (desc != const_cast<TypeID*>(&T::ID)) {
			Log(ERROR, "GUIScript", "Bad CObject deleted.");
			return;
		}
		static_cast<T*>(obj)->release();
	}
};

template <typename T, class Container>
PyObject* MakePyList(const Container &source)
{
	size_t size = source.size();
	PyObject *list = PyList_New(size);
	for (size_t i = 0; i < size; ++i) {
		// SET_ITEM might be preferable to SetItem here, but MSVC6 doesn't like it.
		PyList_SetItem(list, i, CObject<T>(source[i]));
	}
	return list;
}

template <class T>
bool PythonObjectCallback<T>::operator() ()
{
	if (!Function || !Py_IsInitialized()) {
		return false;
	}
	return CallPython(Function);
}


template <class T>
PythonObjectCallback<T>::PythonObjectCallback(PyObject *Function)
	: Function(Function)
{
	if (Function && PyCallable_Check(Function)) {
		Py_INCREF(Function);
	} else {
		Function = NULL;
	}
}

template <class T>
PythonObjectCallback<T>::~PythonObjectCallback()
{
	if (Py_IsInitialized()) {
		Py_XDECREF(Function);
	}
}

template <class T>
bool PythonObjectCallback<T>::operator() (T*) {
	return false;
}

template <>
bool PythonObjectCallback<Control>::operator() (Control *);
template <>
bool PythonObjectCallback<WindowKeyPress>::operator() (WindowKeyPress *);

}

#endif
