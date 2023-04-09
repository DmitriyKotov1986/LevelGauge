/****************************************************************************
** Meta object code from reading C++ file 'tlevelgaugemonitoring.h'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.3.1)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../tlevelgaugemonitoring.h"
#include <QtNetwork/QSslError>
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'tlevelgaugemonitoring.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.3.1. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring_t {
    const uint offsetsAndSize[38];
    char stringdata0[264];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring_t qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring = {
    {
QT_MOC_LITERAL(0, 33), // "LevelGauge::TLevelGaugeMonito..."
QT_MOC_LITERAL(34, 8), // "sendHTTP"
QT_MOC_LITERAL(43, 0), // ""
QT_MOC_LITERAL(44, 4), // "data"
QT_MOC_LITERAL(49, 7), // "startLG"
QT_MOC_LITERAL(57, 8), // "finished"
QT_MOC_LITERAL(66, 5), // "start"
QT_MOC_LITERAL(72, 17), // "getTanksMeasument"
QT_MOC_LITERAL(90, 29), // "TLevelGauge::TTanksMeasuments"
QT_MOC_LITERAL(120, 14), // "tanksMeasument"
QT_MOC_LITERAL(135, 14), // "getTanksConfig"
QT_MOC_LITERAL(150, 26), // "TLevelGauge::TTanksConfigs"
QT_MOC_LITERAL(177, 10), // "tankConfig"
QT_MOC_LITERAL(188, 15), // "errorOccurredLG"
QT_MOC_LITERAL(204, 3), // "msg"
QT_MOC_LITERAL(208, 16), // "sendToHTTPServer"
QT_MOC_LITERAL(225, 13), // "getAnswerHTTP"
QT_MOC_LITERAL(239, 6), // "answer"
QT_MOC_LITERAL(246, 17) // "errorOccurredHTTP"

    },
    "LevelGauge::TLevelGaugeMonitoring\0"
    "sendHTTP\0\0data\0startLG\0finished\0start\0"
    "getTanksMeasument\0TLevelGauge::TTanksMeasuments\0"
    "tanksMeasument\0getTanksConfig\0"
    "TLevelGauge::TTanksConfigs\0tankConfig\0"
    "errorOccurredLG\0msg\0sendToHTTPServer\0"
    "getAnswerHTTP\0answer\0errorOccurredHTTP"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_LevelGauge__TLevelGaugeMonitoring[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    1,   74,    2, 0x06,    1 /* Public */,
       4,    0,   77,    2, 0x06,    3 /* Public */,
       5,    0,   78,    2, 0x06,    4 /* Public */,

 // slots: name, argc, parameters, tag, flags, initial metatype offsets
       6,    0,   79,    2, 0x0a,    5 /* Public */,
       7,    1,   80,    2, 0x08,    6 /* Private */,
      10,    1,   83,    2, 0x08,    8 /* Private */,
      13,    1,   86,    2, 0x08,   10 /* Private */,
      15,    0,   89,    2, 0x08,   12 /* Private */,
      16,    1,   90,    2, 0x08,   13 /* Private */,
      18,    1,   93,    2, 0x08,   15 /* Private */,

 // signals: parameters
    QMetaType::Void, QMetaType::QByteArray,    3,
    QMetaType::Void,
    QMetaType::Void,

 // slots: parameters
    QMetaType::Void,
    QMetaType::Void, 0x80000000 | 8,    9,
    QMetaType::Void, 0x80000000 | 11,   12,
    QMetaType::Void, QMetaType::QString,   14,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QByteArray,   17,
    QMetaType::Void, QMetaType::QString,   14,

       0        // eod
};

void LevelGauge::TLevelGaugeMonitoring::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<TLevelGaugeMonitoring *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->sendHTTP((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 1: _t->startLG(); break;
        case 2: _t->finished(); break;
        case 3: _t->start(); break;
        case 4: _t->getTanksMeasument((*reinterpret_cast< std::add_pointer_t<TLevelGauge::TTanksMeasuments>>(_a[1]))); break;
        case 5: _t->getTanksConfig((*reinterpret_cast< std::add_pointer_t<TLevelGauge::TTanksConfigs>>(_a[1]))); break;
        case 6: _t->errorOccurredLG((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 7: _t->sendToHTTPServer(); break;
        case 8: _t->getAnswerHTTP((*reinterpret_cast< std::add_pointer_t<QByteArray>>(_a[1]))); break;
        case 9: _t->errorOccurredHTTP((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (TLevelGaugeMonitoring::*)(const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TLevelGaugeMonitoring::sendHTTP)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (TLevelGaugeMonitoring::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TLevelGaugeMonitoring::startLG)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (TLevelGaugeMonitoring::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&TLevelGaugeMonitoring::finished)) {
                *result = 2;
                return;
            }
        }
    }
}

const QMetaObject LevelGauge::TLevelGaugeMonitoring::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring.offsetsAndSize,
    qt_meta_data_LevelGauge__TLevelGaugeMonitoring,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring_t
, QtPrivate::TypeAndForceComplete<TLevelGaugeMonitoring, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>
, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const TLevelGauge::TTanksMeasuments &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const TLevelGauge::TTanksConfigs &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QByteArray &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>


>,
    nullptr
} };


const QMetaObject *LevelGauge::TLevelGaugeMonitoring::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *LevelGauge::TLevelGaugeMonitoring::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_LevelGauge__TLevelGaugeMonitoring.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int LevelGauge::TLevelGaugeMonitoring::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
    return _id;
}

// SIGNAL 0
void LevelGauge::TLevelGaugeMonitoring::sendHTTP(const QByteArray & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void LevelGauge::TLevelGaugeMonitoring::startLG()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void LevelGauge::TLevelGaugeMonitoring::finished()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
