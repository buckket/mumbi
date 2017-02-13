/*
 * mumbimodule.c – control m-FS300 remote sockets (via Python)
 * Copyright © 2017 buckket
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <Python.h>

#include "mumbi.h"

static PyObject * mumbi_transmit(PyObject *self, PyObject *args) {
    unsigned int channel, status;
    unsigned int data_pin = DEFAULT_DATA_PIN;
    unsigned int power_pin = DEFAULT_POWER_PIN;
    if (!PyArg_ParseTuple(args, "Ib|II", &channel, &status, &data_pin, &power_pin)) {
        return NULL;
    }

    if (channel > 5) {
        PyErr_SetString(PyExc_ValueError, "Invalid channel number");
        return NULL;
    }

    int sts;
    sts = transmit(data_pin, power_pin, channel, status);
    return PyLong_FromLong(sts);
}

static PyMethodDef MumbiMethods[] = {
        {"transmit", mumbi_transmit, METH_VARARGS, "Transmit mumbi command."},
        {NULL,       NULL,           0,            NULL}
};

static struct PyModuleDef mumbimodule = {
        PyModuleDef_HEAD_INIT,
        "mumbi",
        NULL,
        -1,
        MumbiMethods
};

PyMODINIT_FUNC PyInit_mumbi(void) {
    PyObject *m;
    m = PyModule_Create(&mumbimodule);
    if (m == NULL) {
        return NULL;
    }
    return m;
}
