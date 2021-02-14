/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * MidiInControl
 * Copyright (C) 2021 Francisco Munoz / "Estwald"
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.+
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MidiInControl.h"
#include <QMessageBox>
#include <QThread>
#include "../midi/MidiInput.h"
#include "../midi/MidiOutput.h"
#include "../midi/MidiPlayer.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiChannel.h"
#include "../MidiEvent/ProgChangeEvent.h"
#include "../MidiEvent/ControlChangeEvent.h"
#include "../gui/InstrumentChooser.h"
#include "../gui/SoundEffectChooser.h"

extern int Bank_MIDI[17];
extern int Prog_MIDI[17];

static QSettings *_settings = NULL;
static MidiInControl *MidiIn_ctrl = NULL;
static int first = 1;
static int effect1_on = 0;
static int effect2_on = 0;
static int effect3_on = 0;
static int effect4_on = 0;
static int led_up = 0;
static int led_down = 0;

static bool _split_enable = false;
static int _note_cut = 0;
static bool _note_duo = false;
static bool _note_zero;
static int _inchannelUp;
static int _inchannelDown;
static int _channelUp;
static int _channelDown;
static bool _fixVelUp;
static bool _fixVelDown;
static bool _autoChordUp = false;
static bool _autoChordDown = false;
static bool _notes_only = false;
static bool _events_to_down;
static int _transpose_note_up = 0;
static int _transpose_note_down = 0;

static bool _key_effect = false;

static int _note_effect1;
static int _note_effect1_value;
static int _note_effect1_type;
static bool _note_effect1_usevel;
static bool _note_effect1_fkeypressed;
static int _note_effect2;
static int _note_effect2_value;
static int _note_effect2_type;
static bool _note_effect2_usevel;
static bool _note_effect2_fkeypressed;
static int _note_effect3;
static int _note_effect3_value;
static int _note_effect3_type;
static bool _note_effect3_fkeypressed;
static bool _note_effect3_usevel;
static int _note_effect4;
static int _note_effect4_value;
static int _note_effect4_type;
static bool _note_effect4_fkeypressed;
static bool _note_effect4_usevel;

static bool _skip_prgbanks;
static bool _skip_bankonly;
static bool _record_waits;

static int _current_note;

static int chordTypeUp = 2;
static int chordTypeDown = 2;
static int chordScaleVelocity3Up = 14;
static int chordScaleVelocity5Up = 15;
static int chordScaleVelocity7Up = 16;
static int chordScaleVelocity3Down = 14;
static int chordScaleVelocity5Down = 15;
static int chordScaleVelocity7Down = 16;

static MidiFile *file_live = NULL;

static const char notes[12][3]= {"C ", "C#", "D ", "D#", "E ", "F ", "F#", "G", "G#", "A ", "A#", "B"};

void MidiInControl::init_MidiInControl(QSettings *settings) {

    _settings = settings;

    _split_enable = _settings->value("MIDIin_split_enable", false).toBool();
    _note_cut = _settings->value("MIDIin_note_cut", -1).toInt();
    _note_duo = _settings->value("MIDIin_note_duo", false).toBool();
    _note_zero = _settings->value("MIDIin_note_zero", false).toBool();
    _inchannelUp = _settings->value("MIDIin_inchannelUp", -1).toInt();
    _inchannelDown = _settings->value("MIDIin_inchannelDown", -1).toInt();
    _channelUp = _settings->value("MIDIin_channelUp", -1).toInt();
    _channelDown = _settings->value("MIDIin_channelDown", -1).toInt();
    _fixVelUp = _settings->value("MIDIin_fixVelUp", false).toBool();
    _fixVelDown = _settings->value("MIDIin_fixVelDown", false).toBool();
    _autoChordUp = _settings->value("MIDIin_autoChordUp", false).toBool();
    _autoChordDown = _settings->value("MIDIin_autoChordDown", false).toBool();
    _notes_only = _settings->value("MIDIin_notes_only", true).toBool();
    _events_to_down = _settings->value("MIDIin_events_to_down", false).toBool();
    _transpose_note_up = _settings->value("MIDIin_transpose_note_up", 0).toInt();
    _transpose_note_down = _settings->value("MIDIin_transpose_note_down", 0).toInt();

    _key_effect = _settings->value("MIDIin_key_effect", false).toBool();
    _note_effect1 = _settings->value("MIDIin_note_effect1", -1).toInt();
    _note_effect1_value = _settings->value("MIDIin_note_effect1_value", 0).toInt();
    _note_effect1_type = _settings->value("MIDIin_note_effect1_type", false).toInt();
    _note_effect1_usevel = _settings->value("MIDIin_note_effect1_usevel", true).toBool();
    _note_effect1_fkeypressed = _settings->value("MIDIin_note_effect1_fkeypressed", false).toBool();
    _note_effect2 = _settings->value("MIDIin_note_effect2", -1).toInt();
    _note_effect2_value = _settings->value("MIDIin_note_effect2_value", 0).toInt();
    _note_effect2_type = _settings->value("MIDIin_note_effect2_type", false).toInt();
    _note_effect2_usevel = _settings->value("MIDIin_note_effect2_usevel", true).toBool();
    _note_effect2_fkeypressed = _settings->value("MIDIin_note_effect2_fkeypressed", false).toBool();
    _note_effect3 = _settings->value("MIDIin_note_effect3", -1).toInt();
    _note_effect3_value = _settings->value("MIDIin_note_effect3_value", 0).toInt();
    _note_effect3_type = _settings->value("MIDIin_note_effect3_type", false).toInt();
    _note_effect3_usevel = _settings->value("MIDIin_note_effect3_usevel", true).toBool();
    _note_effect3_fkeypressed = _settings->value("MIDIin_note_effect3_fkeypressed", false).toBool();
    _note_effect4 = _settings->value("MIDIin_note_effect4", -1).toInt();
    _note_effect4_value = _settings->value("MIDIin_note_effect4_value", 0).toInt();
    _note_effect4_type = _settings->value("MIDIin_note_effect4_type", false).toInt();
    _note_effect4_usevel = _settings->value("MIDIin_note_effect4_usevel", true).toBool();
    _note_effect4_fkeypressed = _settings->value("MIDIin_note_effect4_fkeypressed", false).toBool();

    _skip_prgbanks = _settings->value("MIDIin_skip_prgbanks", true).toBool();
    _skip_bankonly = _settings->value("MIDIin_skip_bankonly", true).toBool();
    _record_waits = _settings->value("MIDIin_record_waits", true).toBool();

    chordTypeUp = _settings->value("chordTypeUp", 0).toInt();
    chordScaleVelocity3Up = _settings->value("chordScaleVelocity3Up", 14).toInt();
    chordScaleVelocity5Up = _settings->value("chordScaleVelocity5Up", 15).toInt();
    chordScaleVelocity7Up = _settings->value("chordScaleVelocity7Up", 16).toInt();
    chordTypeDown = _settings->value("chordTypeDown", 0).toInt();
    chordScaleVelocity3Down = _settings->value("chordScaleVelocity3Down", 14).toInt();
    chordScaleVelocity5Down = _settings->value("chordScaleVelocity5Down", 15).toInt();
    chordScaleVelocity7Down = _settings->value("chordScaleVelocity7Down", 16).toInt();

    if(_note_duo) _note_zero = false;

    effect1_on = 0;
    effect2_on = 0;
    effect3_on = 0;
    effect4_on = 0;

    led_up = 0;
    led_down = 0;

    if(_note_effect1_type == 6 && _note_effect1_fkeypressed && _autoChordUp) effect1_on = true;
    if(_note_effect1_type == 7 && _note_effect1_fkeypressed && _autoChordDown) effect1_on  = true;
    if(_note_effect2_type == 6 && _note_effect2_fkeypressed && _autoChordUp) effect2_on = true;
    if(_note_effect2_type == 7 && _note_effect2_fkeypressed && _autoChordDown) effect2_on  = true;
    if(_note_effect3_type == 6 && _note_effect3_fkeypressed && _autoChordUp) effect3_on = true;
    if(_note_effect3_type == 7 && _note_effect3_fkeypressed && _autoChordDown) effect3_on  = true;
    if(_note_effect4_type == 6 && _note_effect4_fkeypressed && _autoChordUp) effect4_on = true;
    if(_note_effect4_type == 7 && _note_effect4_fkeypressed && _autoChordDown) effect4_on  = true;
}

void MidiInControl::paintEvent(QPaintEvent *) {
    QPainter painter(this);

    QLinearGradient linearGrad(0, 0, width(), height());

    linearGrad.setColorAt(0, QColor(0x00, 0x50, 0x70));
    linearGrad.setColorAt(0.5, QColor(0x00, 0x80, 0xa0));
    linearGrad.setColorAt(1, QColor(0x00, 0x60, 0x80));

    painter.fillRect(0, 0, width(), height(), linearGrad);
}

