/*
 * LabDb C Python Bindings
 * Minimal C-based Python extension for db9 S-expression interface
 * 
 * Provides just two functions:
 * - execute_db9_command(command_string) -> json_string
 * - get_db9_specification() -> specification_string
 */

#include <Python.h>

#include "LabDb/Db9Dispatcher.h"
#include "LabDb/DatabaseVerbs.h"
#include <iostream>
#include <fstream>

//-----------------------------------------------------------------------------
// Helper function to convert C++ exceptions to Python exceptions
//-----------------------------------------------------------------------------
static void handle_cpp_exception() {
    try {
        throw;
    } catch (const std::exception& e) {
        PyErr_SetString(PyExc_RuntimeError, e.what());
    } catch (...) {
        PyErr_SetString(PyExc_RuntimeError, "Unknown C++ exception occurred");
    }
}

//-----------------------------------------------------------------------------
// Python wrapper: execute_db9_command(command_string) -> json_string
//-----------------------------------------------------------------------------
static PyObject* py_execute_db9_command(PyObject* self, PyObject* args) {
    const char* command_str;
    
    // Parse Python arguments
    if (!PyArg_ParseTuple(args, "s", &command_str)) {
        return NULL;
    }
    
    try {
        // Execute through global dispatcher
        LabDb::Db9Response response = LabDb::getGlobalDb9Dispatcher().executeCommand(command_str);
        
        // Convert response to JSON and return as Python string
        std::string json_result = response.toJson();
        return PyUnicode_FromString(json_result.c_str());
        
    } catch (...) {
        handle_cpp_exception();
        return NULL;
    }
}

//-----------------------------------------------------------------------------
// Python wrapper: execute_db9_commands(command_list) -> json_string
//-----------------------------------------------------------------------------
static PyObject* py_execute_db9_commands(PyObject* self, PyObject* args) {
    PyObject* command_list;
    
    // Parse Python arguments (expecting a list of strings)
    if (!PyArg_ParseTuple(args, "O!", &PyList_Type, &command_list)) {
        return NULL;
    }
    
    try {
        // Convert Python list to C++ vector
        std::vector<std::string> commands;
        Py_ssize_t list_size = PyList_Size(command_list);
        
        for (Py_ssize_t i = 0; i < list_size; ++i) {
            PyObject* item = PyList_GetItem(command_list, i);
            if (!PyUnicode_Check(item)) {
                PyErr_SetString(PyExc_TypeError, "All commands must be strings");
                return NULL;
            }
            
            const char* command_str = PyUnicode_AsUTF8(item);
            if (command_str == NULL) {
                return NULL;  // Error already set by PyUnicode_AsUTF8
            }
            
            commands.emplace_back(command_str);
        }
        
        // Execute through global dispatcher
        LabDb::Db9Response response = LabDb::getGlobalDb9Dispatcher().executeCommands(commands);
        
        // Convert response to JSON and return as Python string
        std::string json_result = response.toJson();
        return PyUnicode_FromString(json_result.c_str());
        
    } catch (...) {
        handle_cpp_exception();
        return NULL;
    }
}

//-----------------------------------------------------------------------------
// Python wrapper: get_db9_specification() -> specification_string
//-----------------------------------------------------------------------------
static PyObject* py_get_db9_specification(PyObject* self, PyObject* args) {
    try {
        // Get specification from global dispatcher
        std::string specification = LabDb::getGlobalDb9Dispatcher().getSpecification();
        return PyUnicode_FromString(specification.c_str());
        
    } catch (...) {
        handle_cpp_exception();
        return NULL;
    }
}

//-----------------------------------------------------------------------------
// Python wrapper: get_db9_available_verbs() -> list_of_strings
//-----------------------------------------------------------------------------
static PyObject* py_get_db9_available_verbs(PyObject* self, PyObject* args) {
    try {
        // Get available verbs from global dispatcher
        std::vector<std::string> verbs = LabDb::getGlobalDb9Dispatcher().getAvailableVerbs();
        
        // Convert to Python list
        PyObject* py_list = PyList_New(verbs.size());
        if (py_list == NULL) {
            return NULL;
        }
        
        for (size_t i = 0; i < verbs.size(); ++i) {
            PyObject* py_str = PyUnicode_FromString(verbs[i].c_str());
            if (py_str == NULL) {
                Py_DECREF(py_list);
                return NULL;
            }
            PyList_SetItem(py_list, i, py_str);  // Steals reference to py_str
        }
        
        return py_list;
        
    } catch (...) {
        handle_cpp_exception();
        return NULL;
    }
}

