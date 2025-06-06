/*
 * MidiEditor
 * Copyright (C) 2010  Markus Schwenk
 *
 * tabMainVolume (FluidDialog)
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

#ifdef USE_FLUIDSYNTH

#include "FluidDialog.h"

#include "../gui/MainWindow.h"

#include "../MidiEvent/MidiEvent.h"
#include "../MidiEvent/TextEvent.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiFile.h"
#include "../protocol/Protocol.h"
#include "../VST/VST.h"

#include "../MidiEvent/SysExEvent.h"
#include "../midi/MidiFile.h"
#include "../midi/MidiChannel.h"
#include "../midi/MidiTrack.h"
#include "../protocol/Protocol.h"
#include "../tool/Selection.h"
#include "../tool/EventTool.h"
#include "../tool/NewNoteTool.h"

#include <QMessageBox>
#include <QDataStream>
#include <QIODevice>

class QVLabel : public QLabel
{


public:
    explicit QVLabel(const QString &text, QWidget *parent);

protected:
    void paintEvent(QPaintEvent*);

};

QVLabel::QVLabel(const QString &text, QWidget *parent=0)
        : QLabel(text, parent){
}

void QVLabel::paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.setClipping(false);
    painter.translate(8, this->height());
    painter.rotate(270);

    painter.drawText(0, 0, text());
}


QString groupIndicator2 = QString::fromUtf8("\nQGroupBox::indicator:hover {border: 1px solid #c0ffc0; background-color: #c0ffc0;}\n"
                            "QGroupBox::indicator:checked {border: 1px solid #b1b1b1; background-color: #00ff00;}\n"
                            "QGroupBox::indicator:unchecked {border: 1px solid #b1b1b1; background-color: #404040;}");
QString groupIndicator = QString::fromUtf8("\nQGroupBox::indicator:hover {border: 1px solid #c0ffc0; background-color: #c0ffc0;}\n"
                            "QGroupBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                            "QGroupBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n"
                            //"QGroupBox::title {subcontrol-origin: margin; left: 6px;\n"
                            //"subcontrol-position: top left; padding: 0 2px; }\n"
                            "QGroupBox::title {subcontrol-origin: margin; left: 0px;\n"
                            "subcontrol-position: top center; padding: 0 2px; background-color: transparent;}\n"
                            "QGroupBox QCheckBox::indicator:unchecked {image: url(:/run_environment/graphics/custom/unchecked.png);}\n"
                            "QGroupBox QCheckBox::indicator:checked {image: url(:/run_environment/graphics/custom/checked.png);}\n");


void FluidDialog::tab_MainVolume(QDialog */*FluidDialog*/){

    int expX = 240;
    int n;
    int area_size_y = 407;
    int buttons_y = 588;

    channel_selected = 0;

    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(tabWidget->sizePolicy().hasHeightForWidth());

    MainVolume = new QWidget();
    MainVolume->setObjectName(QString::fromUtf8("MainVolume"));

    if(disable_mainmenu) MainVolume->setDisabled(true);

    scrollArea = new QScrollArea(MainVolume);
    scrollArea->setObjectName(QString::fromUtf8("scrollArea"));
    scrollArea->setGeometry(QRect(4, 15, 554+60 + 120 + expX, area_size_y));
    sizePolicy.setHeightForWidth(scrollArea->sizePolicy().hasHeightForWidth());
    scrollArea->setSizePolicy(sizePolicy);
    scrollArea->setFocusPolicy(Qt::NoFocus);
    scrollArea->setContextMenuPolicy(Qt::NoContextMenu);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    scrollArea->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);

    scrollArea->setWidgetResizable(true);
    scrollArea->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);

    QScrollBar * s=scrollArea->horizontalScrollBar();

    s->setMinimum(0);
    s->setMaximum(600);

    scrollArea->setStyleSheet(QString::fromUtf8("color: white; background-color: #80103040;"));

    groupM = new QGroupBox(scrollArea);
    groupM->setObjectName(QString::fromUtf8("groupM"));
    groupM->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));
    groupM->setGeometry(QRect(0, 0, 3000, 600));

    groupE = new QGroupBox(MainVolume);
    groupE->setObjectName(QString::fromUtf8("groupE"));
    groupE->setGeometry(QRect(0, area_size_y+20, 752 + 125 + expX + NEWDESP, buttons_y - area_size_y-20));
    groupE->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));

    //QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    int x = 14;

    // get first track index for Fluidsynth channels
    int track_index[3] = {-1, -1, -1};

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        int r = MidiOutput::AllTracksToOne ? MidiOutput::_midiOutFluidMAP[0] : MidiOutput::_midiOutFluidMAP[n];
        if(r >= 0 && track_index[r % 3] < 0) track_index[r % 3] = n;
    }

    for(n = 0; n < SYNTH_CHANS; n++) {
        groupChan[n] = new QGroupBox(groupM/*scrollArea*/);
        if(n == channel_selected) {
            groupChan[n]->setStyleSheet(QString::fromUtf8("QGroupBox {background-color: #80303060;}\n") + groupIndicator);
        } else
            groupChan[n]->setStyleSheet(groupIndicator);

        groupChan[n]->setObjectName(QString::fromUtf8("groupChan")+QString::number(n));
        groupChan[n]->setGeometry(QRect(x, 2, 53, 371));
        groupChan[n]->setFocusPolicy(Qt::NoFocus);

        x+=60;
/*
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(groupChan[n]->sizePolicy().hasHeightForWidth());
*/
        groupChan[n]->setLayoutDirection(Qt::LeftToRight);
        groupChan[n]->setAlignment(Qt::AlignCenter);
        groupChan[n]->setFlat(false);
        groupChan[n]->setCheckable(true);
        groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
        groupChan[n]->setToolTip("<html><head/><body><p>Mute/Unmute</p>"
                                                  "<p>the channel</p></body></html>");
        line[n] = new QFrame(groupChan[n]);
        line[n]->setObjectName(QString::fromUtf8("line"));
        line[n]->setGeometry(QRect(27 + 6, 2 + 8, 16, 8));
        line[n]->setFrameShadow(QFrame::Plain);
        line[n]->setLineWidth(8);
        line[n]->setFrameShape(QFrame::HLine);
        //line[n]->setVisible(false);

        ChanVol[n] = new QSliderE(groupChan[n], 7, QSLIDER_BLUE);
        ChanVol[n]->setObjectName(QString::fromUtf8("ChanVol")+QString::number(n));
        ChanVol[n]->setGeometry(QRect(12 + 4, 37, 22, 160));
        ChanVol[n]->setMouseTracking(false);
        ChanVol[n]->setMaximum(100);
        ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
        ChanVol[n]->setOrientation(Qt::Vertical);
        //ChanVol[n]->setTickPosition(QSlider::TicksBothSides);
        //ChanVol[n]->setTickInterval(25);
        ChanVol[n]->setToolTip("<html><head/><body><p>Synth Expression Volume</p>"
                                                  "<p>of the channel</p></body></html>");

        BalanceSlider[n] = new QSliderE(groupChan[n], 5, QSLIDER_YELLOW_TINY);
        BalanceSlider[n]->setObjectName(QString::fromUtf8("BalanceSlider")+QString::number(n));
        BalanceSlider[n]->setGeometry(QRect(8 + 4, 212, 30, 22));
        BalanceSlider[n]->setInputMethodHints(Qt::ImhNone);
        BalanceSlider[n]->setMinimum(-100);
        BalanceSlider[n]->setMaximum(100);
        BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
        BalanceSlider[n]->setTracking(true);
        BalanceSlider[n]->setOrientation(Qt::Horizontal);
        //BalanceSlider[n]->setTickPosition(QSlider::TicksAbove);
        //BalanceSlider[n]->setTickInterval(50);
        BalanceSlider[n]->setToolTip("<html><head/><body><p>Output balance control</p>"
                                          "<p>of the channel</p></body></html>");

        BalanceLabel[n] = new QLabel(groupChan[n]);
        BalanceLabel[n]->setObjectName(QString::fromUtf8("BalanceLabel")+QString::number(n));
        BalanceLabel[n]->setGeometry(QRect(5 + 4, 243, 36, 22));
        BalanceLabel[n]->setToolTip("<html><head/><body><p>Output balance control</p>"
                                                  "<p>of the channel</p></body></html>");

        BalanceLabel[n]->setInputMethodHints(Qt::ImhNone);
        BalanceLabel[n]->setFrameShape(QFrame::Box);
        BalanceLabel[n]->setFrameShadow(QFrame::Raised);
        BalanceLabel[n]->setTextFormat(Qt::AutoText);
        BalanceLabel[n]->setAlignment(Qt::AlignCenter);
        BalanceLabel[n]->setWordWrap(true);

        Chan[n] = new QLabel(groupChan[n]);
        Chan[n]->setObjectName(QString::fromUtf8("Chan")+QString::number(n));
        Chan[n]->setGeometry(QRect(2 + 4, 17, 40, 13));
        Chan[n]->setAlignment(Qt::AlignCenter);

        label = new QLabel(groupChan[n]);
        label->setObjectName(QString::fromUtf8("label")+QString::number(n));
        label->setGeometry(QRect(7 + 4, 270, 31, 16));
        label->setAlignment(Qt::AlignCenter);

        chanGainLabel[n] = new QLabel(groupChan[n]);
        chanGainLabel[n]->setObjectName(QString::fromUtf8("chanGainLabel")+QString::number(n));
        chanGainLabel[n]->setGeometry(QRect(6 + 4, 340, 33, 21));
        chanGainLabel[n]->setFrameShape(QFrame::Box);
        chanGainLabel[n]->setFrameShadow(QFrame::Raised);
        chanGainLabel[n]->setAlignment(Qt::AlignCenter);
        chanGainLabel[n]->setToolTip("<html><head/><body><p>Output control gain</p>"
                                      "<p>of the channel</p></body></html>");

        chanGain[n] = new QDialE(groupChan[n], QDIALE_GRAY_NOTCH_WHITEDOT, 10);
        chanGain[n]->setObjectName(QString::fromUtf8("chanGain")+QString::number(n));
        chanGain[n]->setGeometry(QRect(-3 + 4, 270, 50, 91));
        chanGain[n]->setMinimum(-1000);
        chanGain[n]->setMaximum(1000);
        chanGain[n]->setValue(fluid_output->getAudioGain(n));
        chanGain[n]->setPageStep(50);
        chanGain[n]->setNotchTarget(100.0);
        chanGain[n]->setNotchesVisible(true);
        chanGain[n]->setToolTip("<html><head/><body><p>Output control gain</p>"
                                  "<p>of the channel</p></body></html>");


        int nn = (n & 15) + 32 * (n/16);

        QIcon ico(":/run_environment/graphics/channelwidget/vst_effect.png");
        wicon[nn] = new QPushButton(groupM);
        wicon[nn]->setObjectName(QString::asprintf("wicon %i", nn));
        wicon[nn]->setGeometry(QRect(x - 60 + 18 - 10, 374, 16, 16));
        wicon[nn]->setStyleSheet(QString::fromUtf8("background-color: white;"));
        wicon[nn]->setIcon(ico);

        if(VST_proc::VST_isLoaded(nn))
            wicon[nn]->setVisible(true);
        else
            wicon[nn]->setVisible(false);

        wicon[nn + 16] = new QPushButton(groupM);
        wicon[nn + 16]->setObjectName(QString::asprintf("wicon %i", nn + 16));
        wicon[nn + 16]->setGeometry(QRect(x - 60 + 18 + 10, 374, 16, 16));
        wicon[nn + 16]->setStyleSheet(QString::fromUtf8("background-color: white;"));
        wicon[nn + 16]->setIcon(ico);

        if(VST_proc::VST_isLoaded(nn + 16))
            wicon[nn + 16]->setVisible(true);
        else
            wicon[nn + 16]->setVisible(false);

        groupChan[n]->setTitle(QString());
        const QString str= "Ch "+QString::number(n);

        Chan[n]->setText(str);
        BalanceLabel[n]->setText(QString().setNum(((float)(fluid_output->getAudioBalance(n) / 10)/10.0), 'f', 2));
        label->setText("Gain");
        chanGainLabel[n]->setText(QString().setNum(((float)(fluid_output->getAudioGain(n) / 10))/10.0, 'f', 2));

        int indx = track_index[n/16] >= 0 ? ((n & 15) + 4 * (track_index[n/16] > 0)
                + 16 * track_index[n/16]) : (n & 15);


        int _bank = MidiOutput::file->Bank_MIDI[indx];
        int _instrument = MidiOutput::file->Prog_MIDI[indx];

        QString s;

        if(track_index[n/16] >= 0) {

            bool is_instrument = false;

            if((n  & 15) != 9 || ((n & 15) == 9 && (n > 9) && track_index[n/16] > 0
                          && !MidiOutput::file->DrumUseCh9))
                is_instrument = true;

            if(is_instrument)
                s = MidiFile::instrumentName( _bank, _instrument);
            else
                s = MidiFile::drumName(_instrument);
        }

        qv[n] = new QVLabel(s, groupChan[n]);
        qv[n]->setObjectName(QString::fromUtf8("chanInstrum")+QString::number(n));
        qv[n]->setGeometry(QRect(4, 37, 12, 160));
    }

    QGroupBox *groupV = new QGroupBox(MainVolume);
    groupV->setObjectName(QString::fromUtf8("groupV"));
    groupV->setGeometry(QRect(590+50 + 125 + expX - 16 + NEWDESP, 15, 127, area_size_y - 122));
    groupV->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n") + groupIndicator);

    groupMainVol = new QGroupBox(groupV);
    groupMainVol->setObjectName(QString::fromUtf8("groupMainVol"));
    groupMainVol->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));

    groupMainVol->setGeometry(QRect(8, 2, 45, 275));
    groupMainVol->setLayoutDirection(Qt::LeftToRight);
    groupMainVol->setAlignment(Qt::AlignCenter);
    groupMainVol->setFlat(false);
    groupMainVol->setCheckable(true);
    groupMainVol->setTitle(QString());

    QLabel * MainVolVal = new QLabel(groupMainVol);
    MainVolVal->setObjectName(QString::fromUtf8("MainVolVal"));
    MainVolVal->setGeometry(QRect(8, 246, 30, 16));
    MainVolVal->setAlignment(Qt::AlignCenter);
    MainVolVal->setStyleSheet(QString::fromUtf8("color: black; background: white"));
    MainVolVal->setText(QString::number(((double) fluid_output->getSynthGain())/100, 'f', 2));

    MainVol = new QSliderE(groupMainVol, 9, QSLIDER_GREEN);
    MainVol->setObjectName(QString::fromUtf8("MainVol"));
    MainVol->setGeometry(QRect(12, 37, 22, 200));
    MainVol->setMouseTracking(false);
    MainVol->setMaximum(200);
    MainVol->setMinimum(0);
    MainVol->setValue(fluid_output->getSynthGain());
    MainVol->setOrientation(Qt::Vertical);
    //MainVol->setTickPosition(QSlider::TicksBothSides);
    //MainVol->setTickInterval(25);
    MainVol->setToolTip("<html><head/><body><p>Synth Gain</p></body></html>");

    groupVUm = new QGroupBox(groupV);
    groupVUm->setObjectName(QString::fromUtf8("groupVUm"));
    groupVUm->setStyleSheet(QString::fromUtf8("QGroupBox QToolTip {color: black;} \n"));
    groupVUm->setGeometry(QRect(66, 36, 45, 206));
    groupVUm->setLayoutDirection(Qt::LeftToRight);
    groupVUm->setAlignment(Qt::AlignCenter);
    groupVUm->setFlat(false);
    groupVUm->setCheckable(false);
    groupVUm->setTitle(QString());
    groupVUm->setToolTip("<html><head/><body><p>VU Meter</p></body></html>");

    for(n = 0; n < 25; n++) {
        line_l[n] = new QFrame(groupVUm);
        line_l[n]->setObjectName(QString::fromUtf8("line_l")+QString::number(n));
        line_l[n]->setGeometry(QRect(2, 4+200*n/25, 20, 5));
        line_l[n]->setStyleSheet(QString::fromUtf8("color: #408000;"));
        line_l[n]->setFrameShadow(QFrame::Plain);
        line_l[n]->setLineWidth(18);
        line_l[n]->setFrameShape(QFrame::VLine);

        line_r[n] = new QFrame(groupVUm);
        line_r[n]->setObjectName(QString::fromUtf8("line_r")+QString::number(n));
        line_r[n]->setGeometry(QRect(23, 4+200*n/25, 20, 5));
        line_r[n]->setStyleSheet(QString::fromUtf8("color: #608000;"));
        line_r[n]->setFrameShadow(QFrame::Plain);
        line_r[n]->setLineWidth(18);
        line_r[n]->setFrameShape(QFrame::VLine);
    }


    Main = new QLabel(groupMainVol);
    Main->setObjectName(QString::fromUtf8("Main"));
    Main->setGeometry(QRect(2, 20, 41, 16));
    Main->setAlignment(Qt::AlignCenter);
    Main->setText("Main");

