
#include <Python.h>
#include "structmember.h"
#include "encode_file.h"
#include "decode_file.h"

#ifdef __cplusplus
#   define BEGIN_C extern "C" {
#   define END_C   }
#else
#   define BEGIN_C
#   define END_C
#endif

BEGIN_C

static PyObject* CryptofError;

END_C

static Slice make_slice(const char* str)
{
	return make_slice(str, integer_cast<int>(strlen(str)));
}

static Slice Decode2UTF8(PyObject* input, PyObject** utf8)
{
	if (PyUnicode_Check(input))
	{
		*utf8 = PyUnicode_AsUTF8String(input);
		if (*utf8 == NULL)
		{
			PyErr_SetString(CryptofError, "Bad string parameter.");
			return Slice();
		}
		return make_slice(PyString_AsString(*utf8));
	}
    else if (PyString_Check(input))
    {
        return make_slice(PyString_AsString(input));
    }

    PyErr_SetString(CryptofError, "Bad string parameter.");
    return Slice();
}

BEGIN_C

/* CryptoMeta */
typedef struct
{
    PyObject_HEAD
    PyObject* mMd5Sum;
    uint64_t  mFileEnd;
    int       mFileNameVersion;
    int       mFileDataVersion;
    int       mFileNameLength;
} Meta;

static PyMemberDef Meta_members[] =
{
    { (char *)"end",      T_LONG,      offsetof(Meta, mFileEnd),         READONLY, (char *)"file offset end"   },
    { (char *)"name_v",   T_INT,       offsetof(Meta, mFileNameVersion), READONLY, (char *)"file name version" },
    { (char *)"data_v",   T_INT,       offsetof(Meta, mFileDataVersion), READONLY, (char *)"file data version" },
    { (char *)"name_len", T_INT,       offsetof(Meta, mFileNameLength),  READONLY, (char *)"file name length"  },
    { (char *)"md5sum",   T_OBJECT_EX, offsetof(Meta, mMd5Sum),          READONLY, (char *)"file md5sum"       },
    { NULL }  /* Sentinel */
};

static PyObject* Meta_new(PyTypeObject *type, PyObject* args, PyObject* kwargs)
{
    Meta* self = (Meta *)type->tp_alloc(type, 0);

    self->mMd5Sum = Py_None;
    Py_INCREF(Py_None);

    self->mFileNameVersion = v1.first;
    self->mFileDataVersion = v1.second;
    self->mFileNameLength  = 0;
    self->mFileEnd         = 0;

    return (PyObject *)self;
}

static PyObject* Meta_isvaild(Meta* self, PyObject* args)
{
    bool vaild = contains(FileDataCrypto, self->mFileDataVersion)
                && contains(FileNameCrypto, self->mFileNameVersion)
                && self->mFileNameLength >= 0;

    if (vaild)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

static PyObject* Meta_str(PyObject* obj)
{
    Meta* self = (Meta *)obj;
    PyObject* dict = PyDict_New();

    PyDict_SetItemString(dict, "endof",       Py_BuildValue("l", self->mFileEnd));
    PyDict_SetItemString(dict, "version",     Py_BuildValue("(ii)", self->mFileNameVersion, self->mFileDataVersion));
    PyDict_SetItemString(dict, "name length", Py_BuildValue("i", self->mFileNameLength));
    PyDict_SetItemString(dict, "md5sum",      self->mMd5Sum);
    Py_INCREF(self->mMd5Sum);

    PyObject* str = PyObject_Str(dict);
    Py_DECREF(dict);
    return str;
}

static PyMethodDef Meta_methods[] =
{
    { "is_vaild", (PyCFunction)Meta_isvaild, METH_NOARGS, "noargs, Check meta vaild or not" },
    { NULL, NULL, 0, NULL }  /* Sentinel */
};

static PyTypeObject MetaType =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "cryptof.Meta",            /*tp_name*/
    sizeof(Meta),              /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    Meta_str,                  /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "Meta objects",            /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Meta_methods,              /* tp_methods */
    Meta_members,              /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    Meta_new,                  /* tp_new */
};

static PyObject* CryptoMeta2PyObject(const CryptoMeta& meta)
{
    Meta* item = (Meta *)Meta_new(&MetaType, NULL, NULL);

    item->mFileNameVersion = meta.mFileNameVersion;
    item->mFileDataVersion = meta.mFileDataVersion;
    item->mFileNameLength  = meta.mFileNameLength;
    item->mFileEnd         = meta.mFileEnd;

    if (meta.mMd5SumLength > 0)
    {
        item->mMd5Sum = Py_BuildValue("s#", meta.mMd5Sum, meta.mMd5SumLength);
    }
    else
    {
        item->mMd5Sum = Py_None;
        Py_INCREF(Py_None);
    }

    return (PyObject *)item;
};