//-----------------------------------------------------------------------------
// Method definitions table
//-----------------------------------------------------------------------------
static PyMethodDef labdb_methods[] = {
    {
        "execute_db9_command", 
        py_execute_db9_command, 
        METH_VARARGS,
        "Execute single db9 S-expression command and return JSON response.\n"
        "\n"
        "Args:\n"
        "    command (str): S-expression command (e.g., '(add-triple granite contains quartz)')\n"
        "\n"
        "Returns:\n"
        "    str: JSON response with status, result, and auto-reflexive metrics\n"
        "\n"
        "Example:\n"
        "    >>> result = execute_db9_command('(stats)')\n"
        "    >>> json.loads(result)['status']  # 'success' or 'error'\n"
    },
    {
        "execute_db9_commands", 
        py_execute_db9_commands, 
        METH_VARARGS,
        "Execute multiple db9 S-expression commands and return JSON response.\n"
        "\n"
        "Args:\n"
        "    commands (list): List of S-expression command strings\n"
        "\n"
        "Returns:\n"
        "    str: JSON response with combined results\n"
        "\n"
        "Example:\n"
        "    >>> commands = ['(open-database test.db9)', '(stats)']\n"
        "    >>> result = execute_db9_commands(commands)\n"
    },
    {
        "get_db9_specification", 
        py_get_db9_specification, 
        METH_NOARGS,
        "Get complete db9 specification and usage guide.\n"
        "\n"
        "Returns:\n"
        "    str: Complete markdown specification for all available verbs\n"
        "\n"
        "Example:\n"
        "    >>> spec = get_db9_specification()\n"
        "    >>> print(spec)  # Shows all available verbs and usage\n"
    },
    {
        "get_db9_available_verbs", 
        py_get_db9_available_verbs, 
        METH_NOARGS,
        "Get list of available db9 verbs.\n"
        "\n"
        "Returns:\n"
        "    list: List of available verb names\n"
        "\n"
        "Example:\n"
        "    >>> verbs = get_db9_available_verbs()\n"
        "    >>> print(verbs)  # ['add-triple', 'find-triple', 'stats', ...]\n"
    },
    {NULL, NULL, 0, NULL}  // Sentinel
};

//-----------------------------------------------------------------------------
// Module definition
//-----------------------------------------------------------------------------
static struct PyModuleDef labdb_module = {
    PyModuleDef_HEAD_INIT,
    "labdb",                          // Module name
    "LabDb C Python Bindings\n"        // Module docstring
    "\n"
    "Minimal C-based Python extension for db9 S-expression interface.\n"
    "Provides direct access to LabDb's triadic consciousness database operations.\n"
    "\n"
    "Functions:\n"
    "  execute_db9_command(command) -> json_response\n"
    "  execute_db9_commands(commands) -> json_response\n"
    "  get_db9_specification() -> specification_text\n"
    "  get_db9_available_verbs() -> verb_list\n"
    "\n"
    "Example:\n"
    "    import labdb_c\n"
    "    result = labdb_c.execute_db9_command('(stats)')\n"
    "    spec = labdb_c.get_db9_specification()\n",
    -1,                                 // Size of per-interpreter state
    labdb_methods                       // Method table
};

//-----------------------------------------------------------------------------
// Module initialization function
//-----------------------------------------------------------------------------

extern "C" void initDatabaseVerbRegistration();

PyMODINIT_FUNC PyInit_labdb(void) {
    std::ofstream logfile("/tmp/labdb_native_debug.log", std::ios::app);
    logfile << "LabDb native module initializing..." << std::endl;

    initDatabaseVerbRegistration();

    auto &dispatcher = LabDb::getGlobalDb9Dispatcher();
    logfile << "Dispatcher acquired\n";

    auto verbs = dispatcher.getAvailableVerbs();
    logfile << "Available verbs:\n";
    for (const auto& verb : verbs) {
        logfile << "- " << verb << "\n";
    }

    logfile.flush();
    return PyModule_Create(&labdb_module);
}