// effects

    labelChan = new QLabel(groupE);
    labelChan->setObjectName(QString::fromUtf8("labelChan"));
    labelChan->setGeometry(QRect(9, 14, 48, 16));
    labelChan->setAutoFillBackground(true);
    labelChan->setFrameShape(QFrame::Panel);
    labelChan->setAlignment(Qt::AlignCenter);
    labelChan->setText(QString("Chan ")+QString().setNum(channel_selected));
    labelChan->setToolTip("<html><head/><body><p>Channel selected</p></body></html>");

    spinChan = new QSpinBox(groupE);
    spinChan->setObjectName(QString::fromUtf8("spinChan"));
    spinChan->setToolTip("<html><head/><body><p>Current chan for Distortion, Low/High Cut...</p></body></html>");
    spinChan->setGeometry(QRect(11, 40, 45, 29));
    QFont font2;
    font2.setPointSize(16);
    spinChan->setFont(font2);
    spinChan->setAlignment(Qt::AlignCenter);
    spinChan->setMaximum(47);
    spinChan->setValue(channel_selected);
    spinChan->setAutoFillBackground(true);
    spinChan->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));

    labelGroupIndex = new QLabel(groupE);
    labelGroupIndex->setObjectName(QString::fromUtf8("labelGroupIndex"));
    labelGroupIndex->setGeometry(QRect(9, 75, 48, 16));
    labelGroupIndex->setAutoFillBackground(true);
    labelGroupIndex->setFrameShape(QFrame::Panel);
    labelGroupIndex->setAlignment(Qt::AlignCenter);
    labelGroupIndex->setText(QString("Index"));
    labelGroupIndex->setToolTip("Current group index of 16 channels");

    spinGroupIndex = new QSpinBox(groupE);
    spinGroupIndex->setObjectName(QString::fromUtf8("spinGroupIndex"));
    spinGroupIndex->setToolTip("Current group index of 16 channels");
    spinGroupIndex->setGeometry(QRect(11, 101, 45, 29));

    //QFont font2;
    font2.setPointSize(16);
    spinGroupIndex->setFont(font2);
    spinGroupIndex->setAlignment(Qt::AlignCenter);
    spinGroupIndex->setMaximum(2);
    spinGroupIndex->setValue(channel_selected/16);
    spinGroupIndex->setAutoFillBackground(true);
    spinGroupIndex->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));

    DistortionBox = new QGroupBox(groupE);
    DistortionBox->setObjectName(QString::fromUtf8("DistortionBox"));
    DistortionBox->setGeometry(QRect(75, 4, 58, 127));
    DistortionBox->setLayoutDirection(Qt::LeftToRight);
    DistortionBox->setAlignment(Qt::AlignCenter);
    DistortionBox->setFlat(false);
    label_dist_gain = new QLabel(DistortionBox);
    label_dist_gain->setObjectName(QString::fromUtf8("label_dist_gain"));
    label_dist_gain->setGeometry(QRect(12, 35, 31, 16));
    label_dist_gain->setAlignment(Qt::AlignCenter);
    label_dist_gain->setToolTip("<html><head/><body><p>Distortion Input Gain</p>"
                            "<p>of the current channel</p></body></html>");

    label_distortion_disp = new QLabel(DistortionBox);
    label_distortion_disp->setObjectName(QString::fromUtf8("label_distortion_disp"));
    label_distortion_disp->setGeometry(QRect(12, 100, 33, 21));
    label_distortion_disp->setFrameShape(QFrame::Box);
    label_distortion_disp->setFrameShadow(QFrame::Raised);
    label_distortion_disp->setAlignment(Qt::AlignCenter);
    label_distortion_disp->setToolTip("<html><head/><body><p>Distortion Input Gain</p>"
                                        "<p>of the current channel</p></body></html>");

    DistortionGain = new QDialE(DistortionBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    DistortionGain->setObjectName(QString::fromUtf8("DistortionGain"));
    DistortionGain->setGeometry(QRect(3, 41, 50, 71));
    DistortionGain->setMinimum(0);
    DistortionGain->setMaximum(300);
    DistortionGain->setValue(
                fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                channel_selected, GET_FILTER_GAIN));
    DistortionGain->setNotchTarget(20.0);
    DistortionGain->setNotchesVisible(true);
    DistortionGain->setToolTip("<html><head/><body><p>Distortion Input Gain</p>"
                                 "<p>of the current channel</p></body></html>");

    DistortionButton = new QPushButton(DistortionBox);
    DistortionButton->setObjectName(QString::fromUtf8("DistortionButton"));
    DistortionButton->setGeometry(QRect(15, 18, 29, 16));
    DistortionButton->setCheckable(true);
    DistortionButton->setAutoDefault(false);
    DistortionButton->setFlat(false);
    DistortionButton->setDefault(true);
    DistortionButton->setChecked(
               (fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                channel_selected, GET_FILTER_ON)!=0) ? true : false);
    DistortionButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));
    DistortionButton->setToolTip("<html><head/><body><p>Distortion On/Off</p>"
                                   "<p>of the current channel</p></body></html>");

    DistortionBox->setTitle("Distortion");
    label_dist_gain->setText("Level");
    label_distortion_disp->setText(QString().setNum(fluid_output->filter_dist_gain[channel_selected]));
    DistortionButton->setText(QString());

    int lowbox = 75 + 69;

    LowCutBox = new QGroupBox(groupE);
    LowCutBox->setObjectName(QString::fromUtf8("LowCutBox"));
    LowCutBox->setGeometry(QRect(lowbox, 4, 174, 127));
    LowCutBox->setLayoutDirection(Qt::LeftToRight);
    LowCutBox->setAlignment(Qt::AlignCenter);
    LowCutBox->setFlat(false);

    label_low_gain = new QLabel(LowCutBox);
    label_low_gain->setObjectName(QString::fromUtf8("label_low_gain"));
    label_low_gain->setGeometry(QRect(12, 35, 31, 16));
    label_low_gain->setAlignment(Qt::AlignCenter);
    label_low_gain->setToolTip("<html><head/><body><p>Low Cut Filter Gain</p>"
                                 "<p>of the current channel</p></body></html>");

    label_low_freq = new QLabel(LowCutBox);
    label_low_freq->setObjectName(QString::fromUtf8("label_low_freq"));
    label_low_freq->setGeometry(QRect(71, 35, 31, 16));
    label_low_freq->setAlignment(Qt::AlignCenter);
    label_low_freq->setToolTip("<html><head/><body><p>Low Cut Filter Frequency</p>"
                                     "<p>of the current channel</p></body></html>");

    label_low_res = new QLabel(LowCutBox);
    label_low_res->setObjectName(QString::fromUtf8("label_low_res"));
    label_low_res->setGeometry(QRect(76+57, 35, 31, 16));
    label_low_res->setToolTip("<html><head/><body><p>Low Cut Filter Resonance</p>"
                                     "<p>of the current channel</p></body></html>");

    label_low_disp = new QLabel(LowCutBox);
    label_low_disp->setObjectName(QString::fromUtf8("label_low_disp"));
    label_low_disp->setGeometry(QRect(12, 100, 33, 21));
    label_low_disp->setFrameShape(QFrame::Box);
    label_low_disp->setFrameShadow(QFrame::Raised);
    label_low_disp->setAlignment(Qt::AlignCenter);

    LowCutGain = new QDialE(LowCutBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LowCutGain->setObjectName(QString::fromUtf8("LowCutGain"));
    LowCutGain->setGeometry(QRect(3, 41, 50, 71));
    LowCutGain->setMinimum(0);
    LowCutGain->setMaximum(200);
    LowCutGain->setValue(
                fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                channel_selected, GET_FILTER_GAIN));
    LowCutGain->setNotchTarget(20.0);
    LowCutGain->setNotchesVisible(true);
    LowCutGain->setToolTip("<html><head/><body><p>Low Cut Filter Gain</p>"
                                 "<p>of the current channel</p></body></html>");


    label_low_disp2 = new QLabel(LowCutBox);
    label_low_disp2->setObjectName(QString::fromUtf8("label_low_disp2"));
    label_low_disp2->setGeometry(QRect(69, 100, 33, 21));
    label_low_disp2->setFrameShape(QFrame::Box);
    label_low_disp2->setFrameShadow(QFrame::Raised);
    label_low_disp2->setAlignment(Qt::AlignCenter);

    LowCutFreq = new QDialE(LowCutBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LowCutFreq->setObjectName(QString::fromUtf8("LowCutFreq"));
    LowCutFreq->setGeometry(QRect(60, 41, 50, 71));
    LowCutFreq->setMinimum(50);
    LowCutFreq->setMaximum(2500);
    LowCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                        channel_selected, GET_FILTER_FREQ));
    LowCutFreq->setNotchTarget(200.0);
    LowCutFreq->setNotchesVisible(true);
    LowCutFreq->setToolTip("<html><head/><body><p>Low Cut Filter Frequency</p>"
                                         "<p>of the current channel</p></body></html>");

    label_low_disp3 = new QLabel(LowCutBox);
    label_low_disp3->setObjectName(QString::fromUtf8("label_low_disp3"));
    label_low_disp3->setGeometry(QRect(69+57, 100, 33, 21));
    label_low_disp3->setFrameShape(QFrame::Box);
    label_low_disp3->setFrameShadow(QFrame::Raised);
    label_low_disp3->setAlignment(Qt::AlignCenter);

    LowCutRes = new QDialE(LowCutBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LowCutRes->setObjectName(QString::fromUtf8("LowCutRes"));
    LowCutRes->setGeometry(QRect(60+57, 41, 50, 71));
    LowCutRes->setMinimum(0);
    LowCutRes->setMaximum(250);
    LowCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                        channel_selected, GET_FILTER_RES));
    LowCutRes->setNotchTarget(28.0);
    LowCutRes->setNotchesVisible(true);
    LowCutRes->setToolTip("<html><head/><body><p>Low Cut Filter Resonance</p>"
                                         "<p>of the current channel</p></body></html>");

    LowCutButton = new QPushButton(LowCutBox);
    LowCutButton->setObjectName(QString::fromUtf8("LowCutButton"));

    LowCutButton->setGeometry(QRect(69, 18, 29, 16));
    LowCutButton->setCheckable(true);
    LowCutButton->setAutoDefault(false);
    LowCutButton->setDefault(true);
    LowCutButton->setChecked(
               (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                channel_selected, GET_FILTER_ON)!=0) ? true : false);
    LowCutButton->setText(QString());
    LowCutButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));
    LowCutButton->setToolTip("<html><head/><body><p>Low Cut Filter On/Off</p>"
                                         "<p>of the current channel</p></body></html>");

    LowCutBox->setTitle("Low Cut");
    label_low_gain->setText("Gain");
    label_low_freq->setText("Freq");
    label_low_res->setText("Res");
    label_low_disp->setText(QString().setNum(((float)(fluid_output->filter_locut_gain[channel_selected] / 10)/10.0), 'f', 2));

    label_low_disp2->setText(QString().setNum(LowCutFreq->value()));
    label_low_disp3->setText(QString().setNum(((float)(fluid_output->filter_locut_res[channel_selected] / 10)/10.0), 'f', 2));

    HighCutBox = new QGroupBox(groupE);
    HighCutBox->setObjectName(QString::fromUtf8("HighCutBox"));
    lowbox+= 185;
    HighCutBox->setGeometry(QRect(lowbox, 4, 174, 127));
    HighCutBox->setLayoutDirection(Qt::LeftToRight);
    HighCutBox->setAlignment(Qt::AlignCenter);
    HighCutBox->setFlat(false);

    label_high_gain = new QLabel(HighCutBox);
    label_high_gain->setObjectName(QString::fromUtf8("label_high_gain"));
    label_high_gain->setGeometry(QRect(12, 35, 31, 16));
    label_high_gain->setAlignment(Qt::AlignCenter);
    label_high_gain->setToolTip("<html><head/><body><p>High Cut Filter Gain</p>"
                                 "<p>of the current channel</p></body></html>");

    label_high_freq = new QLabel(HighCutBox);
    label_high_freq->setObjectName(QString::fromUtf8("label_high_freq"));
    label_high_freq->setGeometry(QRect(69, 35, 31, 16));
    label_high_freq->setAlignment(Qt::AlignCenter);
    label_high_freq->setToolTip("<html><head/><body><p>High Cut Filter Frequency</p>"
                                             "<p>of the current channel</p></body></html>");

    label_high_res = new QLabel(HighCutBox);
    label_high_res->setObjectName(QString::fromUtf8("label_high_res"));
    label_high_res->setGeometry(QRect(69+57, 35, 31, 16));
    label_high_res->setAlignment(Qt::AlignCenter);
    label_high_res->setToolTip("<html><head/><body><p>High Cut Filter Resonance</p>"
                                             "<p>of the current channel</p></body></html>");

    label_high_disp = new QLabel(HighCutBox);
    label_high_disp->setObjectName(QString::fromUtf8("label_high_disp"));
    label_high_disp->setGeometry(QRect(12, 100, 33, 21));
    label_high_disp->setFrameShape(QFrame::Box);
    label_high_disp->setFrameShadow(QFrame::Raised);
    label_high_disp->setAlignment(Qt::AlignCenter);

    HighCutGain = new QDialE(HighCutBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    HighCutGain->setObjectName(QString::fromUtf8("HighCutGain"));
    HighCutGain->setGeometry(QRect(3, 41, 50, 71));
    HighCutGain->setMinimum(0);
    HighCutGain->setMaximum(200);
    HighCutGain->setValue(
                fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                channel_selected, GET_FILTER_GAIN));
    HighCutGain->setNotchTarget(20.0);
    HighCutGain->setNotchesVisible(true);
    HighCutGain->setToolTip("<html><head/><body><p>High Cut Filter Gain</p>"
                                     "<p>of the current channel</p></body></html>");


    label_high_disp2 = new QLabel(HighCutBox);
    label_high_disp2->setObjectName(QString::fromUtf8("label_high_disp2"));
    label_high_disp2->setGeometry(QRect(69, 100, 33, 21));
    label_high_disp2->setFrameShape(QFrame::Box);
    label_high_disp2->setFrameShadow(QFrame::Raised);
    label_high_disp2->setAlignment(Qt::AlignCenter);

    HighCutFreq = new QDialE(HighCutBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    HighCutFreq->setObjectName(QString::fromUtf8("HighCutFreq"));
    HighCutFreq->setGeometry(QRect(60, 41, 50, 71));
    HighCutFreq->setMinimum(500);
    HighCutFreq->setMaximum(5000);
    HighCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                        channel_selected, GET_FILTER_FREQ));
    HighCutFreq->setNotchTarget(400.0);
    HighCutFreq->setNotchesVisible(true);
    HighCutFreq->setToolTip("<html><head/><body><p>High Cut Filter Frequency</p>"
                              "<p>of the current channel</p></body></html>");


    label_high_disp3 = new QLabel(HighCutBox);
    label_high_disp3->setObjectName(QString::fromUtf8("label_high_disp3"));
    label_high_disp3->setGeometry(QRect(69+57, 100, 33, 21));
    label_high_disp3->setFrameShape(QFrame::Box);
    label_high_disp3->setFrameShadow(QFrame::Raised);
    label_high_disp3->setAlignment(Qt::AlignCenter);

    HighCutRes = new QDialE(HighCutBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    HighCutRes->setObjectName(QString::fromUtf8("HighCutRes"));
    HighCutRes->setGeometry(QRect(60+57, 41, 50, 71));
    HighCutRes->setMinimum(0);
    HighCutRes->setMaximum(250);
    HighCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                                                        channel_selected, GET_FILTER_RES));
    HighCutRes->setNotchTarget(28.0);
    HighCutRes->setNotchesVisible(true);
    HighCutRes->setToolTip("<html><head/><body><p>High Cut Filter Resonance</p>"
                                                 "<p>of the current channel</p></body></html>");

    HighCutButton = new QPushButton(HighCutBox);
    HighCutButton->setObjectName(QString::fromUtf8("HighCutButton"));
    HighCutButton->setGeometry(QRect(70, 18, 29, 16));
    HighCutButton->setCheckable(true);
    HighCutButton->setAutoDefault(false);
    HighCutButton->setDefault(true);
    HighCutButton->setChecked((fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
               channel_selected, GET_FILTER_ON)!=0) ? true : false);
    HighCutButton->setToolTip("<html><head/><body><p>High Cut Filter On/Off</p>"
                                             "<p>of the current channel</p></body></html>");

    HighCutBox->setTitle("High Cut");

    label_high_gain->setText("Gain");
    label_high_freq->setText("Freq");
    label_high_res->setText("Res");
    label_high_disp->setText(QString().setNum(((float)(fluid_output->filter_hicut_gain[channel_selected] / 10)/10.0), 'f', 2));
    label_high_disp2->setText(QString().setNum(HighCutFreq->value()));
    label_high_disp3->setText(QString().setNum(((float)(fluid_output->filter_hicut_res[channel_selected] / 10)/10.0), 'f', 2));

    HighCutButton->setText(QString());
    HighCutButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));

    //&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&&

    TremoloBox = new QGroupBox(groupE);
    TremoloBox->setObjectName(QString::fromUtf8("TremoloBox"));
    lowbox+= 185;
    TremoloBox->setGeometry(QRect(lowbox, 4, 114, 127));
    TremoloBox->setLayoutDirection(Qt::LeftToRight);
    TremoloBox->setAlignment(Qt::AlignCenter);
    TremoloBox->setFlat(false);

    label_trem_level = new QLabel(TremoloBox);
    label_trem_level->setObjectName(QString::fromUtf8("label_trem_level"));
    label_trem_level->setGeometry(QRect(12, 35, 31, 16));
    label_trem_level->setAlignment(Qt::AlignCenter);
    label_trem_level->setToolTip("<html><head/><body><p>Tremolo Level</p>"
                                 "<p>of the current channel</p></body></html>");

    label_trem_freq = new QLabel(TremoloBox);
    label_trem_freq->setObjectName(QString::fromUtf8("label_trem_freq"));
    label_trem_freq->setGeometry(QRect(69, 35, 31, 16));
    label_trem_freq->setAlignment(Qt::AlignCenter);
    label_trem_freq->setToolTip("<html><head/><body><p>Tremolo Frequency</p>"
                                             "<p>of the current channel</p></body></html>");

    label_trem_disp = new QLabel(TremoloBox);
    label_trem_disp->setObjectName(QString::fromUtf8("label_trem_disp"));
    label_trem_disp->setGeometry(QRect(12, 100, 33, 21));
    label_trem_disp->setFrameShape(QFrame::Box);
    label_trem_disp->setFrameShadow(QFrame::Raised);
    label_trem_disp->setAlignment(Qt::AlignCenter);

    TremoloLevel = new QDialE(TremoloBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    TremoloLevel->setObjectName(QString::fromUtf8("TremoloLevel"));
    TremoloLevel->setGeometry(QRect(3, 41, 50, 71));
    TremoloLevel->setMinimum(0);
    TremoloLevel->setMaximum(50);
    TremoloLevel->setValue((int) (100.0f * fluid_output->level_WaveModulator[channel_selected]));
    TremoloLevel->setNotchTarget(4.0);
    TremoloLevel->setNotchesVisible(true);
    TremoloLevel->setToolTip("<html><head/><body><p>Tremolo Level</p>"
                                     "<p>of the current channel</p></body></html>");

    label_trem_disp2 = new QLabel(TremoloBox);
    label_trem_disp2->setObjectName(QString::fromUtf8("label_trem_disp2"));
    label_trem_disp2->setGeometry(QRect(69, 100, 33, 21));
    label_trem_disp2->setFrameShape(QFrame::Box);
    label_trem_disp2->setFrameShadow(QFrame::Raised);
    label_trem_disp2->setAlignment(Qt::AlignCenter);

    TremoloFreq = new QDialE(TremoloBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    TremoloFreq->setObjectName(QString::fromUtf8("TremoloFreq"));
    TremoloFreq->setGeometry(QRect(60, 41, 50, 71));
    TremoloFreq->setMinimum(10);
    TremoloFreq->setMaximum(1500);
    TremoloFreq->setValue((int) (100.0f * fluid_output->freq_WaveModulator[channel_selected]));
    TremoloFreq->setNotchTarget(100.0);
    TremoloFreq->setNotchesVisible(true);
    TremoloFreq->setToolTip("<html><head/><body><p>Tremolo Frequency Frequency</p>"
                              "<p>of the current channel</p></body></html>");


    TremoloBox->setTitle("Tremolo");

    label_trem_level->setText("Level");
    label_trem_freq->setText("Freq");

    label_trem_disp->setText(QString().setNum(((float)(100.0f * fluid_output->level_WaveModulator[channel_selected])/100.0), 'f', 2));
    label_trem_disp2->setText(QString().setNum(((float) TremoloFreq->value())/100, 'f', 2));

    ///////////////////////////v
    // see
    /*
    QGroupBox *groupV = new QGroupBox(MainVolume);
    groupV->setObjectName(QString::fromUtf8("groupV"));
    groupV->setGeometry(QRect(590+50 + 125 + expX - 16 + NEWDESP, 15, 125, area_size_y));
    */
    QPushButton *g = new QPushButton(MainVolume/*groupE*/);

    g->setGeometry(QRect(590+50 + 125 + expX - 18, area_size_y - 120 + 16 , 129 + NEWDESP , 120));
    //g->setGeometry(QRect(lowbox +125, 4 + 6, 161+70, 128 - 8));
    g->setLayoutDirection(Qt::LeftToRight);
    g->setFlat(false);

    static QPixmap pix(161+70, 128);
    static int one = 1;
    if(one) {
        one = 0;
        QPainter pixPaint(&pix);
        QPixmap p1(":/run_environment/graphics/custom/Midicustom.png");
        QPixmap p2(":/run_environment/graphics/icon.png");
        p2 =  p2.scaled(45, 45);
        p1 =  p1.scaled(/*184*/127 + 30, 60);

        pix.fill(QColor(0xc0c0a0));
        pixPaint.drawPixmap(20, 10 + 20, p2);
        pixPaint.drawPixmap(58, 0 + 20, p1);
    }

    QIcon ico(pix);

    g->setIcon(ico);
    g->setIconSize(QSize(161+70, 128));

    //////////////////&&

    LeslieBox = new QGroupBox(groupE);
    LeslieBox->setObjectName(QString::fromUtf8("LeslieBox"));
    //lowbox+= 185;
    //lowbox+= 185;
    //TremoloBox->setGeometry(QRect(lowbox, 4, 114, 127));
    LeslieBox->setGeometry(QRect(lowbox + 124, 4, 174 * 2 - 57, 127));
    LeslieBox->setLayoutDirection(Qt::LeftToRight);
    LeslieBox->setAlignment(Qt::AlignCenter);
    LeslieBox->setFlat(false);
    LeslieBox->setTitle("Leslie Effect");

    label_leslie_deepB = new QLabel(LeslieBox);
    label_leslie_deepB->setObjectName(QString::fromUtf8("label_leslie_deepB"));
    label_leslie_deepB->setGeometry(QRect(12, 35, 31, 16));
    label_leslie_deepB->setAlignment(Qt::AlignCenter);
    label_leslie_deepB->setToolTip("<html><head/><body><p>Leslie Bass Deep Level</p>"
                                "<p>of the current channel</p></body></html>");
    label_leslie_deepB->setText("Deep");

    label_leslie_rotationB = new QLabel(LeslieBox);
    label_leslie_rotationB->setObjectName(QString::fromUtf8("label_leslie_rotationB"));
    label_leslie_rotationB->setGeometry(QRect(69, 35, 31, 16));
    //label_leslie_rotationB->setGeometry(QRect(69+57, 35, 31, 16));
    label_leslie_rotationB->setAlignment(Qt::AlignCenter);
    label_leslie_rotationB->setToolTip("<html><head/><body><p>Leslie Bass Rotation</p>"
                               "<p>of the current channel</p></body></html>");
    label_leslie_rotationB->setText("Rotat");

    label_leslie_dispB = new QLabel(LeslieBox);
    label_leslie_dispB->setObjectName(QString::fromUtf8("label_leslie_dispB"));
    label_leslie_dispB->setGeometry(QRect(12, 100, 33, 21));
    label_leslie_dispB->setFrameShape(QFrame::Box);
    label_leslie_dispB->setFrameShadow(QFrame::Raised);
    label_leslie_dispB->setAlignment(Qt::AlignCenter);
    label_leslie_dispB->setText(QString().setNum(VST_proc::leslie[channel_selected].depthBass * 2.0f, 'f', 2));

    LeslieDeepB = new QDialE(LeslieBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LeslieDeepB->setObjectName(QString::fromUtf8("LeslieDeepB"));
    LeslieDeepB->setGeometry(QRect(3, 41, 50, 71));
    LeslieDeepB->setMinimum(0);
    LeslieDeepB->setMaximum(100);
    LeslieDeepB->setValue(VST_proc::leslie[channel_selected].depthBass * 200.0f);

    connect(LeslieDeepB, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_leslie_dispB->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));
        VST_proc::leslie[channel_selected].depthBass = ((float) num) / 200.0f;
    });

    LeslieDeepB->setNotchTarget(10.0);
    LeslieDeepB->setNotchesVisible(true);
    LeslieDeepB->setToolTip("<html><head/><body><p>Leslie Bass Deep Level</p>"
                            "<p>of the current channel</p></body></html>");

    label_leslie_dispB3 = new QLabel(LeslieBox);
    label_leslie_dispB3->setObjectName(QString::fromUtf8("label_leslie_dispB3"));
    label_leslie_dispB3->setGeometry(QRect(69, 100, 33, 21));
    //label_leslie_dispB3->setGeometry(QRect(69+57, 100, 33, 21));
    label_leslie_dispB3->setFrameShape(QFrame::Box);
    label_leslie_dispB3->setFrameShadow(QFrame::Raised);
    label_leslie_dispB3->setAlignment(Qt::AlignCenter);
    label_leslie_dispB3->setText(QString().setNum((VST_proc::leslie[channel_selected].rotationSpeedBass), 'f', 2));

    LeslieRotationB = new QDialE(LeslieBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LeslieRotationB->setObjectName(QString::fromUtf8("LeslieRotationB"));
    LeslieRotationB->setGeometry(QRect(60, 41, 50, 71));
    //LeslieRotationB->setGeometry(QRect(60+57, 41, 50, 71));
    LeslieRotationB->setMinimum(1);
    LeslieRotationB->setMaximum(200);
    LeslieRotationB->setValue(VST_proc::leslie[channel_selected].rotationSpeedBass * 10.0f);
    LeslieRotationB->setNotchTarget(14.0);
    LeslieRotationB->setNotchesVisible(true);
    LeslieRotationB->setToolTip("<html><head/><body><p>Leslie Bass Rotation</p>"
                           "<p>of the current channel</p></body></html>");

    connect(LeslieRotationB, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_leslie_dispB3->setText(QString().setNum(((float)(num)/10.0), 'f', 2));
        VST_proc::leslie[channel_selected].rotationSpeedBass = ((float) num) / 10.0f;
    });

    label_leslie_Bass = new QLabel(LeslieBox);
    label_leslie_Bass->setObjectName(QString::fromUtf8("label_leslie_Bass"));
    label_leslie_Bass->setGeometry(QRect(70 - 31, 18, 31, 16));
    label_leslie_Bass->setAlignment(Qt::AlignCenter);
    label_leslie_Bass->setToolTip("<html><head/><body><p>Leslie bass</p>"
                                  "<p>of the current channel</p></body></html>");
    label_leslie_Bass->setText("Bass");

    //&&

    label_leslie_dispT2 = new QLabel(LeslieBox);
    label_leslie_dispT2->setObjectName(QString::fromUtf8("label_leslie_dispT2"));
    label_leslie_dispT2->setGeometry(QRect(69+57, 100, 33, 21));
    //label_leslie_dispT2->setGeometry(QRect(174 + 69, 100, 33, 21));
    label_leslie_dispT2->setFrameShape(QFrame::Box);
    label_leslie_dispT2->setFrameShadow(QFrame::Raised);
    label_leslie_dispT2->setAlignment(Qt::AlignCenter);
    label_leslie_dispT2->setText(QString().setNum(VST_proc::leslie[channel_selected].frequency));

    LeslieFreqT = new QDialE(LeslieBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LeslieFreqT->setObjectName(QString::fromUtf8("LeslieFreqT"));
    LeslieFreqT->setGeometry(QRect(60+57, 41, 50, 71));
    //LeslieFreqT->setGeometry(QRect(174 + 60, 41, 50, 71));
    LeslieFreqT->setMinimum(20);
    LeslieFreqT->setMaximum(5000);
    LeslieFreqT->setValue(VST_proc::leslie[channel_selected].frequency);
    LeslieFreqT->setNotchTarget(400.0);
    LeslieFreqT->setNotchesVisible(true);
    LeslieFreqT->setToolTip("<html><head/><body><p>Leslie Treble Frequency</p>"
                            "<p>of the current channel</p></body></html>");

    connect(LeslieFreqT, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_leslie_dispT2->setText(QString().setNum(num));
        VST_proc::leslie[channel_selected].frequency = ((float) num);
    });

    label_leslie_deepT = new QLabel(LeslieBox);
    label_leslie_deepT->setObjectName(QString::fromUtf8("label_leslie_deepT"));
    label_leslie_deepT->setGeometry(QRect(174 + 12, 35, 31, 16));
    label_leslie_deepT->setAlignment(Qt::AlignCenter);
    label_leslie_deepT->setToolTip("<html><head/><body><p>Leslie Treble Deep Level</p>"
                                   "<p>of the current channel</p></body></html>");
    label_leslie_deepT->setText("Deep");

    label_leslie_freqT = new QLabel(LeslieBox);
    label_leslie_freqT->setObjectName(QString::fromUtf8("label_leslie_freqT"));

    label_leslie_freqT->setGeometry(QRect(69+57, 35, 31, 16));
    //label_leslie_freqT->setGeometry(QRect(174 + 69, 35, 31, 16));
    label_leslie_freqT->setAlignment(Qt::AlignCenter);
    label_leslie_freqT->setToolTip("<html><head/><body><p>Leslie Treble Frequency</p>"
                                   "<p>of the current channel</p></body></html>");
    label_leslie_freqT->setText("Freq");

    label_leslie_rotationT = new QLabel(LeslieBox);
    label_leslie_rotationT->setObjectName(QString::fromUtf8("label_leslie_rotationT"));
    label_leslie_rotationT->setGeometry(QRect(174 + 69, 35, 31, 16));
    //label_leslie_rotationT->setGeometry(QRect(174 + 69+57, 35, 31, 16));
    label_leslie_rotationT->setAlignment(Qt::AlignCenter);
    label_leslie_rotationT->setToolTip("<html><head/><body><p>Leslie Treble Rotation</p>"
                                       "<p>of the current channel</p></body></html>");
    label_leslie_rotationT->setText("Rotat");

    label_leslie_dispT = new QLabel(LeslieBox);
    label_leslie_dispT->setObjectName(QString::fromUtf8("label_leslie_dispT"));
    label_leslie_dispT->setGeometry(QRect(174 + 12, 100, 33, 21));
    label_leslie_dispT->setFrameShape(QFrame::Box);
    label_leslie_dispT->setFrameShadow(QFrame::Raised);
    label_leslie_dispT->setAlignment(Qt::AlignCenter);
    label_leslie_dispT->setText(QString().setNum(VST_proc::leslie[channel_selected].depthTreble * 2.0f, 'f', 2));

    LeslieDeepT = new QDialE(LeslieBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LeslieDeepT->setObjectName(QString::fromUtf8("LeslieDeepT"));
    LeslieDeepT->setGeometry(QRect(174 + 3, 41, 50, 71));
    LeslieDeepT->setMinimum(0);
    LeslieDeepT->setMaximum(100);
    LeslieDeepT->setValue(VST_proc::leslie[channel_selected].depthTreble * 200.0f);

    connect(LeslieDeepT, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_leslie_dispT->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));
        VST_proc::leslie[channel_selected].depthTreble = ((float) num) / 200.0f;
    });

    LeslieDeepT->setNotchTarget(10.0);
    LeslieDeepT->setNotchesVisible(true);
    LeslieDeepT->setToolTip("<html><head/><body><p>Leslie Treble Deep Level</p>"
                            "<p>of the current channel</p></body></html>");

