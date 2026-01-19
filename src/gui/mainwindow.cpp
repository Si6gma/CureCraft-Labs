#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <QtGlobal>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupPlots();

    // Reserve space to prevent reallocations (deque pre-allocates)
    xECG_.resize(maxPoints_);
    yECG_.resize(maxPoints_);
    xSpO2_.resize(maxPoints_);
    ySpO2_.resize(maxPoints_);
    xResp_.resize(maxPoints_);
    yResp_.resize(maxPoints_);

    // 50 ms = 20 Hz GUI update. Optimized for Raspberry Pi.
    timer_.setInterval(50);
    connect(&timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_.start();

    // Ensure button states match visibility at start
    ui->plotECG->setVisible(ui->btnToggleECG->isChecked());
    ui->plotSpO2->setVisible(ui->btnToggleSpO2->isChecked());
    ui->plotResp->setVisible(ui->btnToggleResp->isChecked());
}

MainWindow::~MainWindow()
{
    timer_.stop();
    delete ui;
}

void MainWindow::setupPlots()
{
    // Reusable pens and brushes to reduce allocations
    const QPen grayPen(Qt::gray);
    const QPen lightGrayPen(Qt::lightGray);
    const QBrush blackBrush(Qt::black);

    // Common styling helper lambda
    auto stylePlot = [&](QCustomPlot *p)
    {
        p->setBackground(blackBrush);
        p->xAxis->setBasePen(grayPen);
        p->yAxis->setBasePen(grayPen);
        p->xAxis->setTickPen(grayPen);
        p->yAxis->setTickPen(grayPen);
        p->xAxis->setSubTickPen(grayPen);
        p->yAxis->setSubTickPen(grayPen);
        p->xAxis->setTickLabelColor(Qt::lightGray);
        p->yAxis->setTickLabelColor(Qt::lightGray);

        p->legend->setVisible(false);
        p->setNotAntialiasedElements(QCP::aeAll); // Disable antialiasing for weak GPUs
        p->setNoAntialiasingOnDrag(true);
        p->setInteraction(QCP::iRangeDrag, false);
        p->setInteraction(QCP::iRangeZoom, false);

        p->addGraph();
        p->graph(0)->setAdaptiveSampling(true);
    };

    // ECG
    stylePlot(ui->plotECG);
    ui->plotECG->graph(0)->setPen(QPen(Qt::green, 1.5));
    ui->plotECG->yAxis->setRange(-0.2, 1.8);
    ui->plotECG->xAxis->setRange(0, windowSeconds_);

    // SpO2
    stylePlot(ui->plotSpO2);
    ui->plotSpO2->graph(0)->setPen(QPen(QColor(200, 80, 255), 1.5)); // purple
    ui->plotSpO2->yAxis->setRange(0.0, 1.2);
    ui->plotSpO2->xAxis->setRange(0, windowSeconds_);

    // Resp
    stylePlot(ui->plotResp);
    ui->plotResp->graph(0)->setPen(QPen(Qt::lightGray, 1.5));
    ui->plotResp->yAxis->setRange(-1.2, 1.2);
    ui->plotResp->xAxis->setRange(0, windowSeconds_);
}

void MainWindow::pushSample(std::deque<double> &x, std::deque<double> &y, double t, double v)
{
    // Deque push_back is O(1), pop_front is O(1) (unlike QVector)
    x.push_back(t);
    y.push_back(v);

    // Remove oldest samples efficiently
    while (x.size() > static_cast<size_t>(maxPoints_))
    {
        x.pop_front();
        y.pop_front();
    }
}

void MainWindow::onTick()
{
    const double dt = timer_.interval() / 1000.0;
    t_ += dt;

    // Generate sensor signals using precomputed parameters
    const double ecgBase = waveParams_.ecgAmplitude * std::sin(2.0 * M_PI * waveParams_.ecgFreq * t_);
    const double phase = std::fmod(t_, 1.0);
    const double spike = std::exp(-std::pow(phase - 0.12, 2) / 0.0007);
    const double ecg = ecgBase + waveParams_.ecgSpikeAmplitude * spike;

    const double spo2 = waveParams_.spO2Base + waveParams_.spO2Amplitude * std::sin(2.0 * M_PI * waveParams_.spO2Freq * t_);
    const double resp = waveParams_.respAmplitude * std::sin(2.0 * M_PI * waveParams_.respFreq * t_);

    // Add samples to circular buffers (O(1) with deque)
    pushSample(xECG_, yECG_, t_, ecg);
    pushSample(xSpO2_, ySpO2_, t_, spo2);
    pushSample(xResp_, yResp_, t_, resp);

    // Cache visibility states
    const bool ecgVisible = ui->plotECG->isVisible();
    const bool spo2Visible = ui->plotSpO2->isVisible();
    const bool respVisible = ui->plotResp->isVisible();

    // Skip redraws if nothing visible
    if (!ecgVisible && !spo2Visible && !respVisible)
    {
        return;
    }

    const double left = t_ - windowSeconds_;
    const double right = t_;

    // Update visible plots efficiently (convert deque to QVector for plotting)
    if (ecgVisible)
    {
        QVector<double> xVec(xECG_.begin(), xECG_.end());
        QVector<double> yVec(yECG_.begin(), yECG_.end());
        ui->plotECG->graph(0)->setData(xVec, yVec);
        ui->plotECG->xAxis->setRange(left, right);
        ui->plotECG->replot(QCustomPlot::rpQueuedReplot);
    }
    if (spo2Visible)
    {
        QVector<double> xVec(xSpO2_.begin(), xSpO2_.end());
        QVector<double> yVec(ySpO2_.begin(), ySpO2_.end());
        ui->plotSpO2->graph(0)->setData(xVec, yVec);
        ui->plotSpO2->xAxis->setRange(left, right);
        ui->plotSpO2->replot(QCustomPlot::rpQueuedReplot);
    }
    if (respVisible)
    {
        QVector<double> xVec(xResp_.begin(), xResp_.end());
        QVector<double> yVec(yResp_.begin(), yResp_.end());
        ui->plotResp->graph(0)->setData(xVec, yVec);
        ui->plotResp->xAxis->setRange(left, right);
        ui->plotResp->replot(QCustomPlot::rpQueuedReplot);
    }
}

void MainWindow::on_btnToggleECG_toggled(bool checked)
{
    ui->plotECG->setVisible(checked);
}

void MainWindow::on_btnToggleSpO2_toggled(bool checked)
{
    ui->plotSpO2->setVisible(checked);
}

void MainWindow::on_btnToggleResp_toggled(bool checked)
{
    ui->plotResp->setVisible(checked);
}
