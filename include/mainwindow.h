#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QVector>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
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
    void pushSample(QVector<double> &x, QVector<double> &y, double t, double v);

    // Precomputed waveform parameters for faster signal generation
    struct WaveformParams
    {
        double ecgFreq = 1.0;
        double ecgAmplitude = 0.08;
        double ecgSpikeAmplitude = 1.25;
        double spO2Freq = 1.2;
        double spO2Base = 0.55;
        double spO2Amplitude = 0.4;
        double respFreq = 0.3;
        double respAmplitude = 0.6;
    } waveParams_;

    Ui::MainWindow *ui;
    QTimer timer_;

    // Time-series data using QVector (native to QCustomPlot - avoids conversions!)
    QVector<double> xECG_, yECG_;
    QVector<double> xSpO2_, ySpO2_;
    QVector<double> xResp_, yResp_;

    double t_ = 0.0;

    // Plot settings - OPTIMIZED FOR RASPBERRY PI 400
    static constexpr int maxPoints_ = 600;      // Reduced from 1500 (faster rendering)
    static constexpr double windowSeconds_ = 6.0;
    static constexpr int updateInterval_ = 100; // 100ms = 10Hz (was 50ms/20Hz)
};

#endif // MAINWINDOW_H