// mover

    label_leslie_dispT3 = new QLabel(LeslieBox);
    label_leslie_dispT3->setObjectName(QString::fromUtf8("label_leslie_dispT3"));
    label_leslie_dispT3->setGeometry(QRect(174 + 69, 100, 33, 21));
    //label_leslie_dispT3->setGeometry(QRect(174 + 69+57, 100, 33, 21));
    label_leslie_dispT3->setFrameShape(QFrame::Box);
    label_leslie_dispT3->setFrameShadow(QFrame::Raised);
    label_leslie_dispT3->setAlignment(Qt::AlignCenter);
    label_leslie_dispT3->setText(QString().setNum((VST_proc::leslie[channel_selected].rotationSpeedTreble), 'f', 2));

    LeslieRotationT = new QDialE(LeslieBox, QDIALE_GRAY_NOTCH_WHITEDOT, 10);
    LeslieRotationT->setObjectName(QString::fromUtf8("LeslieRotationT"));
    LeslieRotationT->setGeometry(QRect(174 + 60, 41, 50, 71));
    //LeslieRotationT->setGeometry(QRect(174 + 60+57, 41, 50, 71));
    LeslieRotationT->setMinimum(1);
    LeslieRotationT->setMaximum(200);
    LeslieRotationT->setValue(VST_proc::leslie[channel_selected].rotationSpeedTreble * 10.0f);
    LeslieRotationT->setNotchTarget(14.0);
    LeslieRotationT->setNotchesVisible(true);
    LeslieRotationT->setToolTip("<html><head/><body><p>Leslie Treble Rotation</p>"
                                "<p>of the current channel</p></body></html>");

    connect(LeslieRotationT, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_leslie_dispT3->setText(QString().setNum(((float)(num)/10.0), 'f', 2));
        VST_proc::leslie[channel_selected].rotationSpeedTreble = ((float) num) / 10.0f;
    });

    label_leslie_Treble = new QLabel(LeslieBox);
    label_leslie_Treble->setObjectName(QString::fromUtf8("label_leslie_Treble"));
    label_leslie_Treble->setGeometry(QRect(174 + 70 - 57 + 24, 18, 31, 16));
    label_leslie_Treble->setAlignment(Qt::AlignCenter);
    label_leslie_Treble->setToolTip("<html><head/><body><p>Leslie Treble</p>"
                                  "<p>of the current channel</p></body></html>");
    label_leslie_Treble->setText("Treble");

    //&&

    LeslieButton = new QPushButton(LeslieBox);
    LeslieButton->setObjectName(QString::fromUtf8("LeslieButton"));
    LeslieButton->setGeometry(QRect(174 - 16 - 64/2, 18, 29, 16));
    LeslieButton->setCheckable(true);
    LeslieButton->setAutoDefault(false);
    LeslieButton->setDefault(true);
    LeslieButton->setChecked((VST_proc::leslieON[channel_selected]) ? true : false);
    LeslieButton->setToolTip("<html><head/><body><p>Leslie Amp On/Off</p>"
                              "<p>of the current channel</p></body></html>");
    LeslieButton->setText(QString());
    LeslieButton->setStyleSheet(QString::fromUtf8(
        "QPushButton{\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_off.png);\n"
        "background-position: center center;}\n"
        "QPushButton::checked {\n"
        "background-image: url(:/run_environment/graphics/fluid/icon_button_on.png);\n"
        "}"));

    connect(LeslieButton, QOverload<bool>::of(&QPushButton::clicked), [=](bool)
    {
        VST_proc::leslieON[channel_selected]^=true;
    });


    /////////////////&&&DE


    PresetBox = new QGroupBox(groupE);
    PresetBox->setObjectName(QString::fromUtf8("PresetBox"));
    lowbox+= 125 + expX + NEWDESP;
    PresetBox->setGeometry(QRect(lowbox, 4, 161+70, 128));
    PresetBox->setLayoutDirection(Qt::LeftToRight);
    PresetBox->setAlignment(Qt::AlignCenter);
    PresetBox->setFlat(false);
    PresetBox->setTitle("Presets");

    QFont font;
    font.setPointSize(16);
    spinPreset = new QSpinBox(PresetBox);
    spinPreset->setObjectName(QString::fromUtf8("spinPreset"));
    spinPreset->setToolTip("<html><head/><body><p>Current Preset for Load/Save</p></body></html>");
    spinPreset->setGeometry(QRect(10, 14, 45, 49));
    spinPreset->setFont(font);
    spinPreset->setAlignment(Qt::AlignCenter);
    spinPreset->setMaximum(15);
    spinPreset->setValue(preset_selected);
    spinPreset->setStyleSheet(QString::fromUtf8("color: black; background-color: white;"));

    PresetLoadButton = new QPushButton(PresetBox);
    PresetLoadButton->setObjectName(QString::fromUtf8("PresetLoadButton"));
    PresetLoadButton->setGeometry(QRect(10, 70, 63, 26));
    PresetLoadButton->setCheckable(false);
    PresetLoadButton->setAutoDefault(false);
    PresetLoadButton->setFlat(false);
    PresetLoadButton->setDefault(true);
    PresetLoadButton->setText("Load");
    PresetLoadButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetLoadButton->setToolTip("<html><head/><body><p>Load all presets</p>"
                                             "<p>from the indexed register</p></body></html>");

    PresetSaveButton = new QPushButton(PresetBox);
    PresetSaveButton->setObjectName(QString::fromUtf8("PresetSaveButton"));
    PresetSaveButton->setGeometry(QRect(10, 100, 63, 26));
    PresetSaveButton->setCheckable(false);
    PresetSaveButton->setAutoDefault(false);
    PresetSaveButton->setFlat(false);
    PresetSaveButton->setDefault(true);
    PresetSaveButton->setText("Save");
    PresetSaveButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetSaveButton->setToolTip("<html><head/><body><p>Save all presets</p>"
                                                 "<p>to the indexed register</p></body></html>");

    PresetResetButton = new QPushButton(PresetBox);
    PresetResetButton->setObjectName(QString::fromUtf8("PresetResetButton"));
    PresetResetButton->setGeometry(QRect(90, 70, 63, 26));
    PresetResetButton->setCheckable(false);
    PresetResetButton->setAutoDefault(false);
    PresetResetButton->setFlat(false);
    PresetResetButton->setDefault(true);
    PresetResetButton->setText("Reset");
    PresetResetButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetResetButton->setToolTip("<html><head/><body><p>Reset the presets</p>"
                                                     "<p>to the default values</p></body></html>");

    PresetLoadPButton = new QPushButton(PresetBox);
    PresetLoadPButton->setObjectName(QString::fromUtf8("PresetLoadPButton"));
    PresetLoadPButton->setGeometry(QRect(59, 14, 52, 51));
    PresetLoadPButton->setCheckable(false);
    PresetLoadPButton->setAutoDefault(false);
    PresetLoadPButton->setFlat(false);
    PresetLoadPButton->setDefault(true);
    PresetLoadPButton->setText("Get #0\nPresets\nin MIDI");
    PresetLoadPButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetLoadPButton->setToolTip("<html><head/><body><p>Load (if it exits)</p>"
                                         "<p>the systemEx event with all presets </p>"
                                         "<p>from the MIDI file position 0</p></body></html>");

    PresetStorePButton = new QPushButton(PresetBox);
    PresetStorePButton->setObjectName(QString::fromUtf8("PresetStorePButton"));
    PresetStorePButton->setGeometry(QRect(59+53, 14, 52, 51));
    PresetStorePButton->setCheckable(false);
    PresetStorePButton->setAutoDefault(false);
    PresetStorePButton->setFlat(false);
    PresetStorePButton->setDefault(true);
    PresetStorePButton->setText("Store All\nPresets\nin MIDI");
    PresetStorePButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetStorePButton->setToolTip("<html><head/><body><p>Store all presets in the MIDI File</p>"
                                     "<p>from the current position using </p>"
                                     "<p>a SystemEx event</p></body></html>");

    PresetDeletePButton = new QPushButton(PresetBox);
    PresetDeletePButton->setObjectName(QString::fromUtf8("PresetDeletePButton"));
    PresetDeletePButton->setGeometry(QRect(59+53*2, 70, 52, 51));
    PresetDeletePButton->setCheckable(false);
    PresetDeletePButton->setAutoDefault(false);
    PresetDeletePButton->setFlat(false);
    PresetDeletePButton->setDefault(true);
    PresetDeletePButton->setText("Delete\nPresets\nin MIDI");
    PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #40002F5F;\n"));
    PresetDeletePButton->setDisabled(true);
    PresetDeletePButton->setToolTip("<html><head/><body><p>Deletes the MIDI presets</p>"
                                      "<p>edited from the current position</p></body></html>");

    PresetStoreButton = new QPushButton(PresetBox);
    PresetStoreButton->setObjectName(QString::fromUtf8("PresetStoreButton"));
    PresetStoreButton->setGeometry(QRect(59+53*2, 14, 52, 51));
    PresetStoreButton->setCheckable(false);
    PresetStoreButton->setAutoDefault(false);
    PresetStoreButton->setFlat(false);
    PresetStoreButton->setDefault(true);
    PresetStoreButton->setText("Store Ch\nPreset\nin MIDI");
    PresetStoreButton->setStyleSheet(QString::fromUtf8("background-color: #80002F5F;\n"));
    PresetStoreButton->setToolTip("<html><head/><body><p>Store the preset of the current</p>"
                                    "<p>channel into the MIDI File</p>"
                                      "<p>from the current position</p></body></html>");

    for(n = 0; n < SYNTH_CHANS; n++) {

        connect(wicon[n], QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            VST_chan vst(main_widget, n, 1);
        });

        connect(wicon[n + SYNTH_CHANS], QOverload<bool>::of(&QPushButton::clicked), [=](bool)
        {
            VST_chan vst(main_widget, n + SYNTH_CHANS, 1);
        });

        connect(groupChan[n], QOverload<bool>::of(&QGroupBox::clicked), [=](bool checked)
        {
            fluid_output->setAudioMute(n, !checked);

        });

        connect(ChanVol[n], QOverload<int>::of(&QSliderE::valueChanged), [=](int num)
        {
            fluid_output->setSynthChanVolume(n, num*127/100);
        });

        connect(BalanceSlider[n], QOverload<int>::of(&QSliderE::valueChanged), [=](int num)
        {
            BalanceLabel[n]->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));
            fluid_output->setAudioBalance(n, num);
        });

        connect(chanGain[n], QOverload<int>::of(&QDialE::valueChanged), [=](int num)
        {
            chanGainLabel[n]->setText(QString().setNum(((float)(num / 10))/10.0, 'f', 2));
            fluid_output->setAudioGain(n, num);
            groupE->update();
        });

    }

    connect(MainVol, QOverload<int>::of(&QSliderE::valueChanged), [=](int num)
    {
        MainVolVal->setText(QString::number(((double) num)/100, 'f', 2));

        fluid_output->setSynthGain(num);
    });


    // effects
    QObject::connect(DistortionGain, SIGNAL(valueChanged(int)), label_distortion_disp, SLOT(setNum(int)));
    QObject::connect(LowCutFreq, SIGNAL(valueChanged(int)), label_low_disp2, SLOT(setNum(int)));
    connect(LowCutGain, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_low_disp->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    connect(LowCutRes, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_low_disp3->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    QObject::connect(HighCutFreq, SIGNAL(valueChanged(int)), label_high_disp2, SLOT(setNum(int)));

    connect(HighCutGain, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_high_disp->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    connect(HighCutRes, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_high_disp3->setText(QString().setNum(((float)(num / 10)/10.0), 'f', 2));

    });

    connect(TremoloLevel, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_trem_disp->setText(QString().setNum(((float)(num)/100.0), 'f', 2));
        fluid_output->level_WaveModulator[channel_selected] = ((float) num) / 100.0f;

    });

    connect(TremoloFreq, QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        label_trem_disp2->setText(QString().setNum(((float)(num)/100.0), 'f', 2));
        fluid_output->freq_WaveModulator[channel_selected] = ((float) num) / 100.0f;
    });


    block_scroll = false;

    connect(spinGroupIndex, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {

        channel_selected =  16 * num + (channel_selected & 15);

        spinChan->setValue(channel_selected);
    });

    connect(spinChan, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {
        int old = channel_selected;//
        channel_selected = num;

        num &= 15;
        int num2 = num + (channel_selected/16) * 16;

        if(!block_scroll) {
            scrollArea->horizontalScrollBar()->setValue((num2)*7);
            groupM->setGeometry(QRect((num2 < 16) ? 0: (- (num2/16*16))*60, 0, 3000, 600));
        }

        labelChan->setText(QString("Chan ")+QString().setNum(num2));

        groupChan[old]->setStyleSheet(QString::fromUtf8("QGroupBox {background-color: #80103040;}\n") + groupIndicator);
        channel_selected = num + (channel_selected/16) * 16;
        groupChan[channel_selected]->setStyleSheet(QString::fromUtf8("QGroupBox {background-color: #80303060;}\n") + groupIndicator);

        label_leslie_dispB->setText(QString().setNum(VST_proc::leslie[channel_selected].depthBass * 2.0f, 'f', 2));
        LeslieDeepB->setValue(VST_proc::leslie[channel_selected].depthBass * 200.0f);
        label_leslie_dispB3->setText(QString().setNum((VST_proc::leslie[channel_selected].rotationSpeedBass), 'f', 2));
        LeslieRotationB->setValue(VST_proc::leslie[channel_selected].rotationSpeedBass * 10.0f);
        label_leslie_dispT->setText(QString().setNum(VST_proc::leslie[channel_selected].depthTreble * 2.0f, 'f', 2));
        LeslieDeepT->setValue(VST_proc::leslie[channel_selected].depthTreble * 200.0f);
        label_leslie_dispT2->setText(QString().setNum(VST_proc::leslie[channel_selected].frequency));
        LeslieFreqT->setValue(VST_proc::leslie[channel_selected].frequency);
        label_leslie_dispT3->setText(QString().setNum((VST_proc::leslie[channel_selected].rotationSpeedTreble), 'f', 2));
        LeslieRotationT->setValue(VST_proc::leslie[channel_selected].rotationSpeedTreble * 10.0f);
        LeslieButton->setChecked((VST_proc::leslieON[channel_selected]) ? true : false);

        DistortionGain->setValue(
                    fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                    channel_selected, GET_FILTER_GAIN));

        LowCutGain->setValue(
                    fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                    channel_selected, GET_FILTER_GAIN));
        LowCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                           channel_selected, GET_FILTER_FREQ));
        LowCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                                                           channel_selected, GET_FILTER_RES));


        HighCutGain->setValue(
                    fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                    channel_selected, GET_FILTER_GAIN));
        HighCutFreq->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                     channel_selected, GET_FILTER_FREQ));
        HighCutRes->setValue(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                     channel_selected, GET_FILTER_RES));


        DistortionButton->setChecked(false);
        DistortionButton->setChecked(
                   (fluid_output->get_param_filter(PROC_FILTER_DISTORTION,
                    channel_selected, GET_FILTER_ON)!=0) ? true : false);


        LowCutButton->setChecked(false);
        LowCutButton->setChecked(
                   (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS,
                    channel_selected, GET_FILTER_ON)!=0) ? true : false);


        HighCutButton->setDefault(true);
        HighCutButton->setChecked(
                   (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS,
                    channel_selected, GET_FILTER_ON)!=0) ? true : false);

        TremoloLevel->setValue((int) (100.0f * fluid_output->level_WaveModulator[channel_selected]));
        TremoloFreq->setValue((int) (100.0f * fluid_output->freq_WaveModulator[channel_selected]));

        spinGroupIndex->setValue(channel_selected/16);
    });

    QObject::connect(DistortionButton, SIGNAL(clicked()), this, SLOT(distortion_clicked()));
    QObject::connect(DistortionGain, SIGNAL(valueChanged(int)), this, SLOT(distortion_gain(int)));

    QObject::connect(LowCutButton, SIGNAL(clicked()), this, SLOT(lowcut_clicked()));
    QObject::connect(LowCutGain, SIGNAL(valueChanged(int)), this, SLOT(lowcut_gain(int)));
    QObject::connect(LowCutFreq, SIGNAL(valueChanged(int)), this, SLOT(lowcut_freq(int)));
    QObject::connect(LowCutRes, SIGNAL(valueChanged(int)), this, SLOT(lowcut_res(int)));

    QObject::connect(HighCutButton, SIGNAL(clicked()), this, SLOT(highcut_clicked()));
    QObject::connect(HighCutGain, SIGNAL(valueChanged(int)), this, SLOT(highcut_gain(int)));
    QObject::connect(HighCutFreq, SIGNAL(valueChanged(int)), this, SLOT(highcut_freq(int)));
    QObject::connect(HighCutRes, SIGNAL(valueChanged(int)), this, SLOT(highcut_res(int)));

    QObject::connect(PresetSaveButton, SIGNAL(clicked()), this, SLOT(SaveRPressets()));
    QObject::connect(PresetLoadButton, SIGNAL(clicked()), this, SLOT(LoadRPressets()));
    QObject::connect(PresetDeletePButton, SIGNAL(clicked()), this, SLOT(DeletePressets()));
    QObject::connect(PresetStorePButton, SIGNAL(clicked()), this, SLOT(StorePressets()));
    QObject::connect(PresetLoadPButton, SIGNAL(clicked()), this, SLOT(LoadPressets()));
    QObject::connect(PresetStoreButton, SIGNAL(clicked()), this, SLOT(StoreSelectedPresset()));
    QObject::connect(PresetResetButton, SIGNAL(clicked()), this, SLOT(ResetPresset()));

    connect(spinPreset, QOverload<int>::of(&QSpinBox::valueChanged), [=](int num)
    {
        preset_selected = num;

    });

    // scroll
    connect(scrollArea->horizontalScrollBar(), QOverload<int>::of(&QDialE::valueChanged), [=](int num)
    {
        groupM->setGeometry(QRect(-num*58/16, 0, 3000, 600));
    });



}

