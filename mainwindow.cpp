#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <QToolTip>

#include "stdio.h"
#include "string.h"
#include "math.h"
#include "stdlib.h"
#include "stationmapper.h"


#if defined Q_OS_WIN && _MSC_VER
#include <dwmapi.h>
#pragma comment (lib,"Dwmapi.lib") // fixes error LNK2019: unresolved external symbol __imp__DwmExtendFrameIntoClientArea

enum : WORD {
    DwmwaUseImmersiveDarkMode = 20,
    DwmwaUseImmersiveDarkModeBefore20h1 = 19
};

bool setDarkBorderToWindow(HWND hwnd, bool dark)
{
    const BOOL darkBorder = dark ? TRUE : FALSE;
    const bool ok =
        SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkMode, &darkBorder, sizeof(darkBorder)))
        || SUCCEEDED(DwmSetWindowAttribute(hwnd, DwmwaUseImmersiveDarkModeBefore20h1, &darkBorder, sizeof(darkBorder)));
    if (!ok) {
        qDebug()<<QString("%1: Unable to set %2 window border.").arg(__FUNCTION__, dark ? "dark" : "light");
    }
    return ok;
}
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowTitle("Station Map");


    // dark style
    QPalette pal;
    pal.setColor(QPalette::Window, QColor(52,54,58));//QColor(54,57,63));
    pal.setColor(QPalette::Button, QColor(52,54,58));
    pal.setColor(QPalette::Disabled, QPalette::Button, QColor(40,41,45));
    pal.setColor(QPalette::Base, QColor(47,48,52));//47,48,52//72,74,78
    pal.setColor(QPalette::Disabled, QPalette::Base, QColor(35,36,40));
    pal.setColor(QPalette::AlternateBase, QColor(66,66,66));
    pal.setColor(QPalette::ToolTipBase, QColor(75,76,77));

    pal.setColor(QPalette::Dark, QColor(66,66,66));     // qframe
    pal.setColor(QPalette::Light, QColor(66,66,66));    // qframe
    pal.setColor(QPalette::Shadow, QColor(20,20,20));

    pal.setColor(QPalette::Text, QColor(230,231,232));
    pal.setColor(QPalette::Disabled, QPalette::Text, QColor(127,127,127));
    pal.setColor(QPalette::WindowText, QColor(230,231,232));
    pal.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127,127,127));
    pal.setColor(QPalette::ToolTipText, QColor(230,231,232));
    pal.setColor(QPalette::ButtonText, QColor(230,231,232));
    pal.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127,127,127));
    pal.setColor(QPalette::BrightText, Qt::red);
    pal.setColor(QPalette::Link, QColor(20,100,170));
    pal.setColor(QPalette::Highlight, QColor(20,100,170));// QColor(20,150,250));
    pal.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80,80,80));
    pal.setColor(QPalette::HighlightedText, QColor(230,231,232));
    pal.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127,127,127));
    QToolTip::setPalette(pal);
    qApp->setPalette(pal);

#if defined Q_OS_WIN && _MSC_VER
    setDarkBorderToWindow((HWND)window()->winId(), true);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_mapImg_clicked()
{
    m_mapImg.setFileName(QFileDialog::getOpenFileName(this, tr("Открыть изображение"), QDir::currentPath() + "/", tr("Изображение (.bmp) (*.bmp)")));

    if (m_mapImg.open(QIODevice::ReadWrite)) {
        ui->lineEdit_mapImg->setText(m_mapImg.fileName());
        QPixmap pix(m_mapImg.fileName());
        ui->label_mapImage->setPixmap(pix);
        fileSelected();
    } else {
        qDebug() << "cant open map img file";
    }
}


void MainWindow::on_pushButton_mapCsv_clicked()
{
    m_mapCsv.setFileName(QFileDialog::getOpenFileName(this, tr("Открыть файл координат карты"), QDir::currentPath() + "/", tr("Координаты карты (.csv) (*.csv)")));

    if (m_mapCsv.open(QIODevice::ReadWrite)) {
        ui->lineEdit_mapCsv->setText(m_mapCsv.fileName());
        fileSelected();
    } else {
        qDebug() << "cant open map csv file";
    }
}