MidiInControl::MidiInControl(QWidget* parent): QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    MIDIin = this;

    effect1_on = 0;
    effect2_on = 0;
    effect3_on = 0;
    effect4_on = 0;
    led_up = 0;
    led_down = 0;

    if(_note_effect1_type == 6 && _note_effect1_fkeypressed && _autoChordUp) effect1_on = true;
    if(_note_effect1_type == 7 && _note_effect1_fkeypressed && _autoChordDown) effect1_on  = true;
    if(_note_effect2_type == 6 && _note_effect2_fkeypressed && _autoChordUp) effect2_on = true;
    if(_note_effect2_type == 7 && _note_effect2_fkeypressed && _autoChordDown) effect2_on  = true;
    if(_note_effect3_type == 6 && _note_effect3_fkeypressed && _autoChordUp) effect3_on = true;
    if(_note_effect3_type == 7 && _note_effect3_fkeypressed && _autoChordDown) effect3_on  = true;
    if(_note_effect4_type == 6 && _note_effect4_fkeypressed && _autoChordUp) effect4_on = true;
    if(_note_effect4_type == 7 && _note_effect4_fkeypressed && _autoChordDown) effect4_on  = true;

    _thru = MidiInput::thru();
    MidiInput::setThruEnabled(true);

    if(!MidiInput::recording()) {
        MidiPlayer::panic(); // reset
        send_live_events();  // send live event effects
        MidiInput::cleanKeyMaps();
    }

    if (MIDIin->objectName().isEmpty())
        MIDIin->setObjectName(QString::fromUtf8("MIDIin"));

    MIDIin->setFixedSize(592, 540);
    MIDIin->setWindowTitle("MIDI In Control");

    QFrame *MIDIin2 = new QFrame(MIDIin); // for heritage style sheet
    MIDIin2->setGeometry(QRect(0, 0, width(), height()));
    MIDIin2->setObjectName(QString::fromUtf8("MIDIin2"));
    MIDIin2->setStyleSheet(QString::fromUtf8("color: white;")); // background-color: #607080;\n"));


    buttonBox = new QDialogButtonBox(MIDIin2);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setStyleSheet(QString::fromUtf8("color: white; background: #8695a3; \n"));
    buttonBox->setGeometry(QRect(340, 500, 221, 32));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    SplitBox = new QGroupBox(MIDIin2);
    SplitBox->setObjectName(QString::fromUtf8("SplitBox"));

    SplitBox->setStyleSheet(QString::fromUtf8(
"QGroupBox QComboBox {color: white; background-color: #9090b3;} \n"
"QGroupBox QComboBox:disabled {color: darkGray; background-color: #8080a3;} \n"
"QGroupBox QComboBox QAbstractItemView {color: white; background-color: #9090b3; selection-background-color: #24c2c3;}"
"QGroupBox QSpinBox {color: white; background-color: #9090b3;} \n"
"QGroupBox QSpinBox:disabled {color: darkGray; background-color: #9090b3;} \n"
"QGroupBox QPushButton {color: white; background-color: #c7c9df;} \n"
"QGroupBox QToolTip {color: black;} \n"
));

    SplitBox->setGeometry(QRect(30, 20, 531, 151));
    SplitBox->setCheckable(true);
    SplitBox->setChecked(_split_enable);
    SplitBox->setTitle("Split Keyboard");
    SplitBox->setToolTip("Split the keyboard into two parts\n"
                         "or combine up to two voices.\n"
                         "Modify notes and others events to do it\n"
                         "Disable it if you want record from the\n"
                         "MIDI keyboard only");

    NoteBoxCut = new QComboBox(SplitBox);
    NoteBoxCut->setObjectName(QString::fromUtf8("NoteBoxCut"));
    NoteBoxCut->setGeometry(QRect(20, 40, 91, 31));
    NoteBoxCut->setToolTip("Record the Split note from \n"
                           "the MIDI keyboard clicking\n"
                           "'Get It' and pressing one.\n"
                             "Or combine two voices to duo.\n"
                             "C -1 or 'Get It' state disable 'Down'");
    QFont font;
    QFont font1;
    QFont font2;
    font.setPointSize(16);
    font2.setPointSize(12);
    NoteBoxCut->setFont(font);
    NoteBoxCut->addItem("Get It", -1);

    if(_note_cut >= 0) {
        NoteBoxCut->addItem(QString::asprintf("%s %i", notes[_note_cut % 12], _note_cut / 12 - 1), _note_cut);
        NoteBoxCut->setCurrentIndex(1);
    }

    NoteBoxCut->addItem("DUO", -2);

    if(_note_duo) {
        NoteBoxCut->setCurrentIndex(NoteBoxCut->count() - 1);
    }

    NoteBoxCut->addItem("C -1", 0);
    if(_note_zero) {
        NoteBoxCut->setCurrentIndex(NoteBoxCut->count() - 1);
    }

    channelBoxUp = new QComboBox(SplitBox);
    channelBoxUp->setObjectName(QString::fromUtf8("channelBoxUp"));
    channelBoxUp->setGeometry(QRect(140, 40, 81, 31));
    channelBoxUp->setToolTip("Select the MIDI Channel for 'UP'\n"
                             "'--' uses New Events channel");
    channelBoxUp->setFont(font);
    channelBoxUp->addItem("--", -1);

    for(int n = 0; n < 16; n++) {
        channelBoxUp->addItem(QString::asprintf("ch %i", n), n);
    }
    channelBoxUp->setCurrentIndex(_channelUp + 1);

    channelBoxDown = new QComboBox(SplitBox);
    channelBoxDown->setObjectName(QString::fromUtf8("channelBoxDown"));
    channelBoxDown->setGeometry(QRect(140, 100, 81, 31));
    channelBoxDown->setToolTip("Select the MIDI Channel for 'Down'\n"
                                 "'DIS' disable it using the same\n"
                                 "channel of 'Up'");
    channelBoxDown->setFont(font);
    channelBoxDown->addItem("DIS", -1);
    for(int n = 0; n < 16; n++) {
        channelBoxDown->addItem(QString::asprintf("ch %i", n), n);
    }
    channelBoxDown->setCurrentIndex(_channelDown + 1);

    labelUp = new QLabel(SplitBox);
    labelUp->setObjectName(QString::fromUtf8("labelUp"));
    labelUp->setGeometry(QRect(140, 20, 81, 20));
    labelUp->setAlignment(Qt::AlignCenter);
    labelUp->setText("Channel Up");

    labelDown = new QLabel(SplitBox);
    labelDown->setObjectName(QString::fromUtf8("labelDown"));
    labelDown->setGeometry(QRect(140, 80, 81, 20));
    labelDown->setAlignment(Qt::AlignCenter);
    labelDown->setText("Channel Down");

    labelCut = new QLabel(SplitBox);
    labelCut->setObjectName(QString::fromUtf8("labelCut"));
    labelCut->setGeometry(QRect(20, 20, 91, 20));
    labelCut->setAlignment(Qt::AlignCenter);
    labelCut->setText("Note Cut");

    tlabelUp = new QLabel(SplitBox);
    tlabelUp->setObjectName(QString::fromUtf8("tlabelUp"));
    tlabelUp->setGeometry(QRect(250, 20, 71, 20));
    tlabelUp->setAlignment(Qt::AlignCenter);
    tlabelUp->setText("Transpose");

    tlabelDown = new QLabel(SplitBox);
    tlabelDown->setObjectName(QString::fromUtf8("tlabelDown"));
    tlabelDown->setGeometry(QRect(250, 80, 71, 20));
    tlabelDown->setAlignment(Qt::AlignCenter);
    tlabelDown->setText("Transpose");

    tspinBoxUp = new QSpinBox(SplitBox);
    tspinBoxUp->setObjectName(QString::fromUtf8("tspinBoxUp"));
    tspinBoxUp->setGeometry(QRect(251, 40, 71, 31));
    tspinBoxUp->setFont(font);
    tspinBoxUp->setMinimum(-24);
    tspinBoxUp->setMaximum(24);
    tspinBoxUp->setValue(_transpose_note_up);
    tspinBoxUp->setToolTip("Transposes the position of the notes\n"
                           "from -24 to 24 (24 = 2 octaves).\n"
                            "Notes that exceed the MIDI range are clipped");

    tspinBoxDown = new QSpinBox(SplitBox);
    tspinBoxDown->setObjectName(QString::fromUtf8("tspinBoxDown"));
    tspinBoxDown->setGeometry(QRect(250, 100, 71, 31));
    tspinBoxDown->setFont(font);
    tspinBoxDown->setMinimum(-24);
    tspinBoxDown->setMaximum(24);
    tspinBoxDown->setValue(_transpose_note_down);
    tspinBoxDown->setToolTip("Transposes the position of the notes\n"
                               "from -24 to 24 (24 = 2 octaves).\n"
                                "Notes that exceed the MIDI range are clipped");

    vcheckBoxUp = new QCheckBox(SplitBox);
    vcheckBoxUp->setObjectName(QString::fromUtf8("vcheckBoxUp"));
    vcheckBoxUp->setGeometry(QRect(330, 28, 81, 33));
    vcheckBoxUp->setText("Fix Velocity");
    vcheckBoxUp->setChecked(_fixVelUp);
    vcheckBoxUp->setToolTip("Fix velocity notes to 100 (0-127)");

    vcheckBoxDown = new QCheckBox(SplitBox);
    vcheckBoxDown->setObjectName(QString::fromUtf8("vcheckBoxDown"));
    vcheckBoxDown->setGeometry(QRect(330, 88, 81, 33));
    vcheckBoxDown->setText("Fix Velocity");
    vcheckBoxDown->setChecked(_fixVelDown);
    vcheckBoxDown->setToolTip("Fix velocity notes to 100 (0-127)");

    echeckBox = new QCheckBox(SplitBox);
    echeckBox->setObjectName(QString::fromUtf8("echeckBox"));
    echeckBox->setGeometry(QRect(20, 80, 81, 31));
    echeckBox->setText("Notes Only");
    echeckBox->setChecked(_notes_only);
    echeckBox->setToolTip("Skip control events from the keyboard\n"
                           "except the Sustain Pedal event");

    echeckBoxDown = new QCheckBox(SplitBox);
    echeckBoxDown->setObjectName(QString::fromUtf8("echeckBoxDown"));
    echeckBoxDown->setGeometry(QRect(20, 110, 111, 31));
    echeckBoxDown->setText("Events to ch Down");
    echeckBoxDown->setChecked(_events_to_down);
    echeckBoxDown->setToolTip("Send the control events to the Down channel\n"
                               "In Duo mode they are sent to Up and Down channels");

    InstButtonUp = new QPushButton(SplitBox);
    InstButtonUp->setObjectName(QString::fromUtf8("InstButtonUp"));
    InstButtonUp->setGeometry(QRect(410, 40, 31, 31));
    InstButtonUp->setIcon(QIcon(":/run_environment/graphics/channelwidget/instrument.png"));
    InstButtonUp->setIconSize(QSize(24, 24));
    InstButtonUp->setToolTip("Select Live Instrument");
    InstButtonUp->setText(QString());

    InstButtonDown = new QPushButton(SplitBox);
    InstButtonDown->setObjectName(QString::fromUtf8("InstButtonDown"));
    InstButtonDown->setGeometry(QRect(410, 100, 31, 31));
    InstButtonDown->setIcon(QIcon(":/run_environment/graphics/channelwidget/instrument.png"));
    InstButtonDown->setIconSize(QSize(24, 24));
    InstButtonDown->setToolTip("Select Live Instrument");
    InstButtonDown->setText(QString());

    effectButtonUp = new QPushButton(SplitBox);
    effectButtonUp->setObjectName(QString::fromUtf8("effectButtonUp"));
    effectButtonUp->setGeometry(QRect(450, 40, 31, 31));
    effectButtonUp->setIcon(QIcon(":/run_environment/graphics/channelwidget/sound_effect.png"));
    effectButtonUp->setIconSize(QSize(24, 24));
    effectButtonUp->setText(QString());
    effectButtonUp->setToolTip("Select Sound Effects events to play in Live\n"
                               "These events will not be recorded");
    effectButtonDown = new QPushButton(SplitBox);
    effectButtonDown->setObjectName(QString::fromUtf8("effectButtonDown"));
    effectButtonDown->setGeometry(QRect(450, 100, 31, 31));
    effectButtonDown->setIcon(QIcon(":/run_environment/graphics/channelwidget/sound_effect.png"));
    effectButtonDown->setIconSize(QSize(24, 24));
    effectButtonDown->setText(QString());
    effectButtonDown->setToolTip("Select Sound Effects events to play in Live\n"
                               "These events will not be recorded");
    achordcheckBoxUp = new QCheckBox(SplitBox);
    achordcheckBoxUp->setObjectName(QString::fromUtf8("achordcheckBoxUp"));
    achordcheckBoxUp->setGeometry(QRect(330, 50, 77, 31));
    achordcheckBoxUp->setText("Auto Chord");
    achordcheckBoxUp->setChecked(_autoChordUp);
    achordcheckBoxUp->setToolTip("Select Auto Chord Mode for Up channel");
    achordcheckBoxDown = new QCheckBox(SplitBox);
    achordcheckBoxDown->setObjectName(QString::fromUtf8("achordcheckBoxDown"));
    achordcheckBoxDown->setGeometry(QRect(330, 110, 77, 31));
    achordcheckBoxDown->setText("Auto Chord");
    achordcheckBoxDown->setChecked(_autoChordDown);
    achordcheckBoxDown->setToolTip("Select Auto Chord Mode for Down channel");

    LEDBoxUp = new QCheckBox(SplitBox);
    LEDBoxUp->setObjectName(QString::fromUtf8("LEDBoxUp"));
    LEDBoxUp->setGeometry(QRect(120, 30, 16, 31));
    LEDBoxUp->setCheckable(false);
    LEDBoxUp->setText(QString());
    LEDBoxUp->setToolTip("Indicates events on channel Up");
    LEDBoxDown = new QCheckBox(SplitBox);
    LEDBoxDown->setObjectName(QString::fromUtf8("LEDBoxDown"));
    LEDBoxDown->setGeometry(QRect(120, 90, 16, 31));
    LEDBoxDown->setCheckable(false);
    LEDBoxDown->setText(QString());
    LEDBoxDown->setToolTip("Indicates events on channel Down");

    chordButtonUp = new QPushButton(SplitBox);
    chordButtonUp->setObjectName(QString::fromUtf8("chordButtonUp"));
    chordButtonUp->setGeometry(QRect(490, 40, 31, 31));
    chordButtonUp->setIcon(QIcon(":/run_environment/graphics/tool/meter.png"));
    chordButtonUp->setIconSize(QSize(24, 24));
    chordButtonUp->setText(QString());
    chordButtonUp->setToolTip("Select the chord for auto chord mode"); 
    QObject::connect(chordButtonUp, SIGNAL(clicked()), this, SLOT(setChordDialogUp()));

    chordButtonDown = new QPushButton(SplitBox);
    chordButtonDown->setObjectName(QString::fromUtf8("chordButtonDown"));
    chordButtonDown->setGeometry(QRect(490, 100, 31, 31));
    chordButtonDown->setIcon(QIcon(":/run_environment/graphics/tool/meter.png"));
    chordButtonDown->setIconSize(QSize(24, 24));
    chordButtonDown->setText(QString());
    chordButtonDown->setToolTip("Select the chord for auto chord mode");
    QObject::connect(chordButtonDown, SIGNAL(clicked()), this, SLOT(setChordDialogDown()));

    inlabelUp = new QLabel(MIDIin2);
    inlabelUp->setObjectName(QString::fromUtf8("inlabelUp"));
    inlabelUp->setGeometry(QRect(170, 170, 81, 20));
    inlabelUp->setAlignment(Qt::AlignCenter);
    inlabelUp->setText("Channel Up IN");
    inlabelDown = new QLabel(MIDIin2);
    inlabelDown->setObjectName(QString::fromUtf8("inlabelDown"));
    inlabelDown->setGeometry(QRect(280, 170, 91, 20));
    inlabelDown->setAlignment(Qt::AlignCenter);
    inlabelDown->setText("Channel Down IN");

    inchannelBoxUp = new QComboBox(MIDIin2);
    inchannelBoxUp->setObjectName(QString::fromUtf8("inchannelBoxUp"));
    inchannelBoxUp->setStyleSheet(QString::fromUtf8(
            "QComboBox {color: white; background-color: #8695a3;} \n"
            "QComboBox QAbstractItemView {color: white; background-color: #8695a3; selection-background-color: #24c2c3;}"
            "QComboBox:disabled {color: darkGray; background-color: #8695a3;} \n"
            "QToolTip {color: black;} \n"));
    inchannelBoxUp->setToolTip("Input Channel from MIDI Keyboard");
    inchannelBoxUp->setGeometry(QRect(170, 190, 81, 31));
    inchannelBoxUp->setFont(font);
    inchannelBoxUp->addItem("ALL", -1);
    for(int n = 0; n < 16; n++) {
        inchannelBoxUp->addItem(QString::asprintf("ch %i", n), n);
    }
    inchannelBoxUp->setCurrentIndex(_inchannelUp + 1);

    inchannelBoxDown = new QComboBox(MIDIin2);
    inchannelBoxDown->setObjectName(QString::fromUtf8("inchannelBoxDown"));
    inchannelBoxDown->setStyleSheet(QString::fromUtf8(
        "QComboBox {color: white; background-color: #8695a3} \n"
        "QComboBox QAbstractItemView {color: white; background-color: #8695a3; selection-background-color: #24c2c3;}"
        "QComboBox:disabled {color: darkGray; background-color: #8695a3;} \n"
        "QToolTip {color: black;} \n"));
    inchannelBoxDown->setToolTip("Input Channel from MIDI Keyboard");
    inchannelBoxDown->setGeometry(QRect(280, 190, 81, 31));
    inchannelBoxDown->setFont(font);
    inchannelBoxDown->addItem("ALL", -1);
    for(int n = 0; n < 16; n++) {
        inchannelBoxDown->addItem(QString::asprintf("ch %i", n), n);
    }
    inchannelBoxDown->setCurrentIndex(_inchannelDown + 1);


    rstButton = new QPushButton(MIDIin2);
    rstButton->setObjectName(QString::fromUtf8("rstButton"));
    rstButton->setFont(font2);
    rstButton->setStyleSheet(QString::fromUtf8(
            "QPushButton {color: black; background-color: #c7c9df;} \n"
            "QPushButton::disabled { color: gray;}"
            "QToolTip {color: black;} \n"));
    rstButton->setGeometry(QRect(440, 190, 51, 31));
    rstButton->setToolTip("Reset Parameters");
    rstButton->setText("Reset");
    rstButton->setToolTip("Deletes the Split parameters\n"
                           "and use settings by default");

    PanicButton = new QPushButton(MIDIin2);
    PanicButton->setObjectName(QString::fromUtf8("PanicButton"));
    PanicButton->setGeometry(QRect(510, 190, 51, 31));
    PanicButton->setStyleSheet(QString::fromUtf8(
        "QPushButton {color: white; background-color: #c7c9df;} \n"
        "QToolTip {color: black;} \n"));
    PanicButton->setIcon(QIcon(":/run_environment/graphics/tool/panic.png"));
    PanicButton->setIconSize(QSize(24, 24));
    PanicButton->setToolTip("Panic Button\nReset MIDI Output");
    PanicButton->setText(QString());
    PanicButton->setToolTip("MIDI Panic extended");

