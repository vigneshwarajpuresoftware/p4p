#include "p4p_client.h"

namespace pvd = epics::pvData;
namespace pva = epics::pvAccess;

PyClassWrapper_DEF(PyOp, "Operation")

OpBase::~OpBase()
{
    cancel();
}

void OpBase::cancel()
{
    TRACE((cb?"active":"used"));
    if(cb) {
        PyRef err(PyObject_CallFunction(P4PCancelled, "s", "Cancelled"));
        call_cb(err.get());
    }
}

void OpBase::call_cb(PyObject *obj)
{
    if(!cb) return;
    PyObject *junk = PyObject_CallFunctionObjArgs(cb.get(), obj, NULL);
    if(junk) {
        Py_DECREF(junk);
    } else {
        PyErr_Print();
        PyErr_Clear();
    }
    cb.reset();
}

namespace {

#define TRY PyOp::reference_type SELF = PyOp::unwrap(self); try

PyObject* py_op_cancel(PyObject *self)
{
    TRY {
        SELF->cancel();

        Py_RETURN_NONE;
    } CATCH()
    return NULL;
}

int py_op_traverse(PyObject *self, visitproc visit, void *arg)
{
    TRY {
        if(SELF->cb)
            Py_VISIT(SELF->cb.get());
        if(SELF->pyvalue)
            Py_VISIT(SELF->pyvalue.get());
        return 0;
    } CATCH()
    return -1;
}

int py_op_clear(PyObject *self)
{
    TRY {
        // ~= Py_CLEAR(cb)
        {
            PyRef tmp;
            SELF->cb.swap(tmp);
        }
        {
            PyRef tmp;
            SELF->pyvalue.swap(tmp);
        }
        return 0;
    } CATCH()
    return -1;
}

static PyMethodDef OpBase_methods[] = {
    {"cancel", (PyCFunction)&py_op_cancel, METH_NOARGS,
     "Cancel pending operation."},
    {NULL}
};

} // namespace

void p4p_client_op_register(PyObject *mod)
{
    PyOp::buildType();
    PyOp::type.tp_flags = Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC;
    PyOp::type.tp_traverse = &py_op_traverse;
    PyOp::type.tp_clear = &py_op_clear;

    PyOp::type.tp_methods = OpBase_methods;

    PyOp::finishType(mod, "Operation");
}
