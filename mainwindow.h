#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

// signals:
//     void fileSelected();

private slots:
    void on_pushButton_mapImg_clicked();
    void on_pushButton_mapCsv_clicked();
    void on_pushButton_stations_clicked();

    void on_pushButton_calculate_clicked();

    void fileSelected();

private:
    Ui::MainWindow *ui;

    QFile m_mapImg;
    QFile m_mapCsv;
    QFile m_stations;
};
#endif // MAINWINDOW_H
