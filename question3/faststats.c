#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <math.h>

/* convert Python number to C double */
static int extract_double(PyObject *item, double *out) {
    if (PyBool_Check(item)) {
        PyErr_SetString(PyExc_TypeError,
			"Boolean values aren't valid numeric inputs");
        return 0;
    }

    if (PyFloat_Check(item)) {*out = PyFloat_AS_DOUBLE(item);
    } else if (PyLong_Check(item)) {
        *out = PyLong_AsDouble(item);
        if (PyErr_Occurred()) {return 0;}
    } else {
        PyErr_SetString(PyExc_TypeError,
                        "Sequence elements must be int or float");
        return 0;
    }

    if (!isfinite(*out)) {
        PyErr_SetString(PyExc_ValueError, "Values must be finite numbers");
        return 0;
    }

    return 1;
}

/* mean & population var */
static PyObject* py_calculate_stats(PyObject *self, PyObject *args) {
    PyObject *seq_obj;
    if (!PyArg_ParseTuple(args, "O", &seq_obj)) return NULL;

    PyObject *seq = PySequence_Fast(seq_obj, "Expected a sequence");
    if (!seq) return NULL;

    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    if (n == 0) {
        Py_DECREF(seq);
        PyErr_SetString(PyExc_ValueError, "Dataset cannot be empty");
        return NULL;
    }

    PyObject **items = PySequence_Fast_ITEMS(seq);
    double sum = 0.0, val;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (!extract_double(items[i], &val)) {
            Py_DECREF(seq);
            return NULL;
        }
        sum += val;
    }

    double mean = sum / (double)n;
    double sq_diff_sum = 0.0;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (!extract_double(items[i], &val)) {
            Py_DECREF(seq);
            return NULL;
        }
        double diff = val - mean;
        sq_diff_sum += diff * diff;
    }

    Py_DECREF(seq);
    double variance = sq_diff_sum / (double)n;

    return Py_BuildValue("(dd)", mean, variance);
}

static PyObject* py_find_min_max(PyObject *self, PyObject *args) {
    PyObject *seq_obj;
    if (!PyArg_ParseTuple(args, "O", &seq_obj)) return NULL;

    PyObject *seq = PySequence_Fast(seq_obj, "Expected a sequence");
    if (!seq) return NULL;

    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    if (n == 0) {
        Py_DECREF(seq);
        PyErr_SetString(PyExc_ValueError, "Dataset cannot be empty");
        return NULL;
    }

    PyObject **items = PySequence_Fast_ITEMS(seq);
    double min_val, max_val, val;
    if (!extract_double(items[0], &min_val)) {
        Py_DECREF(seq);
        return NULL;
    }

    max_val = min_val;
    for (Py_ssize_t i = 1; i < n; i++) {
        if (!extract_double(items[i], &val)) {
            Py_DECREF(seq);
            return NULL;
        }
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    Py_DECREF(seq);

    return Py_BuildValue("(dd)", min_val, max_val);
}

/* values outside thresholds */
static PyObject* py_count_outliers(PyObject *self, PyObject *args) {
    PyObject *seq_obj, *low_obj, *high_obj;
    double low, high;
    if (!PyArg_ParseTuple(args, "OOO", &seq_obj, &low_obj, &high_obj))
        return NULL;
    if (!extract_double(low_obj, &low) || !extract_double(high_obj, &high))
        return NULL;
    if (low > high) {
        PyErr_SetString(PyExc_ValueError,
                        "Lower threshold cannot exceed upper threshold");
        return NULL;
    }

    PyObject *seq = PySequence_Fast(seq_obj, "Expected a sequence");
    if (!seq) return NULL;

    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    if (n == 0) {
        Py_DECREF(seq);
        PyErr_SetString(PyExc_ValueError, "Dataset cannot be empty");
        return NULL;
    }

    PyObject **items = PySequence_Fast_ITEMS(seq);
    Py_ssize_t count = 0;
    double val;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (!extract_double(items[i], &val)) {
            Py_DECREF(seq);
            return NULL;
        }
        if (val < low || val > high) count++;
    }

    Py_DECREF(seq);

    return PyLong_FromSsize_t(count);
}

/* min-max normalization */
static PyObject* py_normalize(PyObject *self, PyObject *args) {
    PyObject *seq_obj;
    if (!PyArg_ParseTuple(args, "O", &seq_obj)) return NULL;

    PyObject *seq = PySequence_Fast(seq_obj, "Expected a sequence");
    if (!seq) return NULL;

    Py_ssize_t n = PySequence_Fast_GET_SIZE(seq);
    if (n == 0) {
        Py_DECREF(seq);
        PyErr_SetString(PyExc_ValueError, "Dataset cannot be empty");
        return NULL;
    }

    PyObject **items = PySequence_Fast_ITEMS(seq);
    double min_val, max_val, val;
    if (!extract_double(items[0], &min_val)) {
        Py_DECREF(seq);
        return NULL;
    }

    max_val = min_val;
    for (Py_ssize_t i = 1; i < n; i++) {
        if (!extract_double(items[i], &val)) {
            Py_DECREF(seq);
            return NULL;
        }
        if (val < min_val) min_val = val;
        if (val > max_val) max_val = val;
    }

    PyObject *result = PyList_New(n);
    if (!result) {
        Py_DECREF(seq);
        return NULL;
    }

    double span = max_val - min_val;
    for (Py_ssize_t i = 0; i < n; i++) {
        if (!extract_double(items[i], &val)) {
            Py_DECREF(result);
            Py_DECREF(seq);
            return NULL;
        }
        double normalized = 0.0;
        if (span != 0.0) normalized = (val - min_val) / span;
        PyObject *item = PyFloat_FromDouble(normalized);
        if (!item) {
            Py_DECREF(result);
            Py_DECREF(seq);
            return NULL;
        }
        PyList_SET_ITEM(result, i, item);
    }

    Py_DECREF(seq);

    return result;
}

static PyMethodDef FastStatsMethods[] = {
    {"calculate_stats", py_calculate_stats, METH_VARARGS,
     "Calculate mean and population variance"},

    {"find_min_max", py_find_min_max, METH_VARARGS,
     "Find minimum and maximum values"},

    {"count_outliers", py_count_outliers, METH_VARARGS,
     "Count values outside thresholds"},

    {"normalize", py_normalize, METH_VARARGS,
     "Normalize values between 0 and 1"},

    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef faststatsmodule = {
    PyModuleDef_HEAD_INIT,
    "faststats",
    "Statistical operations implemented in C",
    -1,
    FastStatsMethods
};

PyMODINIT_FUNC PyInit_faststats(void) {
    return PyModule_Create(&faststatsmodule);
}