void FluidDialog::mousePressEvent(QMouseEvent* /*event*/) {

    if(MainVolume == NULL || disable_mainmenu) return;

    for(int n= 0; n < SYNTH_CHANS; n++) {
        if(groupChan[n]->hasFocus()) {
            groupChan[channel_selected]->setStyleSheet(QString::fromUtf8("QGroupBox {background-color: #80103040;}\n") + groupIndicator);
            channel_selected = n;
            groupChan[n]->setStyleSheet(QString::fromUtf8("QGroupBox {background-color: #80303060;}\n") + groupIndicator);

            block_scroll = true;
            spinChan->setValue(n);
            spinGroupIndex->setValue(n/16);
            block_scroll = false;
            break;

        }
    }

}

void FluidDialog::distortion_clicked()
{

    int gain = fluid_output->get_param_filter(PROC_FILTER_DISTORTION, channel_selected, GET_FILTER_GAIN);

    if(fluid_output->get_param_filter(PROC_FILTER_DISTORTION, channel_selected, GET_FILTER_ON)==0)
        fluid_output->setAudioDistortionFilter(1, channel_selected, gain);
    else fluid_output->setAudioDistortionFilter(0, channel_selected, gain);
}

void FluidDialog::distortion_gain(int gain)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_DISTORTION, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    fluid_output->setAudioDistortionFilter((check) ? 1: 0, channel_selected, gain);
}