// effects

    groupBoxEffect = new QGroupBox(MIDIin2);
    groupBoxEffect->setObjectName(QString::fromUtf8("groupBoxEffect"));
    groupBoxEffect->setStyleSheet(QString::fromUtf8(
    "QGroupBox QComboBox {color: white; background-color: #60b386;} \n"
    "QGroupBox QComboBox:disabled {color: darkGray; background-color: #50a376;} \n"
    "QGroupBox QComboBox QAbstractItemView {color: white; background-color: #60b386; selection-background-color: #24c2c3;}"
    "QGroupBox QSpinBox {color: white; background-color: #60b386;} \n"
    "QGroupBox QPushButton {color: white; background-color: #c7c9df;} \n"
    "QGroupBox QToolTip {color: black;} \n"
    ));
    groupBoxEffect->setGeometry(QRect(30, 220, 531, 261));
    groupBoxEffect->setCheckable(true);
    groupBoxEffect->setChecked(_key_effect);
    groupBoxEffect->setTitle("Key Effects");
    groupBoxEffect->setToolTip("Key effect is used to connect MIDI keyboard keys\n"
                                "to generate effects like sustain, pitch bend and\n"
                                "other applications.\n"
                                "Note Effect1 has the priority and can be activated\n"
                                "simultaneously by an expression pedal (sustain)");


    int xx = 20, yy = 20;
    NoteBoxEffect1 = new QComboBox(groupBoxEffect);
    NoteBoxEffect1->setObjectName(QString::fromUtf8("NoteBoxEffect1"));
    NoteBoxEffect1->setGeometry(QRect(xx, yy + 20, 91, 31));
    NoteBoxEffect1->setFont(font);
    NoteBoxEffect1->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one.\n"
                               "Expression pedal (sustain) works here");

    NoteBoxEffect1->addItem("Get It", -1);
    if(_note_effect1 >= 0) {
        NoteBoxEffect1->addItem(QString::asprintf("%s %i", notes[_note_effect1 % 12], _note_effect1 / 12 - 1), _note_effect1);
        NoteBoxEffect1->setCurrentIndex(1);
    }

    labelPitch1 = new QLabel(groupBoxEffect);
    labelPitch1->setObjectName(QString::fromUtf8("labelPitch1"));
    labelPitch1->setGeometry(QRect(xx + 10, yy + 62, 101, 16));
    labelPitch1->setText("Pitch Bend");

    typeBoxEffect1 = new QComboBox(groupBoxEffect);
    typeBoxEffect1->setObjectName(QString::fromUtf8("typeBoxEffect1"));
    typeBoxEffect1->setGeometry(QRect(xx + 100, yy + 20, 131, 31));
    typeBoxEffect1->setToolTip("Select the effect to apply here");

    font1.setPointSize(10);
    typeBoxEffect1->setFont(font1);
    typeBoxEffect1->addItem("Pitch Bend", 2);
    typeBoxEffect1->addItem("Modulation Wheel", 1);
    typeBoxEffect1->addItem("Sustain", 0);
    typeBoxEffect1->addItem("Sostenuto", 0);
    typeBoxEffect1->addItem("Reverb Level", 1);
    typeBoxEffect1->addItem("Chorus Level", 1);
    typeBoxEffect1->addItem("AutoChord Up", 0);
    typeBoxEffect1->addItem("AutoChord Down", 0);
    typeBoxEffect1->setCurrentIndex(-1);
    QObject::connect(typeBoxEffect1, SIGNAL(currentIndexChanged(QString)), labelPitch1, SLOT(setText(QString)));

    useVelocityBoxEffect1 = new QCheckBox(groupBoxEffect);
    useVelocityBoxEffect1->setObjectName(QString::fromUtf8("useVelocityBoxEffect1"));
    useVelocityBoxEffect1->setGeometry(QRect(xx + 100, yy, 91, 20));
    useVelocityBoxEffect1->setText("Use Velocity");
    useVelocityBoxEffect1->setChecked(_note_effect1_usevel);
    useVelocityBoxEffect1->setToolTip("Use the velocity from the note as value\n"
                                       "For pressure sensitive MIDI keyboards");
    QObject::connect(useVelocityBoxEffect1, SIGNAL(clicked(bool)), this, SLOT(set_note_effect1_usevel(bool)));

    pressedBoxEffect1 = new QCheckBox(groupBoxEffect);
    pressedBoxEffect1->setObjectName(QString::fromUtf8("pressedBoxEffect1"));
    pressedBoxEffect1->setGeometry(QRect(xx + 120, yy + 51, 91, 31));
    pressedBoxEffect1->setText("key pressed");
    pressedBoxEffect1->setChecked(_note_effect1_fkeypressed);
    pressedBoxEffect1->setToolTip("two working modes here: without checking,\n"
                                  "the effect starts pressing the key and ends\n"
                                  "when it is released.\n"
                                  "When it is checked, pressing the key activates\n"
                                  "the effect and another pressing stops it");

    LEDBoxEffect1 = new QCheckBox(groupBoxEffect);
    LEDBoxEffect1->setObjectName(QString::fromUtf8("LEDBoxEffect1"));
    LEDBoxEffect1->setGeometry(QRect(xx + 100, yy + 51, 16, 31));
    LEDBoxEffect1->setCheckable(false);
    LEDBoxEffect1->setText(QString());
    LEDBoxEffect1->setToolTip("Operation indicator");

    labelEffect1 = new QLabel(groupBoxEffect);
    labelEffect1->setObjectName(QString::fromUtf8("labelEffect1"));
    labelEffect1->setGeometry(QRect(xx, yy, 91, 20));
    labelEffect1->setAlignment(Qt::AlignCenter);
    labelEffect1->setText("Note Effect1");
    labelEffect1->setStyleSheet(QString::fromUtf8("background-color: #40a080"));
    labelEffect1->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one.\n"
                               "Expression pedal (sustain) works here");

    VlabelPitch1 = new QLabel(groupBoxEffect);
    VlabelPitch1->setObjectName(QString::fromUtf8("VlabelPitch1"));
    VlabelPitch1->setGeometry(QRect(xx + 180, yy + 80, 21, 16));
    VlabelPitch1->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));
    VlabelPitch1->setAlignment(Qt::AlignCenter);
    VlabelPitch1->setText(QString());

    horizontalSliderPitch1 = new QSlider(groupBoxEffect);
    horizontalSliderPitch1->setObjectName(QString::fromUtf8("horizontalSliderPitch1"));
    horizontalSliderPitch1->setGeometry(QRect(xx, yy + 80, 171, 22));
    horizontalSliderPitch1->setMinimum(-99);
    horizontalSliderPitch1->setOrientation(Qt::Horizontal);
    horizontalSliderPitch1->setTickPosition(QSlider::TicksAbove);
    horizontalSliderPitch1->setTickInterval(99);
    horizontalSliderPitch1->setValue(-1);
    QObject::connect(horizontalSliderPitch1, SIGNAL(valueChanged(int)), VlabelPitch1, SLOT(setNum(int)));
    horizontalSliderPitch1->setValue(_note_effect1_value);
    horizontalSliderPitch1->setToolTip("Effect value to setting. When key\n"
                                       "velocity is used, this is the scale value\n"
                                        "to fix the range");

    connect(typeBoxEffect1, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        int type = typeBoxEffect1->itemData(v).toInt();
        if(type == 2) {
            horizontalSliderPitch1->setDisabled(false);
            horizontalSliderPitch1->setMinimum(-99);
            horizontalSliderPitch1->setMaximum(99);
            horizontalSliderPitch1->setTickInterval(99);
            if(!first) _note_effect1_value = 0;
        } else if(type == 1) {
            horizontalSliderPitch1->setDisabled(false);
            horizontalSliderPitch1->setMinimum(0);
            horizontalSliderPitch1->setMaximum(127);
            horizontalSliderPitch1->setTickInterval(64);
            if(!first) _note_effect1_value = 64;
        } else if(type == 0) {
            horizontalSliderPitch1->setDisabled(true);
        }

        set_note_effect1_type(v);
        horizontalSliderPitch1->setValue(_note_effect1_value);
        _settings->setValue("MIDIin_note_effect1_value", _note_effect1_value);

    });

    typeBoxEffect1->setCurrentIndex(_note_effect1_type);

    connect(horizontalSliderPitch1, QOverload<int>::of(&QSlider::valueChanged), [=](int i)
    {
        _note_effect1_value = i;
        horizontalSliderPitch1->setValue(_note_effect1_value);
        _settings->setValue("MIDIin_note_effect1_value", _note_effect1_value);
    });

    QObject::connect(pressedBoxEffect1, SIGNAL(clicked(bool)), this, SLOT(set_note_effect1_fkeypressed(bool)));

    xx = 290;
    NoteBoxEffect2 = new QComboBox(groupBoxEffect);
    NoteBoxEffect2->setObjectName(QString::fromUtf8("NoteBoxEffect2"));
    NoteBoxEffect2->setGeometry(QRect(xx, yy + 20, 91, 31));
    NoteBoxEffect2->setFont(font);
    NoteBoxEffect2->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one");
    NoteBoxEffect2->addItem("Get It", -1);
    if(_note_effect2 >= 0) {
        NoteBoxEffect2->addItem(QString::asprintf("%s %i", notes[_note_effect2 % 12], _note_effect2 / 12 - 1), _note_effect2);
        NoteBoxEffect2->setCurrentIndex(1);
    }

    labelPitch2 = new QLabel(groupBoxEffect);
    labelPitch2->setObjectName(QString::fromUtf8("labelPitch2"));
    labelPitch2->setGeometry(QRect(xx + 10, yy + 62, 101, 16));
    labelPitch2->setText("Pitch Bend");

    typeBoxEffect2 = new QComboBox(groupBoxEffect);
    typeBoxEffect2->setObjectName(QString::fromUtf8("typeBoxEffect2"));
    typeBoxEffect2->setGeometry(QRect(xx + 100, yy + 20, 131, 31));
    typeBoxEffect2->setToolTip("Select the effect to apply here");
    typeBoxEffect2->setFont(font1);
    typeBoxEffect2->addItem("Pitch Bend", 2);
    typeBoxEffect2->addItem("Modulation Wheel", 1);
    typeBoxEffect2->addItem("Sustain", 0);
    typeBoxEffect2->addItem("Sostenuto", 0);
    typeBoxEffect2->addItem("Reverb Level", 1);
    typeBoxEffect2->addItem("Chorus Level", 1);
    typeBoxEffect2->addItem("AutoChord Up", 0);
    typeBoxEffect2->addItem("AutoChord Down", 0);
    typeBoxEffect2->setCurrentIndex(-1);
    QObject::connect(typeBoxEffect2, SIGNAL(currentIndexChanged(QString)), labelPitch2, SLOT(setText(QString)));

    useVelocityBoxEffect2 = new QCheckBox(groupBoxEffect);
    useVelocityBoxEffect2->setObjectName(QString::fromUtf8("useVelocityBoxEffect2"));
    useVelocityBoxEffect2->setGeometry(QRect(xx + 100, yy, 91, 20));
    useVelocityBoxEffect2->setText("Use Velocity");
    useVelocityBoxEffect2->setChecked(_note_effect2_usevel);
    useVelocityBoxEffect2->setToolTip("Use the velocity from the note as value\n"
                                       "For pressure sensitive MIDI keyboards");
    QObject::connect(useVelocityBoxEffect2, SIGNAL(clicked(bool)), this, SLOT(set_note_effect2_usevel(bool)));

    pressedBoxEffect2 = new QCheckBox(groupBoxEffect);
    pressedBoxEffect2->setObjectName(QString::fromUtf8("pressedBoxEffect2"));
    pressedBoxEffect2->setGeometry(QRect(xx + 120, yy + 51, 91, 31));
    pressedBoxEffect2->setText("key pressed");
    pressedBoxEffect2->setChecked(_note_effect2_fkeypressed);
    pressedBoxEffect2->setToolTip("two working modes here: without checking,\n"
                                  "the effect starts pressing the key and ends\n"
                                  "when it is released.\n"
                                  "When it is checked, pressing the key activates\n"
                                  "the effect and another pressing stops it");

    LEDBoxEffect2 = new QCheckBox(groupBoxEffect);
    LEDBoxEffect2->setObjectName(QString::fromUtf8("LEDBoxEffect2"));
    LEDBoxEffect2->setGeometry(QRect(xx + 100, yy + 51, 16, 31));
    LEDBoxEffect2->setCheckable(false);
    LEDBoxEffect2->setText(QString());
    LEDBoxEffect2->setToolTip("Operation indicator");

    labelEffect2 = new QLabel(groupBoxEffect);
    labelEffect2->setObjectName(QString::fromUtf8("labelEffect2"));
    labelEffect2->setGeometry(QRect(xx, yy, 91, 20));
    labelEffect2->setAlignment(Qt::AlignCenter);
    labelEffect2->setText("Note Effect2");
    labelEffect2->setStyleSheet(QString::fromUtf8("background-color: #40404040"));
    labelEffect2->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one");

    VlabelPitch2 = new QLabel(groupBoxEffect);
    VlabelPitch2->setObjectName(QString::fromUtf8("VlabelPitch2"));
    VlabelPitch2->setGeometry(QRect(xx + 180, yy + 80, 21, 16));
    VlabelPitch2->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));
    VlabelPitch2->setAlignment(Qt::AlignCenter);
    VlabelPitch2->setText(QString());

    horizontalSliderPitch2 = new QSlider(groupBoxEffect);
    horizontalSliderPitch2->setObjectName(QString::fromUtf8("horizontalSliderPitch2"));
    horizontalSliderPitch2->setGeometry(QRect(xx, yy + 80, 171, 22));
    horizontalSliderPitch2->setMinimum(-99);
    horizontalSliderPitch2->setOrientation(Qt::Horizontal);
    horizontalSliderPitch2->setTickPosition(QSlider::TicksAbove);
    horizontalSliderPitch2->setTickInterval(99);
    horizontalSliderPitch2->setValue(1);
    QObject::connect(horizontalSliderPitch2, SIGNAL(valueChanged(int)), VlabelPitch2, SLOT(setNum(int)));
    horizontalSliderPitch2->setValue(_note_effect2_value);
    horizontalSliderPitch2->setToolTip("Effect value to setting. When key\n"
                                       "velocity is used, this is the scale value\n"
                                        "to fix the range");

    connect(typeBoxEffect2, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        int type = typeBoxEffect2->itemData(v).toInt();
        if(type == 2) {
            horizontalSliderPitch2->setDisabled(false);
            horizontalSliderPitch2->setMinimum(-99);
            horizontalSliderPitch2->setMaximum(99);
            horizontalSliderPitch2->setTickInterval(99);
            if(!first) _note_effect2_value = 0;
        } else if(type == 1) {
            horizontalSliderPitch2->setDisabled(false);
            horizontalSliderPitch2->setMinimum(0);
            horizontalSliderPitch2->setMaximum(127);
            horizontalSliderPitch2->setTickInterval(64);
            if(!first) _note_effect2_value = 64;
        } else if(type == 0) {
            horizontalSliderPitch2->setDisabled(true);
        }

        set_note_effect2_type(v);
        horizontalSliderPitch2->setValue(_note_effect2_value);
        _settings->setValue("MIDIin_note_effect2_value", _note_effect2_value);

    });

    typeBoxEffect2->setCurrentIndex(_note_effect2_type);

    connect(horizontalSliderPitch2, QOverload<int>::of(&QSlider::valueChanged), [=](int i)
    {
        _note_effect2_value = i;
        horizontalSliderPitch2->setValue(_note_effect2_value);
        _settings->setValue("MIDIin_note_effect2_value", _note_effect2_value);
    });

    QObject::connect(pressedBoxEffect2, SIGNAL(clicked(bool)), this, SLOT(set_note_effect2_fkeypressed(bool)));

    xx = 20; yy = 150;
    NoteBoxEffect3 = new QComboBox(groupBoxEffect);
    NoteBoxEffect3->setObjectName(QString::fromUtf8("NoteBoxEffect3"));
    NoteBoxEffect3->setGeometry(QRect(xx, yy + 20, 91, 31));
    NoteBoxEffect3->setFont(font);
    NoteBoxEffect3->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one");
    NoteBoxEffect3->addItem("Get It", -1);
    if(_note_effect3 >= 0) {
        NoteBoxEffect3->addItem(QString::asprintf("%s %i", notes[_note_effect3 % 12], _note_effect3 / 12 - 1), _note_effect3);
        NoteBoxEffect3->setCurrentIndex(1);
    }

    labelPitch3 = new QLabel(groupBoxEffect);
    labelPitch3->setObjectName(QString::fromUtf8("labelPitch3"));
    labelPitch3->setGeometry(QRect(xx + 10, yy + 62, 101, 16));
    labelPitch3->setText("Pitch Bend");

    typeBoxEffect3 = new QComboBox(groupBoxEffect);
    typeBoxEffect3->setObjectName(QString::fromUtf8("typeBoxEffect3"));
    typeBoxEffect3->setGeometry(QRect(xx + 100, yy + 20, 131, 31));
    typeBoxEffect3->setToolTip("Select the effect to apply here");
    typeBoxEffect3->setFont(font1);
    typeBoxEffect3->addItem("Pitch Bend", 2);
    typeBoxEffect3->addItem("Modulation Wheel", 1);
    typeBoxEffect3->addItem("Sustain", 0);
    typeBoxEffect3->addItem("Sostenuto", 0);
    typeBoxEffect3->addItem("Reverb Level", 1);
    typeBoxEffect3->addItem("Chorus Level", 1);
    typeBoxEffect3->addItem("AutoChord Up", 0);
    typeBoxEffect3->addItem("AutoChord Down", 0);
    typeBoxEffect3->setCurrentIndex(-1);
    QObject::connect(typeBoxEffect3, SIGNAL(currentIndexChanged(QString)), labelPitch3, SLOT(setText(QString)));

    useVelocityBoxEffect3 = new QCheckBox(groupBoxEffect);
    useVelocityBoxEffect3->setObjectName(QString::fromUtf8("useVelocityBoxEffect3"));
    useVelocityBoxEffect3->setGeometry(QRect(xx + 100, yy, 91, 20));
    useVelocityBoxEffect3->setText("Use Velocity");
    useVelocityBoxEffect3->setChecked(_note_effect3_usevel);
    useVelocityBoxEffect3->setToolTip("Use the velocity from the note as value\n"
                                       "For pressure sensitive MIDI keyboards");
    QObject::connect(useVelocityBoxEffect3, SIGNAL(clicked(bool)), this, SLOT(set_note_effect3_usevel(bool)));

    pressedBoxEffect3 = new QCheckBox(groupBoxEffect);
    pressedBoxEffect3->setObjectName(QString::fromUtf8("pressedBoxEffect3"));
    pressedBoxEffect3->setGeometry(QRect(xx + 120, yy + 51, 91, 31));
    pressedBoxEffect3->setText("key pressed");
    pressedBoxEffect3->setChecked(_note_effect3_fkeypressed);
    pressedBoxEffect3->setToolTip("two working modes here: without checking,\n"
                                  "the effect starts pressing the key and ends\n"
                                  "when it is released.\n"
                                  "When it is checked, pressing the key activates\n"
                                  "the effect and another pressing stops it");

    LEDBoxEffect3 = new QCheckBox(groupBoxEffect);
    LEDBoxEffect3->setObjectName(QString::fromUtf8("LEDBoxEffect3"));
    LEDBoxEffect3->setGeometry(QRect(xx + 100, yy + 51, 16, 31));
    LEDBoxEffect3->setCheckable(false);
    LEDBoxEffect3->setText(QString());
    LEDBoxEffect3->setToolTip("Operation indicator");

    labelEffect3 = new QLabel(groupBoxEffect);
    labelEffect3->setObjectName(QString::fromUtf8("labelEffect3"));
    labelEffect3->setGeometry(QRect(xx, yy, 91, 20));
    labelEffect3->setAlignment(Qt::AlignCenter);
    labelEffect3->setText("Note Effect3");
    labelEffect3->setStyleSheet(QString::fromUtf8("background-color: #40404040"));
    labelEffect3->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one");

    VlabelPitch3 = new QLabel(groupBoxEffect);
    VlabelPitch3->setObjectName(QString::fromUtf8("VlabelPitch3"));
    VlabelPitch3->setGeometry(QRect(xx + 180, yy + 80, 21, 16));
    VlabelPitch3->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));
    VlabelPitch3->setAlignment(Qt::AlignCenter);
    VlabelPitch3->setText(QString());

    horizontalSliderPitch3 = new QSlider(groupBoxEffect);
    horizontalSliderPitch3->setObjectName(QString::fromUtf8("horizontalSliderPitch3"));
    horizontalSliderPitch3->setGeometry(QRect(xx, yy + 80, 171, 22));
    horizontalSliderPitch3->setMinimum(-99);
    horizontalSliderPitch3->setOrientation(Qt::Horizontal);
    horizontalSliderPitch3->setTickPosition(QSlider::TicksAbove);
    horizontalSliderPitch3->setTickInterval(99);
    horizontalSliderPitch3->setValue(1);
    QObject::connect(horizontalSliderPitch3, SIGNAL(valueChanged(int)), VlabelPitch3, SLOT(setNum(int)));
    horizontalSliderPitch3->setValue(_note_effect3_value);
    horizontalSliderPitch3->setToolTip("Effect value to setting. When key\n"
                                       "velocity is used, this is the scale value\n"
                                        "to fix the range");

    connect(typeBoxEffect3, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        int type = typeBoxEffect3->itemData(v).toInt();
        if(type == 2) {
            horizontalSliderPitch3->setDisabled(false);
            horizontalSliderPitch3->setMinimum(-99);
            horizontalSliderPitch3->setMaximum(99);
            horizontalSliderPitch3->setTickInterval(99);
            if(!first) _note_effect3_value = 0;
        } else if(type == 1) {
            horizontalSliderPitch3->setDisabled(false);
            horizontalSliderPitch3->setMinimum(0);
            horizontalSliderPitch3->setMaximum(127);
            horizontalSliderPitch3->setTickInterval(64);
            if(!first) _note_effect3_value = 64;
        } else if(type == 0) {
            horizontalSliderPitch3->setDisabled(true);
        }

        set_note_effect3_type(v);
        horizontalSliderPitch3->setValue(_note_effect3_value);
        _settings->setValue("MIDIin_note_effect3_value", _note_effect3_value);

    });

    typeBoxEffect3->setCurrentIndex(_note_effect3_type);

    connect(horizontalSliderPitch3, QOverload<int>::of(&QSlider::valueChanged), [=](int i)
    {
        _note_effect3_value = i;
        horizontalSliderPitch3->setValue(_note_effect3_value);
        _settings->setValue("MIDIin_note_effect3_value", _note_effect3_value);
    });

    QObject::connect(pressedBoxEffect3, SIGNAL(clicked(bool)), this, SLOT(set_note_effect3_fkeypressed(bool)));

    xx = 290;
    NoteBoxEffect4 = new QComboBox(groupBoxEffect);
    NoteBoxEffect4->setObjectName(QString::fromUtf8("NoteBoxEffect4"));
    NoteBoxEffect4->setGeometry(QRect(xx, yy + 20, 91, 31));
    NoteBoxEffect4->setFont(font);
    NoteBoxEffect4->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one");
    NoteBoxEffect4->addItem("Get It", -1);
    if(_note_effect4 >= 0) {
        NoteBoxEffect4->addItem(QString::asprintf("%s %i", notes[_note_effect4 % 12], _note_effect4 / 12 - 1), _note_effect4);
        NoteBoxEffect4->setCurrentIndex(1);
    }

    labelPitch4 = new QLabel(groupBoxEffect);
    labelPitch4->setObjectName(QString::fromUtf8("labelPitch4"));
    labelPitch4->setGeometry(QRect(xx + 10, yy + 62, 101, 16));
    labelPitch4->setText("Pitch Bend");

    typeBoxEffect4 = new QComboBox(groupBoxEffect);
    typeBoxEffect4->setObjectName(QString::fromUtf8("typeBoxEffect4"));
    typeBoxEffect4->setGeometry(QRect(xx + 100, yy + 20, 131, 31));
    typeBoxEffect4->setToolTip("Select the effect to apply here");
    typeBoxEffect4->setFont(font1);
    typeBoxEffect4->addItem("Pitch Bend", 2);
    typeBoxEffect4->addItem("Modulation Wheel", 1);
    typeBoxEffect4->addItem("Sustain", 0);
    typeBoxEffect4->addItem("Sostenuto", 0);
    typeBoxEffect4->addItem("Reverb Level", 1);
    typeBoxEffect4->addItem("Chorus Level", 1);
    typeBoxEffect4->addItem("AutoChord Up", 0);
    typeBoxEffect4->addItem("AutoChord Down", 0);
    typeBoxEffect4->setCurrentIndex(-1);
    QObject::connect(typeBoxEffect4, SIGNAL(currentIndexChanged(QString)), labelPitch4, SLOT(setText(QString)));

    useVelocityBoxEffect4 = new QCheckBox(groupBoxEffect);
    useVelocityBoxEffect4->setObjectName(QString::fromUtf8("useVelocityBoxEffect4"));
    useVelocityBoxEffect4->setGeometry(QRect(xx + 100, yy, 91, 20));
    useVelocityBoxEffect4->setText("Use Velocity");
    useVelocityBoxEffect4->setChecked(_note_effect4_usevel);
    useVelocityBoxEffect4->setToolTip("Use the velocity from the note as value\n"
                                       "For pressure sensitive MIDI keyboards");

    QObject::connect(useVelocityBoxEffect4, SIGNAL(clicked(bool)), this, SLOT(set_note_effect4_usevel(bool)));

    pressedBoxEffect4 = new QCheckBox(groupBoxEffect);
    pressedBoxEffect4->setObjectName(QString::fromUtf8("pressedBoxEffect4"));
    pressedBoxEffect4->setGeometry(QRect(xx + 120, yy + 51, 91, 31));
    pressedBoxEffect4->setText("key pressed");
    pressedBoxEffect4->setChecked(_note_effect4_fkeypressed);
    pressedBoxEffect4->setToolTip("two working modes here: without checking,\n"
                                  "the effect starts pressing the key and ends\n"
                                  "when it is released.\n"
                                  "When it is checked, pressing the key activates\n"
                                  "the effect and another pressing stops it");

    LEDBoxEffect4 = new QCheckBox(groupBoxEffect);
    LEDBoxEffect4->setObjectName(QString::fromUtf8("LEDBoxEffect4"));
    LEDBoxEffect4->setGeometry(QRect(xx + 100, yy + 51, 16, 31));
    LEDBoxEffect4->setCheckable(false);
    LEDBoxEffect4->setText(QString());
    LEDBoxEffect4->setToolTip("Operation indicator");

    labelEffect4 = new QLabel(groupBoxEffect);
    labelEffect4->setObjectName(QString::fromUtf8("labelEffect4"));
    labelEffect4->setGeometry(QRect(xx, yy, 91, 20));
    labelEffect4->setAlignment(Qt::AlignCenter);
    labelEffect4->setText("Note Effect4");
    labelEffect4->setStyleSheet(QString::fromUtf8("background-color: #40404040"));
    labelEffect4->setToolTip("Record the activation note from \n"
                               "the MIDI keyboard clicking\n"
                               "'Get It' and pressing one");

    VlabelPitch4 = new QLabel(groupBoxEffect);
    VlabelPitch4->setObjectName(QString::fromUtf8("VlabelPitch4"));
    VlabelPitch4->setGeometry(QRect(xx + 180, yy + 80, 21, 16));
    VlabelPitch4->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));
    VlabelPitch4->setAlignment(Qt::AlignCenter);
    VlabelPitch4->setText(QString());

    horizontalSliderPitch4 = new QSlider(groupBoxEffect);
    horizontalSliderPitch4->setObjectName(QString::fromUtf8("horizontalSliderPitch4"));
    horizontalSliderPitch4->setGeometry(QRect(xx, yy + 80, 171, 22));
    horizontalSliderPitch4->setMinimum(-99);
    horizontalSliderPitch4->setOrientation(Qt::Horizontal);
    horizontalSliderPitch4->setTickPosition(QSlider::TicksAbove);
    horizontalSliderPitch4->setTickInterval(99);
    horizontalSliderPitch4->setValue(1);
    QObject::connect(horizontalSliderPitch4, SIGNAL(valueChanged(int)), VlabelPitch4, SLOT(setNum(int)));
    horizontalSliderPitch4->setValue(_note_effect4_value);
    horizontalSliderPitch4->setToolTip("Effect value to setting. When key\n"
                                       "velocity is used, this is the scale value\n"
                                        "to fix the range");

    connect(typeBoxEffect4, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        int type = typeBoxEffect4->itemData(v).toInt();
        if(type == 2) {
            horizontalSliderPitch4->setDisabled(false);
            horizontalSliderPitch4->setMinimum(-99);
            horizontalSliderPitch4->setMaximum(99);
            horizontalSliderPitch4->setTickInterval(99);
            if(!first) _note_effect4_value = 0;
        } else if(type == 1) {
            horizontalSliderPitch4->setDisabled(false);
            horizontalSliderPitch4->setMinimum(0);
            horizontalSliderPitch4->setMaximum(127);
            horizontalSliderPitch4->setTickInterval(64);
            if(!first) _note_effect4_value = 64;
        } else if(type == 0) {
            horizontalSliderPitch4->setDisabled(true);
        }

        set_note_effect4_type(v);
        horizontalSliderPitch4->setValue(_note_effect4_value);
        _settings->setValue("MIDIin_note_effect4_value", _note_effect4_value);

    });

    typeBoxEffect4->setCurrentIndex(_note_effect4_type);

    connect(horizontalSliderPitch4, QOverload<int>::of(&QSlider::valueChanged), [=](int i)
    {
        _note_effect4_value = i;
        horizontalSliderPitch4->setValue(_note_effect4_value);
        _settings->setValue("MIDIin_note_effect4_value", _note_effect4_value);
    });

    QObject::connect(pressedBoxEffect4, SIGNAL(clicked(bool)), this, SLOT(set_note_effect4_fkeypressed(bool)));


    checkBoxPrgBank = new QCheckBox(MIDIin2);
    checkBoxPrgBank->setObjectName(QString::fromUtf8("checkBoxPrgBank"));
    checkBoxPrgBank->setGeometry(QRect(30, 490, 121, 17));
    checkBoxPrgBank->setStyleSheet(QString::fromUtf8("QToolTip {color: black;} \n"));
    checkBoxPrgBank->setChecked(_skip_prgbanks);
    checkBoxPrgBank->setText("Skip Prg/Bank Events");
    checkBoxPrgBank->setToolTip("Skip Prg/Bank Events from the MIDI keyboard\n"
                                "and instead use the 'Split Keyboard' instruments\n"
                                "or from 'New Events' channel");

    bankskipcheckBox = new QCheckBox(MIDIin2);
    bankskipcheckBox->setObjectName(QString::fromUtf8("bankskipcheckBox"));
    bankskipcheckBox->setGeometry(QRect(160, 490, 101, 17));
    bankskipcheckBox->setStyleSheet(QString::fromUtf8("QToolTip {color: black;} \n"));
    bankskipcheckBox->setChecked(_skip_bankonly);
    bankskipcheckBox->setText("Skip Bank Only");
    bankskipcheckBox->setToolTip("ignore the bank change event coming from the\n"
                                 "MIDI keyboard and use bank 0 instead\n"
                                 "Useful if your MIDI keyboard is compatible with\n"
                                 "the General Midi list");

    checkBoxWait = new QCheckBox(MIDIin2);
    checkBoxWait->setObjectName(QString::fromUtf8("checkBoxWait"));
    checkBoxWait->setStyleSheet(QString::fromUtf8("QToolTip {color: black;} \n"));
    checkBoxWait->setGeometry(QRect(30, 510, 241, 17));
    checkBoxWait->setChecked(_record_waits);
    checkBoxWait->setText("Recording starts when one key is pressed");
    checkBoxWait->setToolTip("Waits for a key pressed on the MIDI keyboard\n"
                             "to start the recording");

    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    QObject::connect(SplitBox, SIGNAL(clicked(bool)), this, SLOT(set_split_enable(bool)));
    QObject::connect(inchannelBoxUp, SIGNAL(activated(int)), this, SLOT(set_inchannelUp(int)));
    QObject::connect(inchannelBoxDown, SIGNAL(activated(int)), this, SLOT(set_inchannelDown(int)));
    QObject::connect(channelBoxUp, SIGNAL(activated(int)), this, SLOT(set_channelUp(int)));
    QObject::connect(channelBoxDown, SIGNAL(activated(int)), this, SLOT(set_channelDown(int)));
    QObject::connect(vcheckBoxUp, SIGNAL(clicked(bool)), this, SLOT(set_fixVelUp(bool)));
    QObject::connect(vcheckBoxDown, SIGNAL(clicked(bool)), this, SLOT(set_fixVelDown(bool)));
    QObject::connect(achordcheckBoxUp, SIGNAL(clicked(bool)), this, SLOT(set_autoChordUp(bool)));
    QObject::connect(achordcheckBoxDown, SIGNAL(clicked(bool)), this, SLOT(set_autoChordDown(bool)));
    QObject::connect(echeckBox, SIGNAL(clicked(bool)), this, SLOT(set_notes_only(bool)));
    QObject::connect(echeckBoxDown, SIGNAL(clicked(bool)), this, SLOT(set_events_to_down(bool)));

    QObject::connect(tspinBoxUp, SIGNAL(valueChanged(int)), this, SLOT(set_transpose_note_up(int)));
    QObject::connect(tspinBoxDown, SIGNAL(valueChanged(int)), this, SLOT(set_transpose_note_down(int)));

    QObject::connect(rstButton, SIGNAL(clicked()), this, SLOT(split_reset()));
    QObject::connect(PanicButton, SIGNAL(clicked()), this, SLOT(panic_button()));
    QObject::connect(InstButtonUp, SIGNAL(clicked()), this, SLOT(select_instrumentUp()));
    QObject::connect(InstButtonDown, SIGNAL(clicked()), this, SLOT(select_instrumentDown()));
    QObject::connect(effectButtonUp, SIGNAL(clicked()), this, SLOT(select_SoundEffectUp()));
    QObject::connect(effectButtonDown, SIGNAL(clicked()), this, SLOT(select_SoundEffectDown()));

    QObject::connect(groupBoxEffect, SIGNAL(clicked(bool)), this, SLOT(set_key_effect(bool)));

    QObject::connect(checkBoxPrgBank, SIGNAL(clicked(bool)), this, SLOT(set_skip_prgbanks(bool)));
    QObject::connect(bankskipcheckBox, SIGNAL(clicked(bool)), this, SLOT(set_skip_bankonly(bool)));
    QObject::connect(checkBoxWait, SIGNAL(clicked(bool)), this, SLOT(set_record_waits(bool)));

    connect(NoteBoxCut, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
       int nitem = NoteBoxCut->itemData(v).toInt();
       if(nitem == -1) {

           set_current_note(-1);
           int note = get_key();
           if(note >= 0) {

               while(NoteBoxCut->count() > 1) {
                   NoteBoxCut->removeItem(1);
               }

               NoteBoxCut->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
               NoteBoxCut->setCurrentIndex(1);
               NoteBoxCut->addItem("DUO", -2);
               NoteBoxCut->addItem("C -1", 0);
               set_note_cut(note);

           } else {
               if(NoteBoxCut->itemData(1).toInt() >= 0)  NoteBoxCut->removeItem(1);
               set_note_cut(-1);
           }
           _settings->setValue("MIDIin_note_cut", _note_cut);
           _note_duo = false;
           _note_zero = false;
       } else if(nitem == -2) {
           _note_duo = true;
           _note_zero = false;
       }
       else {
           _note_duo = false;
           _note_zero = false;

           if(nitem == 0) _note_zero = true;
           else _note_cut = nitem;
       }

       _settings->setValue("MIDIin_note_duo", _note_duo);
       _settings->setValue("MIDIin_note_zero", _note_zero);

    });

    connect(NoteBoxEffect1, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
       if(NoteBoxEffect1->itemData(v).toInt() < 0) {

           set_current_note(-1);
           int note = get_key();
           if(note >= 0) {

               NoteBoxEffect1->removeItem(1);
               NoteBoxEffect1->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
               NoteBoxEffect1->setCurrentIndex(1);
               _note_effect1 = note;


           } else _note_effect1 = -1;
           _settings->setValue("MIDIin_note_effect1", _note_effect1);
       }
    });

    connect(NoteBoxEffect2, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
       if(NoteBoxEffect2->itemData(v).toInt() < 0) {

           set_current_note(-1);
           int note = get_key();
           if(note >= 0) {

               NoteBoxEffect2->removeItem(1);
               NoteBoxEffect2->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
               NoteBoxEffect2->setCurrentIndex(1);
               _note_effect2 = note;


           } else _note_effect2 = -1;
           _settings->setValue("MIDIin_note_effect2", _note_effect2);
       }
    });

    connect(NoteBoxEffect3, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
       if(NoteBoxEffect3->itemData(v).toInt() < 0) {

           set_current_note(-1);
           int note = get_key();
           if(note >= 0) {

               NoteBoxEffect3->removeItem(1);
               NoteBoxEffect3->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
               NoteBoxEffect3->setCurrentIndex(1);
               _note_effect3 = note;


           } else _note_effect3 = -1;
           _settings->setValue("MIDIin_note_effect3", _note_effect3);
       }
    });

    connect(NoteBoxEffect4, QOverload<int>::of(&QComboBox::activated), [=](int v)
    {
       if(NoteBoxEffect4->itemData(v).toInt() < 0) {

           set_current_note(-1);
           int note = get_key();
           if(note >= 0) {

               NoteBoxEffect4->removeItem(1);
               NoteBoxEffect4->insertItem(1, QString::asprintf("%s %i", notes[note % 12], note / 12 - 1), note);
               NoteBoxEffect4->setCurrentIndex(1);
               _note_effect4 = note;


           } else _note_effect4 = -1;
           _settings->setValue("MIDIin_note_effect4", _note_effect4);
       }
    });

    time_updat= new QTimer(this);
    time_updat->setSingleShot(false);

    connect(time_updat, SIGNAL(timeout()), this, SLOT(update_checks()), Qt::DirectConnection);
    time_updat->setSingleShot(false);
    time_updat->start(50); //flip

    QMetaObject::connectSlotsByName(MIDIin);
    first = 0;
}