/* FileMeta */
typedef struct
{
    PyObject_HEAD
    PyObject* mName;
    PyObject* mMeta;
    uint64_t  mOffset;
    uint64_t  mFileSize;
    int       mWriteBSize;
} FileMetaV;

static PyMemberDef FileMetaV_members[] =
{
    { (char *)"name",        T_OBJECT_EX, offsetof(FileMetaV, mName),       READONLY, (char *)"file name" },
    { (char *)"meta",        T_OBJECT_EX, offsetof(FileMetaV, mMeta),       READONLY, (char *)"file meta" },
    { (char *)"offset",      T_INT,       offsetof(FileMetaV, mOffset),     READONLY, (char *)"file offset in encoder file" },
    { (char *)"size",        T_LONG,      offsetof(FileMetaV, mFileSize),   READONLY, (char *)"file size" },
    { (char *)"write_bsize", T_INT,       offsetof(FileMetaV, mWriteBSize), READONLY, (char *)"file write block size" },
    { NULL }  /* Sentinel */
};

static PyObject* FileMetaV_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    FileMetaV* self = (FileMetaV *)type->tp_alloc(type, 0);

    self->mName = Py_None;
    Py_INCREF(Py_None);

    self->mMeta = Py_None;
    Py_INCREF(Py_None);

    self->mOffset     = 0;
    self->mFileSize   = 0;
    self->mWriteBSize = 0;

    return (PyObject *)self;
}

static PyObject* FileMetaV_str(PyObject* obj)  
{
    FileMetaV* self = (FileMetaV *)obj;
    PyObject* dict = PyDict_New();

    PyDict_SetItemString(dict, "name", self->mName); Py_INCREF(self->mName);
    PyDict_SetItemString(dict, "meta", self->mMeta); Py_INCREF(self->mMeta);
    PyDict_SetItemString(dict, "offset",     Py_BuildValue("l", self->mOffset));
    PyDict_SetItemString(dict, "filesize",   Py_BuildValue("l", self->mFileSize));
    PyDict_SetItemString(dict, "writebsize", Py_BuildValue("l", self->mWriteBSize));

    PyObject* str = PyObject_Str(dict);
    Py_DECREF(dict);
    return str;
}

static PyTypeObject FileMetaType =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "cryptof.FileMeta",        /*tp_name*/
    sizeof(FileMetaV),         /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    0,                         /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    FileMetaV_str,             /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    "FileMeta objects",        /* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    0,                         /* tp_methods */
    FileMetaV_members,         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    0,                         /* tp_init */
    0,                         /* tp_alloc */
    FileMetaV_new,             /* tp_new */
};

static PyObject* FileMeta2PyObject(const FileMeta& fmeta)
{
    FileMetaV* item = (FileMetaV *)FileMetaV_new(&FileMetaType, NULL, NULL);

    const char *errors = "DecodeFileName of meta failed";
    PyObject* name = PyUnicode_DecodeUTF8(fmeta.mName.data(), integer_cast<Py_ssize_t>(fmeta.mName.length()), errors);
    if (name != NULL)
    {
        item->mName = name;
    }

    item->mMeta = CryptoMeta2PyObject(fmeta.mMeta);
    item->mOffset = fmeta.mOffset;
    item->mFileSize = fmeta.mFileSize;
    item->mWriteBSize = fmeta.mWriteBSize;

    return (PyObject *)item;
};

static PyObject* FileList2Dict(const FileList& flist)
{
    PyObject* dict = PyDict_New();
    if (dict == NULL) return NULL;

    for (BOOST_AUTO(file, flist.begin()); file != flist.end(); ++file)
    {
        const char* errors = "DecodeFileName of list failed";
        PyObject* name = PyUnicode_DecodeUTF8(file->first.data(), integer_cast<Py_ssize_t>(file->first.length()), errors);
        if (name != NULL)
        {
            PyObject* meta = FileMeta2PyObject(file->second);
            if (0 != PyDict_SetItem(dict, name, meta))
            {
                Py_DECREF(dict);
                PyErr_SetString(CryptofError, "FileList2Dict add item faild");
                return NULL;
            }
        }
    }

    return dict;
}

/* Encoder */
typedef struct
{
    PyObject_HEAD
    EncodeFile* coder;  
} Encoder;

static PyObject* Encoder_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    Encoder* self;

    self = (Encoder *)type->tp_alloc(type, 0);
    if (self == NULL) return NULL;

    self->coder = NULL;

    return (PyObject *)self;
}

