/****************************************************************************
** Meta object code from reading C++ file 'sens.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../sens.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'sens.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LevelGauge__Sens_t {
    const uint offsetsAndSize[22];
    char stringdata0[157];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LevelGauge__Sens_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LevelGauge__Sens_t qt_meta_stringdata_LevelGauge__Sens = {
    {
QT_MOC_LITERAL(0, 16), // "LevelGauge::Sens"
QT_MOC_LITERAL(17, 5), // "start"
QT_MOC_LITERAL(23, 0), // ""
QT_MOC_LITERAL(24, 15), // "connentedSocket"
QT_MOC_LITERAL(40, 18), // "disconnentedSocket"
QT_MOC_LITERAL(59, 15), // "readyReadSocket"
QT_MOC_LITERAL(75, 19), // "errorOccurredSocket"
QT_MOC_LITERAL(95, 28), // "QAbstractSocket::SocketError"
QT_MOC_LITERAL(124, 7), // "getData"
QT_MOC_LITERAL(132, 15), // "watchDocTimeout"
QT_MOC_LITERAL(148, 8) // "sendData"

    },
    "LevelGauge::Sens\0start\0\0connentedSocket\0"
    "disconnentedSocket\0readyReadSocket\0"
    "errorOccurredSocket\0QAbstractSocket::SocketError\0"
    "getData\0watchDocTimeout\0sendData"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LevelGauge__Sens[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       8,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   62,    2, 0x0a,    1 /* Public */,
       3,    0,   63,    2, 0x08,    2 /* Private */,
       4,    0,   64,    2, 0x08,    3 /* Private */,
       5,    0,   65,    2, 0x08,    4 /* Private */,
       6,    1,   66,    2, 0x08,    5 /* Private */,
       8,    0,   69,    2, 0x08,    7 /* Private */,
       9,    0,   70,    2, 0x08,    8 /* Private */,
      10,    0,   71,    2, 0x08,    9 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 7,    2,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void LevelGauge::Sens::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Sens *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->start(); break;
        case 1: _t->connentedSocket(); break;
        case 2: _t->disconnentedSocket(); break;
        case 3: _t->readyReadSocket(); break;
        case 4: _t->errorOccurredSocket((*reinterpret_cast< std::add_pointer_t<QAbstractSocket::SocketError>>(_a[1]))); break;
        case 5: _t->getData(); break;
        case 6: _t->watchDocTimeout(); break;
        case 7: _t->sendData(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QAbstractSocket::SocketError >(); break;
            }
            break;
        }
    }
}

const QMetaObject LevelGauge::Sens::staticMetaObject = { {
    QMetaObject::SuperData::link<TLevelGauge::staticMetaObject>(),
    qt_meta_stringdata_LevelGauge__Sens.offsetsAndSize,
    qt_meta_data_LevelGauge__Sens,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LevelGauge__Sens_t
, QtPrivate::TypeAndForceComplete<Sens, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QAbstractSocket::SocketError, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>


>,
    nullptr
} };


const QMetaObject *LevelGauge::Sens::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LevelGauge::Sens::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LevelGauge__Sens.stringdata0))
        return static_cast<void*>(this);
    return TLevelGauge::qt_metacast(_clname);
}

int LevelGauge::Sens::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TLevelGauge::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 8)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