void FluidDialog::lowcut_clicked()
{

    int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_RES);

    if(fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)==0)
        fluid_output->setAudioLowPassFilter(1, channel_selected, freq, gain, res);
    else fluid_output->setAudioLowPassFilter(0, channel_selected, freq, gain, res);
}

void FluidDialog::lowcut_gain(int gain)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioLowPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::lowcut_freq(int freq)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_GAIN);
    int res = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioLowPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::lowcut_res(int res)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_LOW_PASS, channel_selected, GET_FILTER_FREQ);

    fluid_output->setAudioLowPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_clicked()
{

    int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_RES);

    if(fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)==0)
        fluid_output->setAudioHighPassFilter(1, channel_selected, freq, gain, res);
    else fluid_output->setAudioHighPassFilter(0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_gain(int gain)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_FREQ);
    int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioHighPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_freq(int freq)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_GAIN);
    int res = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_RES);

    fluid_output->setAudioHighPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::highcut_res(int res)
{
    bool check = (fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_ON)!=0) ? true : false;

    int gain = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_GAIN);
    int freq = fluid_output->get_param_filter(PROC_FILTER_HIGH_PASS, channel_selected, GET_FILTER_FREQ);

    fluid_output->setAudioHighPassFilter((check) ? 1: 0, channel_selected, freq, gain, res);
}