void MainWindow::on_pushButton_stations_clicked()
{
    m_stations.setFileName(QFileDialog::getOpenFileName(this, tr("Открыть файл остановок"), QDir::currentPath() + "/", tr("Остановки (.csv) (*.csv)")));

    if (m_stations.open(QIODevice::ReadWrite)) {
        ui->lineEdit_stations->setText(m_stations.fileName());
        fileSelected();
    } else {
        qDebug() << "cant open stations file";
    }
}


void MainWindow::on_pushButton_calculate_clicked()
{
    // Get library version;
    version_t version = get_library_version();
    qDebug().nospace() << "Using stationmapper " << version.major << "." << version.minor << "." << version.patch;

    // Load map and stations list
    peace_of_map_t map = load_map(m_mapImg.fileName().toUtf8().constData(), m_mapCsv.fileName().toUtf8().constData());
    stations_list_t stations = load_stations(m_stations.fileName().toUtf8().constData());

    // Get user's location
    float user_lat = ui->doubleSpinBox_lat->value();
    float user_lon = ui->doubleSpinBox_lon->value();

    // Draw user's location
    draw_point_by_lat_lon(&map, user_lat, user_lon, 0, 0, 255, 155);

    // Draw all stations
    for (int i = 0; i < stations.num_stations; i++) {
        draw_point_by_lat_lon(&map, stations.stations[i].lat, stations.stations[i].lon, 255, 0, 0, 155);
        qDebug()<<stations.stations[i].lat << stations.stations[i].lon;
    }

    // Nearest station search
    station_t nearest_station = get_nearest_station(&stations, user_lat, user_lon);
    draw_point_by_lat_lon(&map, nearest_station.lat, nearest_station.lon, 255, 200, 0, 255);

    ui->label_nearStation->setText(QString::fromUtf8(reinterpret_cast<const char*>(nearest_station.name)));
    ui->label_nearLat->setNum(nearest_station.lat);
    ui->label_nearLon->setNum(nearest_station.lon);
    ui->label_nearDist->setText(QString::number(get_distance_in_km(nearest_station.lat, nearest_station.lon, user_lat, user_lon), 0, 2) + " км");

    qDebug() << "The nearest station:" << QString::fromUtf8(reinterpret_cast<const char*>(nearest_station.name))
             << "(lat:" << nearest_station.lat << "lon:" << nearest_station.lon << ")";

    qDebug() << "Distance to the station:" << get_distance_in_km(nearest_station.lat, nearest_station.lon, user_lat, user_lon) << "km";

    // Generate output map image
    const char output_map_name[] = "map_out.bmp";
    int err = save_map(&map, output_map_name);
    if (err) {
        qDebug() << "Failed to save" << output_map_name << "error code:" << err;
    } else {
        qDebug() << "Output map image generated" << output_map_name;
        QPixmap pix(QString(QDir::currentPath() + "/" + output_map_name));
        ui->label_mapImage->setPixmap(pix);
    }

    // Cleanup
    free(map.image);
    free(stations.stations);
}


void MainWindow::fileSelected()
{
    if (m_mapCsv.exists()) {
        float l_lat, l_lon, r_lat, r_lon;

        QFile file(m_mapCsv.fileName());
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return;
        }

        QTextStream in(&file);
        in.readLine();  // Skip header

        if (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split(',', Qt::SkipEmptyParts);

            if (parts.size() >= 4) {
                l_lat = parts[0].trimmed().toFloat();
                l_lon = parts[1].trimmed().toFloat();
                r_lat = parts[2].trimmed().toFloat();
                r_lon = parts[3].trimmed().toFloat();
            }
        }

        file.close();


        ui->doubleSpinBox_lat->setMinimum(qMin(l_lat, r_lat));
        ui->doubleSpinBox_lat->setMaximum(qMax(l_lat, r_lat));
        ui->doubleSpinBox_lat->setValue((ui->doubleSpinBox_lat->minimum() + ui->doubleSpinBox_lat->maximum()) / 2);
        ui->doubleSpinBox_lat->setEnabled(true);

        ui->doubleSpinBox_lon->setMinimum(qMin(l_lon, r_lon));
        ui->doubleSpinBox_lon->setMaximum(qMax(l_lon, r_lon));
        ui->doubleSpinBox_lon->setValue((ui->doubleSpinBox_lon->minimum() + ui->doubleSpinBox_lon->maximum()) / 2);
        ui->doubleSpinBox_lon->setEnabled(true);

        if (m_stations.exists() && m_mapImg.exists()) {
            ui->pushButton_calculate->setEnabled(true);
        }
    }
}