static int Encoder_init(Encoder* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "output", "key", NULL };
    
    PyObject* output = NULL;
    char* key = NULL; 
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|s", const_cast<char **>(kwlist),
            &output, &key))
    {
        return -1;
    }

    if (key == NULL)
    {
        key = const_cast<char *>("a2b4c6d8e0,");
    }

    PyObject* outUni = NULL;
    try
    {
        self->coder = new EncodeFile(Decode2UTF8(output, &outUni), make_slice(key));
        self->coder->default_version(v1);
    }
    catch (std::exception& ex)
    {
        Py_XDECREF(outUni);
        PyErr_SetString(CryptofError, ex.what());
        return -1;
    }

    Py_XDECREF(outUni);
    return 0;
}

static void Encoder_dealloc(Encoder* self)
{
    delete self->coder;
    self->coder = NULL;
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject* Encoder_filelist(Encoder* self, PyObject* args)
{
    return FileList2Dict(self->coder->filelist());
}

static PyObject* Encoder_version(Encoder* self, PyObject* args)
{
    CryptoVersion version = self->coder->default_version();
    return Py_BuildValue("(ii)", version.first, version.second);
}

static PyObject* Encoder_setVersion(Encoder* self, PyObject* args)
{
    CryptoVersion version;
    if (!PyArg_ParseTuple(args, "(ii)", &version.first, &version.second))
    {
        return NULL;
    }

    self->coder->default_version(version);
    Py_RETURN_TRUE;
}

static PyObject* Encoder_str(PyObject* obj)  
{
    Encoder* self = (Encoder *)obj;
    PyObject* dict = PyDict_New();

    PyDict_SetItemString(dict, "version",  Encoder_version(self, NULL));
    PyDict_SetItemString(dict, "filelist", Encoder_filelist(self, NULL));

    PyObject* str = PyObject_Str(dict);
    Py_DECREF(dict);
    return str;
}

static PyObject* Encoder_push(Encoder* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "input", "version", NULL };

    CryptoVersion version = self->coder->default_version();
    PyObject* input = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|(ii)", const_cast<char **>(kwlist),
            &input, &version.first, &version.second))
    {
        return NULL;
    }

    PyObject* inUni = NULL;
    bool result = self->coder->push(Decode2UTF8(input, &inUni), &version);

    Py_XDECREF(inUni);

    if (result)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

static PyObject* Encoder_meta(Encoder* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "input", NULL };
    
    PyObject* input = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", const_cast<char **>(kwlist), &input))
    {
        return NULL;
    }

    FileMeta fmeta;
    PyObject* inUni = NULL;
    if (self->coder->get(Decode2UTF8(input, &inUni), fmeta))
    {
        Py_XDECREF(inUni);
        Py_RETURN_NONE;
    }

    Py_XDECREF(inUni);
    return FileMeta2PyObject(fmeta);
}

static PyMethodDef Encoder_methods[] =
{
    { "filelist", (PyCFunction)Encoder_filelist, METH_NOARGS, "noargs, Return the file list in encoder file" },
    { "version",  (PyCFunction)Encoder_version,  METH_NOARGS, "noargs, Return the encoder's version" },
    { "push",     (PyCFunction)Encoder_push,     METH_VARARGS | METH_KEYWORDS, "args:(input, version), Push a file into encoder file" },
    { "meta",     (PyCFunction)Encoder_meta,     METH_VARARGS | METH_KEYWORDS, "args:(input), Get filemeta in encoder file" },
    { "set_version", (PyCFunction)Encoder_setVersion,  METH_VARARGS, "args:(version), Set the encoder's default version" },
    { NULL, NULL, 0, NULL }  /* Sentinel */
};

static PyTypeObject EncoderType =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "cryptof.Encoder",         /*tp_name*/
    sizeof(Encoder),           /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Encoder_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    Encoder_str,               /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/*tp_flags*/
    "Encoder objects, args:(output, key)",/* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Encoder_methods,           /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Encoder_init,    /* tp_init */
    0,                         /* tp_alloc */
    Encoder_new,               /* tp_new */
};

/* Decoder */
typedef struct
{
    PyObject_HEAD
    DecodeFile* coder;  
} Decoder;

static PyObject* Decoder_new(PyTypeObject* type, PyObject* args, PyObject* kwargs)
{
    Decoder* self;

    self = (Decoder *)type->tp_alloc(type, 0);
    if (self == NULL) return NULL;

    self->coder = NULL;

    return (PyObject *)self;
}

static int Decoder_init(Decoder* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "input", "key", NULL };
    
    PyObject* input = NULL;
    char* key = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O|s", const_cast<char **>(kwlist), &input, &key))
    {
        return -1;
    }

    if (key == NULL)
    {
        key = const_cast<char *>("a2b4c6d8e0,");
    }

    PyObject* inUni = NULL;
    try
    {
        self->coder = new DecodeFile(Decode2UTF8(input, &inUni), make_slice(key));
    }
    catch (std::exception& ex)
    {
        Py_XDECREF(inUni);
        PyErr_SetString(CryptofError, ex.what());
        return -1;
    }

    Py_XDECREF(inUni);
    return 0;
}