void FluidDialog::timer_update(){

    if(fluid_output== NULL || MainVolume == NULL || disable_mainmenu) return;

    for(int n = 0; n < 16; n++) {
        if(groupChan[n]) {
            if(groupChan[n]->isChecked() && fluid_output->getAudioMute(n))
                groupChan[n]->setChecked(false);
            if(!groupChan[n]->isChecked() && !fluid_output->getAudioMute(n))
                groupChan[n]->setChecked(true);
        }
    }

    if(_cur_edit == -666 && _edit_mode) {
        _edit_mode = 0;
        PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #40002F5F;\n"));
        PresetDeletePButton->setDisabled(true);
    } else if(_cur_edit < 0 && _edit_mode) {
        _cur_edit = _current_tick;
        MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
        MWin->update();
    }

    // VU meter
    int vl = fluid_output->cleft;
    int vr = fluid_output->cright;

    for(int n = 0; n < 25;n++) {
        int flag= 25 - vl / 8;
        if(n>=flag) {
            if(n < 3) line_l[n]->setStyleSheet(QString::fromUtf8("color: #FF6000;"));
            else line_l[n]->setStyleSheet(QString::fromUtf8("color: #60FF00;"));
        } else
            line_l[n]->setStyleSheet(QString::fromUtf8("color: #408000;"));
        flag= 25 - vr / 8;
        if(n >= flag) {
            if(n < 3) line_r[n]->setStyleSheet(QString::fromUtf8("color: #FF8000;"));
            else line_r[n]->setStyleSheet(QString::fromUtf8("color: #80FF00;"));
        } else
            line_r[n]->setStyleSheet(QString::fromUtf8("color: #608000;"));
    }

    // get first track index for Fluidsynth channels
    int track_index[3] = {-1, -1, -1};

    for(int n = 0; n < MAX_OUTPUT_DEVICES; n++) {
        int r = MidiOutput::AllTracksToOne ? MidiOutput::_midiOutFluidMAP[0] : MidiOutput::_midiOutFluidMAP[n];
        if(r >= 0 && track_index[r % 3] < 0) track_index[r % 3] = n;
    }

    // channel animation
    for(int n = 0; n < SYNTH_CHANS; n++) {
        if(fluid_output->isNoteOn[n]) {
            if(!fluid_output->getAudioMute(n))
                line[n]->setStyleSheet(QString::fromUtf8("color: green;"));
            else
                line[n]->setStyleSheet(QString::fromUtf8("color: red;"));
        } else
            line[n]->setStyleSheet(QString::fromUtf8("color: #00000000;"));

        if(fluid_output->isNoteOn[n]) fluid_output->isNoteOn[n]--;
    }

    for(int n = 0; n < SYNTH_CHANS; n++) {
        int indx = track_index[n/16] >= 0 ? ((n & 15) + 4 * (track_index[n/16] > 0)
                + 16 * track_index[n/16]) : (n & 15);

        int _bank = MidiOutput::file->Bank_MIDI[indx];
        int _instrument = MidiOutput::file->Prog_MIDI[indx];

        QString s;

        if(track_index[n/16] >= 0) {

            bool is_instrument = false;

            if((n  & 15) != 9 || ((n & 15) == 9 && (n > 9) && track_index[n/16] > 0
                          && !MidiOutput::file->DrumUseCh9))
                is_instrument = true;

            if(is_instrument)
                s = MidiFile::instrumentName( _bank, _instrument);
            else
                s = MidiFile::drumName(_instrument);
        }


        qv[n]->setText(s);
    }
    update();
}
void FluidDialog::UpdateCursor() {

    if(_edit_mode == 0) {
        MainWindow *MWin = ((MainWindow *) _parent);
        _current_tick = (MWin->getFile()) ? MWin->getFile()->cursorTick() : 0;
    }

}
void FluidDialog::DeletePressets() // delete
{
    QByteArray b;

    char id[4]=
    {0x0, 0x66, 0x66, 'R'};

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    if(_edit_mode) {// delete preset

        MWin->getFile()->protocol()->startNewAction("SYSex Presets Deleted");
        int dtick= MWin->getFile()->tick(150);
        int current_tick = _current_tick;
        foreach (MidiEvent* event,
                 *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            if(sys) {
                b = sys->data();
                if((((b[0] == (char) 0 || b[0] == (char) 1 || b[0] == (char) 2) && b[3] == (char) (0x70+channel_selected))
                  || b[3] == 'R') && b[1] == id[1] && b[2] == id[2] &&
                  (b[3] == 'R' || b[3] == (char) (0x70+channel_selected))){
                    MWin->getFile()->channel(16)->removeEvent(sys);
                }
            }

        }

        MWin->getFile()->protocol()->endAction();

        return;
    }
}
void FluidDialog::StorePressets() // store or delete
{

    QByteArray b, c;

    char id[4]=
    {0x10, 0x66, 0x66, 'R'};
    int entries = 21 * SYNTH_CHANS + 1;
    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    QDataStream qd(&b,
    QIODevice::WriteOnly); // save header

    // write the header
    if(qd.writeRawData((const char *) id, 4)<0) return;

    encode_sys_format(qd, (void *) &entries);

    for(int n = 0; n < SYNTH_CHANS; n++) {

        if(n == 0)
            encode_sys_format(qd, (void *) &fluid_output->synth_gain);

        encode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

        if(fluid_output->filter_dist_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

        if(fluid_output->filter_locut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

        if(fluid_output->filter_hicut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

        // new WaveModulator
        encode_sys_format(qd, (void *) &fluid_output->level_WaveModulator[n]);
        encode_sys_format(qd, (void *) &fluid_output->freq_WaveModulator[n]);

        // Leslie effect
        encode_sys_format(qd, (void *) &VST_proc::leslieON[n]);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthBass);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthTreble);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].frequency);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedBass);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedTreble);
    }

    if(_save_mode) {
        QString entry=QString("presets_audio_index")+QString().setNum(preset_selected)+QString("")+QString().setNum(16);
        fluid_output->fluid_settings->setValue(entry, b);
        return;
    }


    MWin->getFile()->protocol()->startNewAction("SYSex Presets stored");
    int dtick= MWin->getFile()->tick(150);

    int current_tick = _current_tick;

    foreach (MidiEvent* event,
             *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            c = sys->data();
            // delete for individual chans
            if((c[0] == (char) 0 || c[0] == (char) 1 || c[0] == (char) 2) && c[1] == id[1] && c[2] == id[2] && (c[3] & 0xF0) == (char) 0x70){
                MWin->getFile()->channel(event->channel())->removeEvent(sys);
            }

            if(c[1] == id[1] && c[2] == id[2] && c[3] == 'R'){
                MWin->getFile()->channel(16)->removeEvent(sys);
            }

        }

    }


    SysExEvent *sys_event = new SysExEvent(16, b, MWin->getFile()->track(0));
    MWin->getFile()->channel(16)->insertEvent(sys_event, current_tick);


    MWin->getFile()->protocol()->endAction();

}

