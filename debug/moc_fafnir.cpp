/****************************************************************************
** Meta object code from reading C++ file 'fafnir.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../fafnir.h"
#include <QtNetwork/QSslError>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#include <QtCore/QList>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'fafnir.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LevelGauge__Fafnir_t {
    const uint offsetsAndSize[30];
    char stringdata0[177];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LevelGauge__Fafnir_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LevelGauge__Fafnir_t qt_meta_stringdata_LevelGauge__Fafnir = {
    {
QT_MOC_LITERAL(0, 18), // "LevelGauge::Fafnir"
QT_MOC_LITERAL(19, 5), // "start"
QT_MOC_LITERAL(25, 0), // ""
QT_MOC_LITERAL(26, 7), // "getData"
QT_MOC_LITERAL(34, 13), // "replyFinished"
QT_MOC_LITERAL(48, 14), // "QNetworkReply*"
QT_MOC_LITERAL(63, 4), // "resp"
QT_MOC_LITERAL(68, 15), // "watchDocTimeout"
QT_MOC_LITERAL(84, 22), // "authenticationRequired"
QT_MOC_LITERAL(107, 5), // "reply"
QT_MOC_LITERAL(113, 15), // "QAuthenticator*"
QT_MOC_LITERAL(129, 13), // "authenticator"
QT_MOC_LITERAL(143, 9), // "sslErrors"
QT_MOC_LITERAL(153, 16), // "QList<QSslError>"
QT_MOC_LITERAL(170, 6) // "errors"

    },
    "LevelGauge::Fafnir\0start\0\0getData\0"
    "replyFinished\0QNetworkReply*\0resp\0"
    "watchDocTimeout\0authenticationRequired\0"
    "reply\0QAuthenticator*\0authenticator\0"
    "sslErrors\0QList<QSslError>\0errors"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LevelGauge__Fafnir[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       6,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       0,       // signalCount

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   50,    2, 0x0a,    1 /* Public */,
       3,    0,   51,    2, 0x08,    2 /* Private */,
       4,    1,   52,    2, 0x08,    3 /* Private */,
       7,    0,   55,    2, 0x08,    5 /* Private */,
       8,    2,   56,    2, 0x08,    6 /* Private */,
      12,    2,   61,    2, 0x08,    9 /* Private */,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5,    6,
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 5, 0x80000000 | 10,    9,   11,
    QMetaType::Void, 0x80000000 | 5, 0x80000000 | 13,    9,   14,

       0        // eod
};

void LevelGauge::Fafnir::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<Fafnir *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->start(); break;
        case 1: _t->getData(); break;
        case 2: _t->replyFinished((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1]))); break;
        case 3: _t->watchDocTimeout(); break;
        case 4: _t->authenticationRequired((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QAuthenticator*>>(_a[2]))); break;
        case 5: _t->sslErrors((*reinterpret_cast< std::add_pointer_t<QNetworkReply*>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<QList<QSslError>>>(_a[2]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
        case 2:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 4:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        case 5:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType(); break;
            case 1:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QList<QSslError> >(); break;
            case 0:
                *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType::fromType< QNetworkReply* >(); break;
            }
            break;
        }
    }
}

const QMetaObject LevelGauge::Fafnir::staticMetaObject = { {
    QMetaObject::SuperData::link<TLevelGauge::staticMetaObject>(),
    qt_meta_stringdata_LevelGauge__Fafnir.offsetsAndSize,
    qt_meta_data_LevelGauge__Fafnir,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LevelGauge__Fafnir_t
, QtPrivate::TypeAndForceComplete<Fafnir, std::true_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>, QtPrivate::TypeAndForceComplete<QAuthenticator *, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<QNetworkReply *, std::false_type>, QtPrivate::TypeAndForceComplete<const QList<QSslError> &, std::false_type>


>,
    nullptr
} };


const QMetaObject *LevelGauge::Fafnir::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LevelGauge::Fafnir::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LevelGauge__Fafnir.stringdata0))
        return static_cast<void*>(this);
    return TLevelGauge::qt_metacast(_clname);
}

int LevelGauge::Fafnir::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = TLevelGauge::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 6)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
    }
    return _id;
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