void MidiInControl::reject() {

    hide();
}

void MidiInControl::accept() {

    hide();
}

MidiInControl::~MidiInControl() {
    time_updat->stop();
    delete time_updat;
    qWarning("MidiInControl() destructor");
}
void MidiInControl::MidiIn_toexit(MidiInControl *MidiIn) {
    MidiIn_ctrl = MidiIn;
}

void MidiInControl::my_exit() {
    if(file_live) delete file_live;
    file_live = NULL;
    if(MidiIn_ctrl) delete MidiIn_ctrl;
    MidiIn_ctrl = NULL;
}

void MidiInControl::split_reset() {

    _split_enable = false;
    _note_cut = -1;
    _note_duo = false;
    _note_zero = false;
    _inchannelUp = -1;
    _inchannelDown = -1;
    _channelUp = -1;
    _channelDown = -1;
    _fixVelUp = false;
    _fixVelDown = false;
    _autoChordUp = false;
    _autoChordDown = false;
    _notes_only = true;
    _events_to_down = false;
    _transpose_note_up = 0;
    _transpose_note_down = 0;
    _skip_prgbanks = true;
    _skip_bankonly = true;
    _record_waits = true;

    SplitBox->setChecked(_split_enable);

    while(NoteBoxCut->count() > 1) {
        NoteBoxCut->removeItem(1);
    }

    NoteBoxCut->addItem("DUO", -2);
    NoteBoxCut->addItem("C -1", 0);
    NoteBoxCut->setCurrentIndex(0);

    inchannelBoxUp->setCurrentIndex(_inchannelUp + 1);
    inchannelBoxDown->setCurrentIndex(_inchannelDown + 1);
    channelBoxUp->setCurrentIndex(_channelUp + 1);
    channelBoxDown->setCurrentIndex(_channelDown + 1);
    vcheckBoxUp->setChecked(_fixVelUp);
    vcheckBoxDown->setChecked(_fixVelDown);
    achordcheckBoxUp->setChecked(_autoChordUp);
    achordcheckBoxDown->setChecked(_autoChordDown);
    echeckBox->setChecked(_notes_only);
    echeckBoxDown->setChecked(_events_to_down);
    tspinBoxUp->setValue(_transpose_note_up);
    tspinBoxDown->setValue(_transpose_note_down);
    checkBoxPrgBank->setChecked(_skip_prgbanks);
    bankskipcheckBox->setChecked(_skip_bankonly);
    checkBoxWait->setChecked(_record_waits);

    if(file_live) delete file_live;
    file_live = NULL;
    panic_button();
    effect1_on = 0;
    effect2_on = 0;
    effect3_on = 0;
    effect4_on = 0;
    led_up = 0;
    led_down = 0;
    update_checks();

    if(!MidiInput::recording())
        MidiInput::cleanKeyMaps();
}