void FluidDialog::LoadPressets()
{
    QByteArray b, d;

    char id2[4]= {0x10, 0x66, 0x66, 'R'};
    int found= 0;
    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    if(_save_mode) {

        QString entry=QString("presets_audio_index")+QString().setNum(preset_selected)+QString("")+QString().setNum(16);
        b= fluid_output->fluid_settings->value(entry).toByteArray();

        found = 2;
    } else {
        foreach (MidiEvent* event,
                 *(MWin->getFile()->eventsBetween(0, 50))) {

            SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

            if(sys) {
                d = sys->data();
                if(d[1]!=id2[1] || d[2]!=id2[2] ) continue;

                if(d[3]=='R' && d[0]==id2[0] ) {b = d; found=2;}

            }
        }

    }

    if(!found) return;

    int entries = 13 * 16 + 1;
    char id[4];

    if(b.size() < 4) return;

    QDataStream qd(&b,
                   QIODevice::ReadOnly);
    qd.startTransaction();

    qd.readRawData((char *) id, 4);

    if(id[1]==id2[1] && id[2]==id2[2] && id[0] == id2[0] && id[3] == 'R') {

        if(id[3] == 'R') found = 2;
        else found =1;

        if(decode_sys_format(qd, (void *) &entries)<0) {

            return;
        }

        if(found == 2 && (entries == (21 * SYNTH_CHANS + 1)))
            found = 3;

        if(found !=3 && (found == 1 && entries != 13 * 16 + 1) &&
                (found == 2 && entries != (15 * 16 + 1) && entries != (15 * SYNTH_CHANS + 1))) {

            return;
        }

        int nchan = 16;

        if(entries == (15 * SYNTH_CHANS + 1) || entries == (21 * SYNTH_CHANS + 1))
            nchan = SYNTH_CHANS;

        for(int n = 0; n < nchan; n++) {

            if(n == 0)
                decode_sys_format(qd, (void *) &fluid_output->synth_gain);

            fluid_output->audio_chanmute[n]= false;
            groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
            decode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
            decode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
            decode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

            decode_sys_format(qd, (void *) &BOOL);
            fluid_output->filter_dist_on[n]= (BOOL) ? true : false;
            decode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

            decode_sys_format(qd, (void *) &BOOL);
            fluid_output->filter_locut_on[n]= (BOOL) ? true : false;
            decode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
            decode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
            decode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

            decode_sys_format(qd, (void *) &BOOL);
            fluid_output->filter_hicut_on[n]= (BOOL) ? true : false;
            decode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
            decode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
            decode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

            if(found == 3) {

                decode_sys_format(qd, (void *) &fluid_output->level_WaveModulator[n]);
                decode_sys_format(qd, (void *) &fluid_output->freq_WaveModulator[n]);

                decode_sys_format(qd, (void *) &VST_proc::leslieON[n]);
                decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthBass);
                decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthTreble);
                decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].frequency);
                decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedBass);
                decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedTreble);

            } else if(found == 2) {

                decode_sys_format(qd, (void *) &fluid_output->level_WaveModulator[n]);
                decode_sys_format(qd, (void *) &fluid_output->freq_WaveModulator[n]);

            } else {

                fluid_output->level_WaveModulator[n] = 0.0f;
                fluid_output->freq_WaveModulator[n] = 0.0f;

            }

            ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
            BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
            chanGain[n]->setValue(fluid_output->getAudioGain(n));

            //Leslie effect
            LeslieButton->setChecked((VST_proc::leslieON[n]) ? true : false);
            LeslieDeepB->setValue(VST_proc::leslie[n].depthBass * 200.0f);
            LeslieDeepT->setValue(VST_proc::leslie[n].depthTreble * 200.0f);
            LeslieFreqT->setValue(VST_proc::leslie[n].frequency);
            LeslieRotationB->setValue(VST_proc::leslie[n].rotationSpeedBass * 10.0f);
            LeslieRotationT->setValue(VST_proc::leslie[n].rotationSpeedTreble * 10.0f);
            emit changeVolBalanceGain(n, fluid_output->getSynthChanVolume(n)* 100 / 127,
                                      fluid_output->getAudioBalance(n), fluid_output->getAudioGain(n));
        }
    }

    MainVol->setValue(fluid_output->getSynthGain());
    channel_selected=0;
    spinChan->valueChanged(channel_selected);
    update();

    emit changeMainVolume(fluid_output->getSynthGain());
}

