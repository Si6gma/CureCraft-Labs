#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onTick();

    void on_btnToggleECG_toggled(bool checked);
    void on_btnToggleSpO2_toggled(bool checked);
    void on_btnToggleResp_toggled(bool checked);

private:
    void setupPlots();
    void pushSample(QVector<double>& x, QVector<double>& y, double t, double v);

    Ui::MainWindow *ui;

    QTimer timer_;

    // Data buffers
    QVector<double> xECG_, yECG_;
    QVector<double> xSpO2_, ySpO2_;
    QVector<double> xResp_, yResp_;

    double t_ = 0.0;

    // Plot settings
    const int maxPoints_ = 1500;
    const double windowSeconds_ = 6.0;
};

#endif // MAINWINDOW_H