void MidiInControl::panic_button() {
    MidiPlayer::panic();
    send_live_events();  // send live event effects
    effect1_on = 0;
    effect2_on = 0;
    effect3_on = 0;
    effect4_on = 0;

    if(_note_effect1_type == 6 && _note_effect1_fkeypressed && _autoChordUp) effect1_on = true;
    if(_note_effect1_type == 7 && _note_effect1_fkeypressed && _autoChordDown) effect1_on  = true;
    if(_note_effect2_type == 6 && _note_effect2_fkeypressed && _autoChordUp) effect2_on = true;
    if(_note_effect2_type == 7 && _note_effect2_fkeypressed && _autoChordDown) effect2_on  = true;
    if(_note_effect3_type == 6 && _note_effect3_fkeypressed && _autoChordUp) effect3_on = true;
    if(_note_effect3_type == 7 && _note_effect3_fkeypressed && _autoChordDown) effect3_on  = true;
    if(_note_effect4_type == 6 && _note_effect4_fkeypressed && _autoChordUp) effect4_on = true;
    if(_note_effect4_type == 7 && _note_effect4_fkeypressed && _autoChordDown) effect4_on  = true;

    led_up = 0;
    led_down = 0;
    if(!MidiInput::recording())
        MidiInput::cleanKeyMaps();
    update_checks();
}