static void Decoder_dealloc(Decoder* self)
{
    delete self->coder;
    self->coder = NULL;
    self->ob_type->tp_free((PyObject *)self);
}

static PyObject* Decoder_load(Decoder* self, PyObject* args)
{
    if (self->coder->load())
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

static PyObject* Decoder_filelist(Decoder* self, PyObject* args)
{
    return FileList2Dict(self->coder->filelist());
}

static PyObject* Decoder_str(PyObject* obj)  
{
    Decoder* self = (Decoder *)obj;
    PyObject* dict = Decoder_filelist(self, NULL);
    PyObject* str = PyObject_Str(dict);
    Py_DECREF(dict);
    return str;
}

static PyObject* Decoder_pop(Decoder* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "input", "output", NULL };
    
    PyObject* input = NULL;
    PyObject* output = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "OO", const_cast<char **>(kwlist),
            &input, &output))
    {
        return NULL;
    }

    PyObject* inUni = NULL;
    PyObject* outUni = NULL;
    bool result = self->coder->pop(Decode2UTF8(input, &inUni), Decode2UTF8(output, &outUni));

    Py_XDECREF(inUni);
    Py_XDECREF(outUni);

    if (result)
    {
        Py_RETURN_TRUE;
    }
    else
    {
        Py_RETURN_FALSE;
    }
}

static PyObject* Decoder_meta(Decoder* self, PyObject* args, PyObject* kwargs)
{
    static const char* kwlist[] = { "input", NULL };
    
    PyObject* input = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "O", const_cast<char **>(kwlist), &input))
    {
        return NULL;
    }

    FileMeta fmeta;
    PyObject* inUni = NULL;
    if (self->coder->get(Decode2UTF8(input, &inUni), fmeta))
    {
        Py_XDECREF(inUni);
        Py_RETURN_NONE;
    }

    Py_XDECREF(inUni);
    return FileMeta2PyObject(fmeta);
}

static PyMethodDef Decoder_methods[] =
{
    { "load",     (PyCFunction)Decoder_load,     METH_NOARGS, "noargs, Load a encoder file" },
    { "filelist", (PyCFunction)Decoder_filelist, METH_NOARGS, "noargs, Return the file list in encoder file" },
    { "pop",      (PyCFunction)Decoder_pop,      METH_VARARGS | METH_KEYWORDS, "args:(input, output), Pop a file from encoder file" },
    { "meta",     (PyCFunction)Decoder_meta,     METH_VARARGS | METH_KEYWORDS, "args:(input), Get filemeta in encoder file" },
    { NULL, NULL, 0, NULL }  /* Sentinel */
};

static PyTypeObject DecoderType =
{
    PyObject_HEAD_INIT(NULL)
    0,                         /*ob_size*/
    "cryptof.Decoder",         /*tp_name*/
    sizeof(Decoder),           /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Decoder_dealloc,/*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    Decoder_str,               /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,/*tp_flags*/
    "Decoder objects, args:(input, key)",/* tp_doc */
    0,                         /* tp_traverse */
    0,                         /* tp_clear */
    0,                         /* tp_richcompare */
    0,                         /* tp_weaklistoffset */
    0,                         /* tp_iter */
    0,                         /* tp_iternext */
    Decoder_methods,           /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Decoder_init,    /* tp_init */
    0,                         /* tp_alloc */
    Decoder_new,               /* tp_new */
};

static PyMethodDef cryptof_methods[] =
{
    { NULL, NULL, 0, NULL } /* Sentinel */
};

END_C

/* module init */
PyMODINIT_FUNC initcryptof()
{
    PyObject* m = NULL;

    init_global();

    CryptofError = PyErr_NewException((char *)"cryptof.Error", NULL, NULL);

    if (PyType_Ready(&MetaType) < 0)
        return ;

    if (PyType_Ready(&FileMetaType) < 0)
        return ;

    if (PyType_Ready(&EncoderType) < 0)
        return ;

    if (PyType_Ready(&DecoderType) < 0)
        return ;

    m = Py_InitModule("cryptof", cryptof_methods);
    if (m == NULL)
        return ;

    Py_INCREF(CryptofError);
    PyModule_AddObject(m, "Error", CryptofError);

    Py_INCREF(&MetaType);
    PyModule_AddObject(m, "Meta", (PyObject *)&MetaType);

    Py_INCREF(&FileMetaType);
    PyModule_AddObject(m, "FileMeta", (PyObject *)&FileMetaType);

    Py_INCREF(&EncoderType);
    PyModule_AddObject(m, "Encoder", (PyObject *)&EncoderType);

    Py_INCREF(&DecoderType);
    PyModule_AddObject(m, "Decoder", (PyObject *)&DecoderType);
}