void FluidDialog::StoreSelectedPresset()
{
    QByteArray b;

    char id[4]=
    {0x0, 0x66, 0x66, 0x70};
    int entries = 21 * 1;

    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D

    int track_index = MWin->getFile()->track(NewNoteTool::editTrack())->fluid_index();

    id[0] = track_index % 3;
    id[3]|= (channel_selected & 15);

    QDataStream qd(&b,
                   QIODevice::WriteOnly); // save header

    // write the header
    if(qd.writeRawData((const char *) id, 4)<0) return;

    encode_sys_format(qd, (void *) &entries);

    encode_sys_format(qd, (void *) &fluid_output->synth_gain);

    int n = channel_selected;

        encode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
        encode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

        if(fluid_output->filter_dist_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

        if(fluid_output->filter_locut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

        if(fluid_output->filter_hicut_on[n]) BOOL=1; else BOOL=0;
        encode_sys_format(qd, (void *) &BOOL);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
        encode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

        // new WaveModulator
        encode_sys_format(qd, (void *) &fluid_output->level_WaveModulator[n]);
        encode_sys_format(qd, (void *) &fluid_output->freq_WaveModulator[n]);

        //Leslie effect
        encode_sys_format(qd, (void *) &VST_proc::leslieON[n]);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthBass);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthTreble);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].frequency);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedBass);
        encode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedTreble);

    MWin->getFile()->track(0);
    SysExEvent *sys_event = new SysExEvent(16, b, MWin->getFile()->track(0));

    MWin->getFile()->protocol()->startNewAction("SYSex Preset stored");
    int dtick= MWin->getFile()->tick(150);

    int current_tick =_current_tick;
    foreach (MidiEvent* event,
             *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            b = sys->data();
            if(b[0]== (char) 0x10 && b[1]==id[1] && b[2]==id[2] && b[3]=='R'){

                StorePressets(); // use colective command
                return;
            }
            if(b[0]==id[0] && b[1]==id[1] && b[2]==id[2] && b[3]==id[3]){
                MWin->getFile()->channel(16)->removeEvent(sys);
            }
        }

    }
    MWin->getFile()->channel(16)->insertEvent(sys_event, current_tick);
    MWin->getFile()->protocol()->endAction();
}

int FluidDialog::EditMode() {
    return _edit_mode;
}

void FluidDialog::LoadSelectedPresset(int current_tick)
{
    QByteArray b;

    char id2[4]= {0x0, 0x66, 0x66, 0x70};

    int BOOL;

    MainWindow *MWin = ((MainWindow *) _parent); // get MainWindow :D
    int dtick= MWin->getFile()->tick(150);

    //int current_tick = MWin->getFile()->cursorTick();
    foreach (MidiEvent* event,
             *(MWin->getFile()->eventsBetween(current_tick-dtick, current_tick+dtick))) {

        SysExEvent* sys = dynamic_cast<SysExEvent*>(event);

        if(sys) {
            b = sys->data();

            if(b[1] == id2[1] && b[2] == id2[2] &&
                    (((b[0] == (char) 0 || b[0] == (char) 1 || b[0] == (char) 2) && (b[3] & 0xf0) == 0x70)
                     || b[3] == 'R')){
                int entries = 13 * 1;
                char id[4];

                int flag = 1;

                int n = b[3] & 0xf;
                int bucle = 1;

                if(b[3] == 'R') {
                    flag = 2;
                    if(b[0] >= (char) 16) {flag = 3; bucle = 16;}
                    else n = b[0] & 0xf;
                }

                if(flag == 1) {
                    n+= (b[0] % 3) * 16;
                }

                QDataStream qd(&b,
                               QIODevice::ReadOnly);
                qd.startTransaction();

                qd.readRawData((char *) id, 4);

                if(flag == 1 && ((id[0] != (char) 0 && id[0] != (char) 1 && id[0] != (char) 2)
                    || id[1]!=id2[1] || id[2]!=id2[2] || (id[3] & 0xF0) != 0x70)) {
                    continue;
                }

                if(flag == 2 && (id[1]!=id2[1] || id[2]!=id2[2] || id[3] != 'R')) {
                    continue;
                }

                if(flag == 3 && (id[0]!= 0x10 || id[1]!=id2[1] || id[2]!=id2[2] || id[3] != 'R')) {
                    continue;
                }

                if(decode_sys_format(qd, (void *) &entries)<0) {

                    continue;
                }

                if(flag == 3 && (entries != (15 * 16 + 1) && entries != (15 * SYNTH_CHANS + 1)
                    && entries != (21 * SYNTH_CHANS + 1))) {
                    QMessageBox::information(this, "Ehhh!", "entries differents");
                    continue;
                }

                if(flag == 2 && n == 0 && entries != (13 + 1)) {
                    QMessageBox::information(this, "Ehhh!", "entries differents");
                    continue;
                }

                if(flag != 3 && (n != 0 || flag == 1) && (entries != 13 && entries != 15 && entries != 21)) {
                    QMessageBox::information(this, "Ehhh!", "entries differents");
                    continue;
                }

                bool extend = false;

                if(flag == 3 && (entries == (21 * SYNTH_CHANS + 1)))
                    extend = true;
                if(flag == 1 && entries == 21)
                    extend = true;

                for(int m = 0; m < bucle; m++) {

                    if(flag == 3) n = m;

                    if(((flag == 2 || flag == 3 ) && n == 0) || flag == 1)
                        decode_sys_format(qd, (void *) &fluid_output->synth_gain);

                    fluid_output->audio_chanmute[n]= false;
                    groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
                    decode_sys_format(qd, (void *) &fluid_output->synth_chanvolume[n]);
                    decode_sys_format(qd, (void *) &fluid_output->audio_changain[n]);
                    decode_sys_format(qd, (void *) &fluid_output->audio_chanbalance[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    fluid_output->filter_dist_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &fluid_output->filter_dist_gain[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    fluid_output->filter_locut_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &fluid_output->filter_locut_freq[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_locut_gain[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_locut_res[n]);

                    decode_sys_format(qd, (void *) &BOOL);
                    fluid_output->filter_hicut_on[n]= (BOOL) ? true : false;
                    decode_sys_format(qd, (void *) &fluid_output->filter_hicut_freq[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_hicut_gain[n]);
                    decode_sys_format(qd, (void *) &fluid_output->filter_hicut_res[n]);

                    if(flag == 3 || flag == 4 || (flag == 1 && (entries == 15 || entries == 21))) {
                        decode_sys_format(qd, (void *) &fluid_output->level_WaveModulator[n]);
                        decode_sys_format(qd, (void *) &fluid_output->freq_WaveModulator[n]);

                        if(extend) {

                            decode_sys_format(qd, (void *) &VST_proc::leslieON[n]);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthBass);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].depthTreble);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].frequency);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedBass);
                            decode_sys_format(qd, (void *) &VST_proc::VST_proc::leslie[n].rotationSpeedTreble);

                        }

                    } else {

                        fluid_output->level_WaveModulator[n] = 0.0f;
                        fluid_output->freq_WaveModulator[n] = 0.0f;

                    }

                    ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
                    BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
                    chanGain[n]->setValue(fluid_output->getAudioGain(n));

                    //Leslie effect
                    LeslieButton->setChecked((VST_proc::leslieON[n]) ? true : false);
                    LeslieDeepB->setValue(VST_proc::leslie[n].depthBass * 200.0f);
                    LeslieDeepT->setValue(VST_proc::leslie[n].depthTreble * 200.0f);
                    LeslieFreqT->setValue(VST_proc::leslie[n].frequency);
                    LeslieRotationB->setValue(VST_proc::leslie[n].rotationSpeedBass * 10.0f);
                    LeslieRotationT->setValue(VST_proc::leslie[n].rotationSpeedTreble * 10.0f);

                }

                _edit_mode = 1;
                _current_tick = current_tick;
                this->setWindowTitle("Fluid Synth Control - Edit");

                PresetDeletePButton->setStyleSheet(QString::fromUtf8("background-color: #805f2F00;\n"));
                PresetDeletePButton->setDisabled(false);

                if(flag == 1) {
                    channel_selected = n;
                    spinChan->setValue(n);
                    spinChan->valueChanged(n);
                    channel_selected = n;
                } else {

                    MainVol->setValue(fluid_output->getSynthGain());
                    channel_selected=0;
                    spinChan->valueChanged(channel_selected);
                }

            }

        } // sys
    }

}

void FluidDialog::ResetPresset() {

    fluid_output->synth_gain= 1.0;

    for(int n = 0; n < SYNTH_CHANS; n++) {
        fluid_output->audio_chanmute[n]= false;
        groupChan[n]->setChecked(!fluid_output->getAudioMute(n));
        fluid_output->synth_chanvolume[n]= 127.0;
        fluid_output->audio_changain[n]= 0.0;
        fluid_output->audio_chanbalance[n]= 0.0;

        fluid_output->filter_dist_on[n]= false;
        fluid_output->filter_dist_gain[n]= 0.0;

        fluid_output->filter_locut_on[n]= false;
        fluid_output->filter_locut_freq[n]= 500.0;
        fluid_output->filter_locut_gain[n]= 0.0;
        fluid_output->filter_locut_res[n]= 0.0;

        fluid_output->level_WaveModulator[n] = 0.0;
        fluid_output->freq_WaveModulator[n] = 0.0;


        fluid_output->filter_hicut_on[n]= false;
        fluid_output->filter_hicut_freq[n]= 2500.0;
        fluid_output->filter_hicut_gain[n]= 0.0;
        fluid_output->filter_hicut_res[n]= 0.0;

        ChanVol[n]->setValue(fluid_output->getSynthChanVolume(n)*100/127);
        BalanceSlider[n]->setValue(fluid_output->getAudioBalance(n));
        chanGain[n]->setValue(fluid_output->getAudioGain(n));

    }

    VST_proc::VST_LeslieReset();

    MainVol->setValue(fluid_output->getSynthGain());
    channel_selected=0;
    spinChan->valueChanged(channel_selected);
    update();
}

void FluidDialog::SaveRPressets() {
    _save_mode = 1;
    StorePressets();
    _save_mode = 0;
}
void FluidDialog::LoadRPressets() {
    _save_mode = 1;
    LoadPressets();
    _save_mode = 0;
}

#endif