void MidiInControl::send_live_events() {
    if(file_live) {
        for(int n = 0; n < 16; n++) {
            foreach (MidiEvent* event, file_live->channel(n)->eventMap()->values()) {
                MidiOutput::sendCommand(event);
            }
        }

    }
}

void MidiInControl::select_instrumentUp() {

    int ch = ((MidiInControl::channelUp() < 0)
              ? MidiOutput::standardChannel()
              : (MidiInControl::channelUp() & 15));

    if(!file_live) {
        file_live = new MidiFile();
    }

    InstrumentChooser* d = new InstrumentChooser(NULL, ch, this); // no MidiFile()
    d->setModal(true);
    d->exec();
    delete d;
    send_live_events();
}

void MidiInControl::select_SoundEffectUp()
{
    int ch = ((MidiInControl::channelUp() < 0)
              ? MidiOutput::standardChannel()
              : (MidiInControl::channelUp() & 15));

    if(!file_live) {
        file_live = new MidiFile();
    }

    SoundEffectChooser* d = new SoundEffectChooser(file_live, ch, this, SOUNDEFFECTCHOOSER_FORMIDIIN);
    d->exec();
    delete d;
    send_live_events();
}

void MidiInControl::select_instrumentDown() {

    int ch = ((MidiInControl::channelDown() < 0)
              ? ((MidiInControl::channelUp() < 0)
                 ? MidiOutput::standardChannel()
                 : (MidiInControl::channelUp() & 15))
              : (MidiInControl::channelDown() & 15));


    if(!file_live) {
        file_live = new MidiFile();
    }


    InstrumentChooser* d = new InstrumentChooser(NULL, ch, this); // no MidiFile()
    d->setModal(true);
    d->exec();
    delete d;
    send_live_events();
}

void MidiInControl::select_SoundEffectDown()
{
    int ch = ((MidiInControl::channelDown() < 0)
              ? ((MidiInControl::channelUp() < 0)
                 ? MidiOutput::standardChannel()
                 : (MidiInControl::channelUp() & 15))
              : (MidiInControl::channelDown() & 15));


    if(!file_live) {
        file_live = new MidiFile();
    }

    SoundEffectChooser* d = new SoundEffectChooser(file_live, ch, this, SOUNDEFFECTCHOOSER_FORMIDIIN, 0);
    d->exec();
    delete d;
    send_live_events();
}


void MidiInControl::set_prog(int channel, int value) {
    Prog_MIDI[channel] = value;
}

void MidiInControl::set_bank(int channel, int value) {
    Bank_MIDI[channel] = value;
}

void MidiInControl::set_output_prog_bank_channel(int channel) {

    ProgChangeEvent* pevent;
    ControlChangeEvent* cevent;
    pevent = new ProgChangeEvent(channel, Prog_MIDI[channel], 0);
    cevent = new ControlChangeEvent(channel, 0x0, Bank_MIDI[channel], 0);
    MidiOutput::sendCommand2(cevent);
    MidiOutput::sendCommand2(pevent);
}

static QMessageBox *mb2 = NULL;

int MidiInControl::get_key() {

    set_current_note(-1);
    QMessageBox *mb = new QMessageBox("MIDI Input Control",
                                      "Press a note in the keyboard",
                                      QMessageBox::Information,
                          QMessageBox::Cancel, 0, 0, this);

    QFont font;
    font.setPixelSize(24);
    mb->setFont(font);
    mb->setIconPixmap(QPixmap(":/run_environment/graphics/channelwidget/instrument.png"));
    mb->button(QMessageBox::Cancel)->animateClick(10000);
    mb2 = mb;
    mb->exec();
    mb2 = NULL;
    delete mb;

    for(int n = 0; n < 16; n++) {
        QByteArray array;
        array.clear();
        array.append(0xB0 | n);
        array.append(char(123)); // all notes off
        array.append(char(127));

        MidiOutput::sendCommand(array);
    }

    return current_note();
}

int MidiInControl::wait_record(QWidget *parent) {

    if(!_record_waits) return 0;

    set_current_note(-1);
    QMessageBox *mb = new QMessageBox("MIDI Input Control",
                                      "Press a note in the keyboard\nto start the recording",
                                      QMessageBox::Information,
                          QMessageBox::Cancel, 0, 0, parent);

    QFont font;
    font.setPixelSize(24);
    mb->setFont(font);
    mb->setIconPixmap(QPixmap(":/run_environment/graphics/channelwidget/instrument.png"));

    mb2 = mb;
    mb->exec();
    mb2 = NULL;
    delete mb;

    return current_note();
}


void MidiInControl::set_key(int key) {
    if(mb2 == NULL) return;
    _current_note = key;
    mb2->hide();
    mb2 = NULL;
    QThread::msleep(100); // very important sleep()

}


bool MidiInControl::split_enable() {
    return _split_enable;
}

int MidiInControl::note_cut() {
    return ((_note_zero) ? 0 :_note_cut);
}

bool MidiInControl::note_duo() {
    return _note_duo;
}

int MidiInControl::inchannelUp() {
    return _inchannelUp;
}

int MidiInControl::inchannelDown() {
    return _inchannelDown;
}

int MidiInControl::channelUp() {
    return _channelUp;
}

int MidiInControl::channelDown() {
    return _channelDown;
}

bool MidiInControl::fixVelUp() {
    return _fixVelUp;
}

bool MidiInControl::fixVelDown() {
    return _fixVelDown;
}

bool MidiInControl::autoChordUp() {
    return _autoChordUp;
}

bool MidiInControl::autoChordDown() {
    return _autoChordDown;
}

int MidiInControl::current_note() {
    return _current_note;
}

bool MidiInControl::skip_prgbanks() {
    return _skip_prgbanks;
}

bool MidiInControl::skip_bankonly() {
    return _skip_bankonly;
}

bool MidiInControl::notes_only() {
    return _notes_only;
}

bool MidiInControl::events_to_down() {
    return _events_to_down;
}

int MidiInControl::transpose_note_up() {
    return _transpose_note_up;
}

int MidiInControl::transpose_note_down() {
    return _transpose_note_down;
}

bool MidiInControl::key_effect() {
    return _key_effect;
}

////////////////////////////////////////////

void MidiInControl::set_note_cut(int v) {
    _note_cut = v;
}

void MidiInControl::set_current_note(int v){
    _current_note = v;
}

// slots

void MidiInControl::set_split_enable(bool v) {

    _split_enable= v;
    _settings->setValue("MIDIin_split_enable", _split_enable);

}

void MidiInControl::set_inchannelUp(int v) {
    _inchannelUp = v - 1;
    _settings->setValue("MIDIin_inchannelUp", _inchannelUp);

}

void MidiInControl::set_inchannelDown(int v) {
    _inchannelDown = v - 1;
    _settings->setValue("MIDIin_inchannelDown", _inchannelDown);

}

void MidiInControl::set_channelUp(int v) {
    _channelUp = v - 1;
    _settings->setValue("MIDIin_channelUp", _channelUp);

}

void MidiInControl::set_channelDown(int v) {
    _channelDown = v - 1;
    _settings->setValue("MIDIin_channelDown", _channelDown);

}

void MidiInControl::set_fixVelUp(bool v) {
    _settings->setValue("MIDIin_fixVelUp", v);
    _fixVelUp = v;
}

void MidiInControl::set_fixVelDown(bool v) {
    _settings->setValue("MIDIin_fixVelDown", v);
    _fixVelDown = v;
}

void MidiInControl::set_autoChordUp(bool v) {

    _settings->setValue("MIDIin_autoChordUp", v);
    _autoChordUp = v;

    if(_note_effect1_fkeypressed && _note_effect1_type == 6) effect1_on = _autoChordUp;
    if(_note_effect2_fkeypressed && _note_effect2_type == 6) effect2_on = _autoChordUp;
    if(_note_effect3_fkeypressed && _note_effect3_type == 6) effect3_on = _autoChordUp;
    if(_note_effect4_fkeypressed && _note_effect4_type == 6) effect4_on = _autoChordUp;
}

void MidiInControl::set_autoChordDown(bool v) {

    _settings->setValue("MIDIin_autoChordDown", v);
    _autoChordDown = v;

    if(_note_effect1_fkeypressed && _note_effect1_type == 7) effect1_on = _autoChordDown;
    if(_note_effect2_fkeypressed && _note_effect2_type == 7) effect2_on = _autoChordDown;
    if(_note_effect3_fkeypressed && _note_effect3_type == 7) effect3_on = _autoChordDown;
    if(_note_effect4_fkeypressed && _note_effect4_type == 7) effect4_on = _autoChordDown;
}
void MidiInControl::set_notes_only(bool v) {
    _settings->setValue("MIDIin_notes_only", v);
    _notes_only = v;
}

void MidiInControl::set_events_to_down(bool v) {
    _settings->setValue("MIDIin_events_to_down", v);
    _events_to_down = v;
}

void MidiInControl::set_transpose_note_up(int v) {
    _settings->setValue("MIDIin_transpose_note_up", v);
    _transpose_note_up = v;
}

void MidiInControl::set_transpose_note_down(int v) {
    _settings->setValue("MIDIin_transpose_note_down", v);
    _transpose_note_down = v;
}


void MidiInControl::set_key_effect(bool v) {
    _settings->setValue("MIDIin_key_effect", v);
    _key_effect = v;
}

void MidiInControl::set_note_effect1_value(int v) {
    _settings->setValue("MIDIin_note_effect1_value", v);
    _note_effect1_value = v;
}

void MidiInControl::set_note_effect1_type(int v) {
    _settings->setValue("MIDIin_note_effect1_type", v);
    _note_effect1_type = v;

    if(_note_effect1_fkeypressed && _note_effect1_type == 6) effect1_on = _autoChordUp;
    if(_note_effect1_fkeypressed && _note_effect1_type == 7) effect1_on = _autoChordDown;
}

void MidiInControl::set_note_effect1_fkeypressed(bool v) {
    if(!v) effect1_on = 0;
    _settings->setValue("MIDIin_note_effect1_fkeypressed", v);
    _note_effect1_fkeypressed = v;
}

void MidiInControl::set_note_effect1_usevel(bool v) {
    _settings->setValue("MIDIin_note_effect1_usevel", v);
    _note_effect1_usevel = v;
}

void MidiInControl::set_note_effect2_value(int v) {
    _settings->setValue("MIDIin_note_effect2_value", v);
    _note_effect2_value = v;
}

void MidiInControl::set_note_effect2_type(int v) {
    _settings->setValue("MIDIin_note_effect2_type", v);
    _note_effect2_type = v;

    if(_note_effect2_fkeypressed && _note_effect2_type == 6) effect2_on = _autoChordUp;
    if(_note_effect2_fkeypressed && _note_effect2_type == 7) effect2_on = _autoChordDown;
}

void MidiInControl::set_note_effect2_fkeypressed(bool v) {
    if(!v) effect2_on = 0;
    _settings->setValue("MIDIin_note_effect2_fkeypressed", v);
    _note_effect2_fkeypressed = v;
}

void MidiInControl::set_note_effect2_usevel(bool v) {
    _settings->setValue("MIDIin_note_effect2_usevel", v);
    _note_effect2_usevel = v;
}

void MidiInControl::set_note_effect3_value(int v) {
    _settings->setValue("MIDIin_note_effect3_value", v);
    _note_effect3_value = v;
}

void MidiInControl::set_note_effect3_type(int v) {
    _settings->setValue("MIDIin_note_effect3_type", v);
    _note_effect3_type = v;

    if(_note_effect3_fkeypressed && _note_effect3_type == 6) effect3_on = _autoChordUp;
    if(_note_effect3_fkeypressed && _note_effect3_type == 7) effect3_on = _autoChordDown;
}

void MidiInControl::set_note_effect3_fkeypressed(bool v) {
    if(!v) effect3_on = 0;
    _settings->setValue("MIDIin_note_effect3_fkeypressed", v);
    _note_effect3_fkeypressed = v;
}

void MidiInControl::set_note_effect3_usevel(bool v) {
    _settings->setValue("MIDIin_note_effect3_usevel", v);
    _note_effect3_usevel = v;
}

