#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <deque>
#include <array>

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
    void pushSample(std::deque<double> &x, std::deque<double> &y, double t, double v);

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

    // Circular buffers for time-series data (deque for O(1) pop_front)
    std::deque<double> xECG_, yECG_;
    std::deque<double> xSpO2_, ySpO2_;
    std::deque<double> xResp_, yResp_;

    double t_ = 0.0;
    int updateCount_ = 0; // For frame skipping

    // Plot settings
    static constexpr int maxPoints_ = 1500;
    static constexpr double windowSeconds_ = 6.0;
};

#endif // MAINWINDOW_H