void MidiInControl::set_note_effect4_value(int v) {
    _settings->setValue("MIDIin_note_effect4_value", v);
    _note_effect4_value = v;
}

void MidiInControl::set_note_effect4_type(int v) {
    _settings->setValue("MIDIin_note_effect4_type", v);
    _note_effect4_type = v;

    if(_note_effect4_fkeypressed && _note_effect4_type == 6) effect4_on = _autoChordUp;
    if(_note_effect4_fkeypressed && _note_effect4_type == 7) effect4_on = _autoChordDown;
}

void MidiInControl::set_note_effect4_fkeypressed(bool v) {
    if(!v) effect4_on = 0;
    _settings->setValue("MIDIin_note_effect4_fkeypressed", v);
    _note_effect4_fkeypressed = v;
}

void MidiInControl::set_note_effect4_usevel(bool v) {
    _settings->setValue("MIDIin_note_effect4_usevel", v);
    _note_effect4_usevel = v;
}

void MidiInControl::set_skip_prgbanks(bool v) {
    _settings->setValue("MIDIin_skip_prgbanks", v);
    _skip_prgbanks = v;
}

void MidiInControl::set_skip_bankonly(bool v) {
    _settings->setValue("MIDIin_skip_bankonly", v);
    _skip_bankonly = v;
}

void MidiInControl::set_leds(bool up, bool down) {
    if(up) led_up = 7;
    if(down) led_down = 7;
}

// from timer, set LEDs, ecc

void MidiInControl::update_checks() {
    static int flip = 0;
    flip++;

    if(!groupBoxEffect->isEnabled() || !groupBoxEffect->isChecked()) flip = 0;

    if(MidiInput::isConnected() && !MIDIin->isEnabled()) {
        setDisabled(false);
    } else if(!MidiInput::isConnected() && MIDIin->isEnabled()) {

        setDisabled(true);
    }

    if(_autoChordUp != achordcheckBoxUp->isChecked())
        achordcheckBoxUp->setChecked(_autoChordUp);
    if(_autoChordDown != achordcheckBoxDown->isChecked())
        achordcheckBoxDown->setChecked(_autoChordDown);

    if(led_up) {
        led_up--;
        LEDBoxUp->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ff60ff00\n}"));
    } else
        LEDBoxUp->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #80004010\n}"));

    if(led_down) {
        led_down--;
        LEDBoxDown->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ff60ff00\n}"));
    } else
        LEDBoxDown->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #80004010\n}"));


    if(effect1_on && ((flip & 2) || !_note_effect1_fkeypressed)) {
        if(_note_effect1_fkeypressed)
            LEDBoxEffect1->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ffff3030\n}"));
        else
            LEDBoxEffect1->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ff60ff00\n}"));
    } else
        LEDBoxEffect1->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #80004010\n}"));

    if(effect2_on && ((flip & 2) || !_note_effect2_fkeypressed)) {
        if(_note_effect2_fkeypressed)
            LEDBoxEffect2->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ffff3030\n}"));
        else
            LEDBoxEffect2->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ff60ff00\n}"));
    } else
        LEDBoxEffect2->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #80004010\n}"));

    if(effect3_on && ((flip & 2) || !_note_effect3_fkeypressed)) {
        if(_note_effect3_fkeypressed)
            LEDBoxEffect3->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ffff3030\n}"));
        else
            LEDBoxEffect3->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ff60ff00\n}"));
    } else
        LEDBoxEffect3->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #80004010\n}"));

    if(effect4_on && ((flip & 2) || !_note_effect4_fkeypressed)) {
        if(_note_effect4_fkeypressed)
            LEDBoxEffect4->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ffff3030\n}"));
        else
            LEDBoxEffect4->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #ff60ff00\n}"));
    } else
        LEDBoxEffect4->setStyleSheet(QString::fromUtf8("QCheckBox::indicator{\nbackground-color: #80004010\n}"));

    update();
}

///////////////////////////////////////////////////////////////////////////////////////
// effects from keyboard

int MidiInControl::set_effect(std::vector<unsigned char>* message) {
    int evt = message->at(0) & 0xF0;
    int pedal = (evt == 0xB0 && message->at(1) == 64);
    int pedal_on = 0;

    if(pedal)
        pedal_on = (((int) message->at(2)) >= 64);

    if((pedal ||
        ((evt == 0x80 || evt == 0x90) && ((int) message->at(1)) == _note_effect1))
            && message->size() == 3) {

        int channel = ((MidiInControl::channelUp() < 0)
                       ? MidiOutput::standardChannel()
                       : (MidiInControl::channelUp() & 15));

        if(evt == 0x90 || (evt == 0xB0 && pedal_on)) {

            if(_note_effect1_fkeypressed)
                effect1_on^= 1; // toggle
            else
                effect1_on = 1;
        } else if(!_note_effect1_fkeypressed)
                effect1_on = 0;

        if(_note_effect1_type == 0) { // pitch bend

            int v = (_note_effect1_usevel) ? message->at(2) : 127;
            v = (((v*_note_effect1_value / 198)+64)*128) & 16383;

            message->at(0) = 0xE0 | channel;

            if(evt == 0x80 || (evt == 0xB0 && !pedal_on)) {
                if(_note_effect1_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 8192;
            } else if(_note_effect1_fkeypressed && !effect1_on)
                v = 8192;

            message->at(1) = (v & 0x7F);
            message->at(2) = ((v >> 7) & 0x7F);

            return 1;

        } else if(_note_effect1_type == 1 || // Modulation Wheel
                  _note_effect1_type == 4 || // Reverb
                  _note_effect1_type == 5) { // Chorus

                   int v = (_note_effect1_usevel) ? message->at(2) : 127;
                   v = ((v*(_note_effect1_value)) / 127);

                   message->at(0) = 0xB0 | channel;

                   if(evt == 0x80 || (evt == 0xB0 && !pedal_on)) {
                       if(_note_effect1_fkeypressed) {
                           message->at(0) = 0; // no event
                           return 1;
                       } else v = 0;
                   } else if(_note_effect1_fkeypressed && !effect1_on)
                       v = 0;

                   switch(_note_effect1_type) {
                       case 1:
                           message->at(1) = 1; // Modulation Wheel
                           break;
                       case 4:
                           message->at(1) = 91;  // Reverb
                           break;
                       case 5:
                           message->at(1) = 93; // Chorus
                           break;
                       default:
                           message->at(0) = 0; // invalid
                           break;
                   }

                   message->at(2) = v;

                   return 1;

               } else if(_note_effect1_type == 2 || _note_effect1_type == 3) {

            message->at(0) = 0xB0 | channel;

            message->at(1) = 64 + 2 * (_note_effect1_type == 3); // sustain / sostenuto
            message->at(2) = 127;

            if(evt == 0x80 || (evt == 0xB0 && !pedal_on)) {
                if(_note_effect1_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else message->at(2) = 0;
            } else if(_note_effect1_fkeypressed && !effect1_on)
                message->at(2) = 0;

            return 1;
        }  else if(_note_effect1_type == 6 || _note_effect1_type == 7) { // autochord
            //
            if(evt == 0x80 || (evt == 0xB0 && !pedal_on)) {
                if(_note_effect1_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else {
                    if(_note_effect1_type == 6)
                        _autoChordUp = false;
                    else _autoChordDown = false;
                }
            } else if(_note_effect1_fkeypressed && !effect1_on){
                if(_note_effect1_type == 6)
                    _autoChordUp = false;
                else _autoChordDown = false;
            } else {
                if(_note_effect1_type == 6)
                    _autoChordUp = true;
                else _autoChordDown = true;
            }

            message->at(0) = 0;
            return 1;
        }else {
            message->at(0) = 0;
            return 1;
        }
    } else if((evt == 0x80 || evt == 0x90) && ((int) message->at(1)) == _note_effect2
              && message->size() == 3) {

        int channel = ((MidiInControl::channelUp() < 0)
                       ? MidiOutput::standardChannel()
                       : (MidiInControl::channelUp() & 15));

        if(evt == 0x90) {
            if(_note_effect2_fkeypressed)
                effect2_on^= 1; // toggle
            else
                effect2_on = 1;
        } else if(!_note_effect2_fkeypressed)
                effect2_on = 0;

        if(_note_effect2_type == 0) { // pitch bend

            int v = (_note_effect2_usevel) ? message->at(2) : 127;
            v = (((v*_note_effect2_value / 198)+64)*128) & 16383;

            message->at(0) = 0xE0 | channel;

            if(evt == 0x80) {
                if(_note_effect2_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 8192;
            } else if(_note_effect2_fkeypressed && !effect2_on)
                v = 8192;

            message->at(1) = (v & 0x7F);
            message->at(2) = ((v >> 7) & 0x7F);

            return 1;

        } else if(_note_effect2_type == 1 || // Modulation Wheel
                  _note_effect2_type == 4 || // Reverb
                  _note_effect2_type == 5) { // Chorus

            int v = (_note_effect2_usevel) ? message->at(2) : 127;
            v = ((v*(_note_effect2_value)) / 127);

            message->at(0) = 0xB0 | channel;

            if(evt == 0x80) {
                if(_note_effect2_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 0;
            } else if(_note_effect2_fkeypressed && !effect2_on)
                v = 0;

            switch(_note_effect2_type) {
            case 1:
                message->at(1) = 1; // Modulation Wheel
                break;
            case 4:
                message->at(1) = 91;  // Reverb
                break;
            case 5:
                message->at(1) = 93; // Chorus
                break;
            default:
                message->at(0) = 0; // invalid
                break;
            }

            message->at(2) = v;

            return 1;

        } else if(_note_effect2_type == 2 || _note_effect2_type == 3) {

            message->at(0) = 0xB0 | channel;

            message->at(1) = 64 + 2 * (_note_effect2_type == 3); // sustain / sostenuto
            message->at(2) = 127;

            if(evt == 0x80) {
                if(_note_effect2_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else message->at(2) = 0;
            } else if(_note_effect2_fkeypressed && !effect2_on)
                message->at(2) = 0;

            return 1;
        }  else if(_note_effect2_type == 6 || _note_effect2_type == 7) { // autochord
            //
            if(evt == 0x80) {
                if(_note_effect2_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else {
                    if(_note_effect2_type == 6)
                        _autoChordUp = false;
                    else _autoChordDown = false;
                }
            } else if(_note_effect2_fkeypressed && !effect2_on){
                if(_note_effect2_type == 6)
                    _autoChordUp = false;
                else _autoChordDown = false;
            } else {
                if(_note_effect2_type == 6)
                    _autoChordUp = true;
                else _autoChordDown = true;
            }

            message->at(0) = 0;
            return 1;
        }else {
            message->at(0) = 0;
            return 1;
        }
    } else if((evt == 0x80 || evt == 0x90) && ((int) message->at(1)) == _note_effect3
              && message->size() == 3) {

        int channel = ((MidiInControl::channelUp() < 0)
                       ? MidiOutput::standardChannel()
                       : (MidiInControl::channelUp() & 15));

        if(evt == 0x90) {
            if(_note_effect3_fkeypressed)
                effect3_on^= 1; // toggle
            else
                effect3_on = 1;
        } else if(!_note_effect3_fkeypressed)
            effect3_on = 0;

        if(_note_effect3_type == 0) { // pitch bend

            int v = (_note_effect3_usevel) ? message->at(2) : 127;
            v = (((v*_note_effect3_value / 198)+64)*128) & 16383;

            message->at(0) = 0xE0 | channel;

            if(evt == 0x80) {
                if(_note_effect3_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 8192;
            } else if(_note_effect3_fkeypressed && !effect3_on)
                v = 8192;

            message->at(1) = (v & 0x7F);
            message->at(2) = ((v >> 7) & 0x7F);

            return 1;

        } else if(_note_effect3_type == 1 || // Modulation Wheel
                  _note_effect3_type == 4 || // Reverb
                  _note_effect3_type == 5) { // Chorus

            int v = (_note_effect3_usevel) ? message->at(2) : 127;
            v = ((v*(_note_effect3_value)) / 127);

            message->at(0) = 0xB0 | channel;

            if(evt == 0x80) {
                if(_note_effect3_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 0;
            } else if(_note_effect3_fkeypressed && !effect2_on)
                v = 0;

            switch(_note_effect3_type) {
            case 1:
                message->at(1) = 1; // Modulation Wheel
                break;
            case 4:
                message->at(1) = 91;  // Reverb
                break;
            case 5:
                message->at(1) = 93; // Chorus
                break;
            default:
                message->at(0) = 0; // invalid
                break;
            }

            message->at(2) = v;

            return 1;

        } else if(_note_effect3_type == 2 || _note_effect3_type == 3) {

            message->at(0) = 0xB0 | channel;

            message->at(1) = 64 + 2 * (_note_effect3_type == 3); // sustain / sostenuto
            message->at(2) = 127;

            if(evt == 0x80) {
                if(_note_effect3_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else message->at(2) = 0;
            } else if(_note_effect3_fkeypressed && !effect3_on)
                message->at(2) = 0;

            return 1;
        }  else if(_note_effect3_type == 6 || _note_effect3_type == 7) { // autochord
            //
            if(evt == 0x80) {
                if(_note_effect3_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else {
                    if(_note_effect3_type == 6)
                        _autoChordUp = false;
                    else _autoChordDown = false;
                }
            } else if(_note_effect3_fkeypressed && !effect3_on){
                if(_note_effect3_type == 6)
                    _autoChordUp = false;
                else _autoChordDown = false;
            } else {
                if(_note_effect3_type == 6)
                    _autoChordUp = true;
                else _autoChordDown = true;
            }

            message->at(0) = 0;
            return 1;
        }else {
            message->at(0) = 0;
            return 1;
        }
    } else if((evt == 0x80 || evt == 0x90) && ((int) message->at(1)) == _note_effect4
              && message->size() == 3) {

        int channel = ((MidiInControl::channelUp() < 0)
                       ? MidiOutput::standardChannel()
                       : (MidiInControl::channelUp() & 15));

        if(evt == 0x90) {
            if(_note_effect4_fkeypressed)
                effect4_on^= 1; // toggle
            else
                effect4_on = 1;
        } else if(!_note_effect4_fkeypressed)
            effect4_on = 0;

        if(_note_effect4_type == 0) { // pitch bend

            int v = (_note_effect4_usevel) ? message->at(2) : 127;
            v = (((v*_note_effect4_value / 198)+64)*128) & 16383;

            message->at(0) = 0xE0 | channel;

            if(evt == 0x80) {
                if(_note_effect4_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 8192;
            } else if(_note_effect4_fkeypressed && !effect4_on)
                v = 8192;

            message->at(1) = (v & 0x7F);
            message->at(2) = ((v >> 7) & 0x7F);


            return 1;

        } else if(_note_effect4_type == 1 || // Modulation Wheel
                  _note_effect4_type == 4 || // Reverb
                  _note_effect4_type == 5) { // Chorus

            int v = (_note_effect4_usevel) ? message->at(2) : 127;
            v = ((v*(_note_effect4_value)) / 127);

            message->at(0) = 0xB0 | channel;

            if(evt == 0x80) {
                if(_note_effect4_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else v = 0;
            } else if(_note_effect4_fkeypressed && !effect4_on)
                v = 0;

            switch(_note_effect4_type) {
            case 1:
                message->at(1) = 1; // Modulation Wheel
                break;
            case 4:
                message->at(1) = 91;  // Reverb
                break;
            case 5:
                message->at(1) = 93; // Chorus
                break;
            default:
                message->at(0) = 0; // invalid
                break;
            }

            message->at(2) = v;

            return 1;

        } else if(_note_effect4_type == 2 || _note_effect4_type == 3) {

            message->at(0) = 0xB0 | channel;

            message->at(1) = 64 + 2 * (_note_effect4_type == 3); // sustain / sostenuto
            message->at(2) = 127;

            if(evt == 0x80) {
                if(_note_effect4_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else message->at(2) = 0;
            } else if(_note_effect4_fkeypressed && !effect4_on)
                message->at(2) = 0;

            return 1;
        } else if(_note_effect4_type == 6 || _note_effect4_type == 7) { // autochord
            //

            if(evt == 0x80) {
                if(_note_effect4_fkeypressed) {
                    message->at(0) = 0; // no event
                    return 1;
                } else {
                    if(_note_effect4_type == 6)
                        _autoChordUp = false;
                    else _autoChordDown = false;
                }
            } else if(_note_effect4_fkeypressed && !effect4_on){
                if(_note_effect4_type == 6)
                    _autoChordUp = false;
                else _autoChordDown = false;
            } else {
                if(_note_effect4_type == 6)
                    _autoChordUp = true;
                else _autoChordDown = true;
            }

            message->at(0) = 0;
            return 1;
        } else {
            message->at(0) = 0;
            return 1;
        }
    }

    return 0;
}

void MidiInControl::set_record_waits(bool v) {
    _settings->setValue("MIDIin_record_waits", v);
    _record_waits = v;
}


////////////////////////////////////////////////////////////////////////////////////
// auto chord section

extern int chord_noteM(int note, int position);
extern int chord_notem(int note, int position);

static int buildCMajorProg(int index, int note) {

    switch(index) {
        case 0:
            note = note/12 * 12 + chord_noteM(note % 12, 1);
        break;
        case 1:
            note = note/12 * 12 + chord_noteM(note % 12, 3);
        break;
        case 2:
            note = note/12 * 12 + chord_noteM(note % 12, 5);
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}

static int buildPowerChord(int index, int note) {

    switch(index) {
        case 1:
            note += 7;
        break;
        case AUTOCHORD_MAX:
            return 2;
    }

    return note;
}

static int buildPowerChordExt(int index, int note) {

    switch(index) {
        case 1:
            note += 7;
        break;
        case 2:
            note += 12;
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}

static int buildCMajor(int index, int note) {

    switch(index) {
        case 1:
            note += 4;
        break;
        case 2:
            note += 7;
        break;
        case AUTOCHORD_MAX:
            return 3;
    }

    return note;
}


int MidiInControl::autoChordfunUp(int index, int note, int vel) {

    if(!_autoChordUp) { // no chord
        if(index == AUTOCHORD_MAX) return 1;
        if(note >= 0) return note;
        return vel;
    }

    if(note >= 0 || index == AUTOCHORD_MAX) {
        switch(chordTypeUp) {
            case 0:
                note = buildPowerChord(index, note);
                break;
            case 1:
                note = buildPowerChordExt(index, note);
                break;
            case 2:
                note = buildCMajor(index, note);
                break;
            case 3:
                note = buildCMajorProg(index, note);
                break;
        }

        if(note < 0) note = 0;
        if(note > 127) note = 127;

        return note;
    } else { // velocity
        switch(index) {
            case 1:
                vel = vel * chordScaleVelocity3Up / 20;
                break;
            case 2:
                vel = vel * chordScaleVelocity5Up / 20;
                break;
            case 3:
                vel = vel * chordScaleVelocity7Up / 20;
                break;
        }
        return vel;
    }
    return 0;
}

int MidiInControl::autoChordfunDown(int index, int note, int vel) {

    if(!_autoChordDown) { // no chord
        if(index == AUTOCHORD_MAX) return 1;
        if(note >= 0) return note;
        return vel;
    }

    if(note >= 0 || index == AUTOCHORD_MAX) {
        switch(chordTypeDown) {
            case 0:
                note = buildPowerChord(index, note);
                break;
            case 1:
                note = buildPowerChordExt(index, note);
                break;
            case 2:
                note = buildCMajor(index, note);
                break;
            case 3:
                note = buildCMajorProg(index, note);
                break;
        }

        if(note < 0) note = 0;
        if(note > 127) note = 127;

        return note;
    } else { // velocity
        switch(index) {
            case 1:
                vel = vel * chordScaleVelocity3Down / 20;
                break;
            case 2:
                vel = vel * chordScaleVelocity5Down / 20;
                break;
            case 3:
                vel = vel * chordScaleVelocity7Down / 20;
                break;
        }
        return vel;
    }
        return 0;
}


void MidiInControl::setChordDialogUp() {

    MidiInControl_chord* d = new MidiInControl_chord(this);
    d->chordBox->setCurrentIndex(chordTypeUp);
    d->Slider3->setValue(chordScaleVelocity3Up);
    d->Slider5->setValue(chordScaleVelocity5Up);
    d->Slider7->setValue(chordScaleVelocity7Up);

    d->extChord = &chordTypeUp;
    d->extSlider3 = &chordScaleVelocity3Up;
    d->extSlider5 = &chordScaleVelocity5Up;
    d->extSlider7 = &chordScaleVelocity7Up;

    d->exec();

    _settings->setValue("chordTypeUp", chordTypeUp);
    _settings->setValue("chordScaleVelocity3Up", chordScaleVelocity3Up);
    _settings->setValue("chordScaleVelocity5Up", chordScaleVelocity5Up);
    _settings->setValue("chordScaleVelocity7Up", chordScaleVelocity7Up);

    delete d;
}

void MidiInControl::setChordDialogDown() {

    MidiInControl_chord* d = new MidiInControl_chord(this);
    d->chordBox->setCurrentIndex(chordTypeDown);
    d->Slider3->setValue(chordScaleVelocity3Down);
    d->Slider5->setValue(chordScaleVelocity5Down);
    d->Slider7->setValue(chordScaleVelocity7Down);

    d->extChord = &chordTypeDown;
    d->extSlider3 = &chordScaleVelocity3Down;
    d->extSlider5 = &chordScaleVelocity5Down;
    d->extSlider7 = &chordScaleVelocity7Down;

    d->exec();

    _settings->setValue("chordTypeDown", chordTypeDown);
    _settings->setValue("chordScaleVelocity3Down", chordScaleVelocity3Down);
    _settings->setValue("chordScaleVelocity5Down", chordScaleVelocity5Down);
    _settings->setValue("chordScaleVelocity7Down", chordScaleVelocity7Down);

    delete d;
}

MidiInControl_chord::MidiInControl_chord(QWidget* parent): QDialog(parent, Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint) {

    QDialog *chord = this;

    extChord = NULL;
    extSlider3 = NULL;
    extSlider5 = NULL;
    extSlider7 = NULL;

    if (chord->objectName().isEmpty())
        chord->setObjectName(QString::fromUtf8("chord"));
    chord->resize(373, 275);
    chord->setFixedSize(373, 275);
    chord->setWindowTitle("AutoChord & Velocity (Live)");

    buttonBox = new QDialogButtonBox(chord);
    buttonBox->setObjectName(QString::fromUtf8("buttonBox"));
    buttonBox->setGeometry(QRect(160, 220, 191, 31));
    buttonBox->setOrientation(Qt::Horizontal);
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    label3 = new QLabel(chord);
    label3->setObjectName(QString::fromUtf8("label3"));
    label3->setGeometry(QRect(30, 80, 21, 21));
    label3->setAlignment(Qt::AlignCenter);
    label3->setText("3:");

    Slider3 = new QSlider(chord);
    Slider3->setObjectName(QString::fromUtf8("Slider3"));
    Slider3->setGeometry(QRect(60, 80, 241, 22));
    Slider3->setMaximum(20);
    Slider3->setValue(14);
    Slider3->setOrientation(Qt::Horizontal);
    Slider3->setTickPosition(QSlider::TicksBelow);

    label5 = new QLabel(chord);
    label5->setObjectName(QString::fromUtf8("label5"));
    label5->setGeometry(QRect(30, 120, 21, 21));
    label5->setAlignment(Qt::AlignCenter);
    label5->setText("5:");

    Slider5 = new QSlider(chord);
    Slider5->setObjectName(QString::fromUtf8("Slider5"));
    Slider5->setGeometry(QRect(60, 120, 241, 22));
    Slider5->setMaximum(20);
    Slider5->setValue(15);
    Slider5->setOrientation(Qt::Horizontal);
    Slider5->setTickPosition(QSlider::TicksBelow);
    label7 = new QLabel(chord);
    label7->setObjectName(QString::fromUtf8("label7"));
    label7->setGeometry(QRect(30, 160, 21, 21));
    label7->setAlignment(Qt::AlignCenter);
    label7->setText("7:");

    Slider7 = new QSlider(chord);
    Slider7->setObjectName(QString::fromUtf8("Slider7"));
    Slider7->setGeometry(QRect(60, 160, 241, 22));
    Slider7->setMaximum(20);
    Slider7->setValue(16);
    Slider7->setOrientation(Qt::Horizontal);
    Slider7->setTickPosition(QSlider::TicksBelow);

    rstButton = new QPushButton(chord);
    rstButton->setObjectName(QString::fromUtf8("rstButton"));
    rstButton->setGeometry(QRect(30, 226, 75, 23));
    rstButton->setText("Reset");

    chordBox = new QComboBox(chord);
    chordBox->setObjectName(QString::fromUtf8("chordBox"));
    chordBox->setGeometry(QRect(30, 21, 311, 22));
    chordBox->addItem("Power Chord");
    chordBox->addItem("Power Chord Extended");
    chordBox->addItem("C Major Chord (CM)");
    chordBox->addItem("C Major Chord  Progression (CM)");
    chordBox->setCurrentIndex(0);

    label3_2 = new QLabel(chord);
    label3_2->setObjectName(QString::fromUtf8("label3_2"));
    label3_2->setGeometry(QRect(320, 81, 21, 16));
    label3_2->setFrameShape(QFrame::StyledPanel);
    label3_2->setText("0");
    label5_2 = new QLabel(chord);
    label5_2->setObjectName(QString::fromUtf8("label5_2"));
    label5_2->setGeometry(QRect(320, 123, 21, 16));
    label5_2->setFrameShape(QFrame::StyledPanel);
    label5_2->setText("0");
    label7_2 = new QLabel(chord);
    label7_2->setObjectName(QString::fromUtf8("label7_2"));
    label7_2->setGeometry(QRect(320, 161, 21, 16));
    label7_2->setFrameShape(QFrame::StyledPanel);
    label7_2->setText("0");
    labelvelocity = new QLabel(chord);
    labelvelocity->setObjectName(QString::fromUtf8("labelvelocity"));
    labelvelocity->setGeometry(QRect(110, 52, 151, 20));
    labelvelocity->setAlignment(Qt::AlignCenter);
    labelvelocity->setText("Velocity scale x/20");

    connect(rstButton, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {
        Slider3->setValue(14);
        Slider5->setValue(15);
        Slider7->setValue(16);
    });

    Slider3->setValue(-1);
    Slider5->setValue(-1);
    Slider7->setValue(-1);

    QObject::connect(buttonBox, SIGNAL(accepted()), chord, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), chord, SLOT(reject()));
    QObject::connect(Slider3, SIGNAL(valueChanged(int)), label3_2, SLOT(setNum(int)));
    QObject::connect(Slider5, SIGNAL(valueChanged(int)), label5_2, SLOT(setNum(int)));
    QObject::connect(Slider7, SIGNAL(valueChanged(int)), label7_2, SLOT(setNum(int)));

    connect(chordBox, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int v)
    {
        if(extChord) *extChord = v;
    });

    connect(Slider3, QOverload<int>::of(&QSlider::valueChanged), [=](int v)
    {
        if(extSlider3) *extSlider3 = v;
    });

    connect(Slider5, QOverload<int>::of(&QSlider::valueChanged), [=](int v)
    {
        if(extSlider5) *extSlider5 = v;
    });

    connect(Slider7, QOverload<int>::of(&QSlider::valueChanged), [=](int v)
    {
        if(extSlider7) *extSlider7 = v;
    });

    Slider3->setValue(0);
    Slider5->setValue(0);
    Slider7->setValue(0);

    QMetaObject::connectSlotsByName(chord);
}




