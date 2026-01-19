#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <cmath>
#include <QtGlobal>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setupPlots();

    // 20 ms = 50 Hz GUI update. On Raspberry Pi you can lower to 25–40 ms if needed.
    timer_.setInterval(20);
    connect(&timer_, &QTimer::timeout, this, &MainWindow::onTick);
    timer_.start();

    // Ensure button states match visibility at start
    ui->plotECG->setVisible(ui->btnToggleECG->isChecked());
    ui->plotSpO2->setVisible(ui->btnToggleSpO2->isChecked());
    ui->plotResp->setVisible(ui->btnToggleResp->isChecked());
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupPlots()
{
    // Common styling helper lambda
    auto stylePlot = [&](QCustomPlot* p) {
        p->setBackground(QBrush(Qt::black));
        p->xAxis->setBasePen(QPen(Qt::gray));
        p->yAxis->setBasePen(QPen(Qt::gray));
        p->xAxis->setTickPen(QPen(Qt::gray));
        p->yAxis->setTickPen(QPen(Qt::gray));
        p->xAxis->setSubTickPen(QPen(Qt::gray));
        p->yAxis->setSubTickPen(QPen(Qt::gray));
        p->xAxis->setTickLabelColor(Qt::lightGray);
        p->yAxis->setTickLabelColor(Qt::lightGray);

        p->legend->setVisible(false);
        p->setNotAntialiasedElements(QCP::aeAll); // can improve performance on weak GPUs
        p->setNoAntialiasingOnDrag(true);

        p->addGraph();
        p->graph(0)->setAdaptiveSampling(true);
    };

    // ECG
    stylePlot(ui->plotECG);
    ui->plotECG->graph(0)->setPen(QPen(Qt::green));
    ui->plotECG->yAxis->setRange(-0.2, 1.8);
    ui->plotECG->xAxis->setRange(0, windowSeconds_);

    // SpO2
    stylePlot(ui->plotSpO2);
    ui->plotSpO2->graph(0)->setPen(QPen(QColor(200, 80, 255))); // purple
    ui->plotSpO2->yAxis->setRange(0.0, 1.2);
    ui->plotSpO2->xAxis->setRange(0, windowSeconds_);

    // Resp
    stylePlot(ui->plotResp);
    ui->plotResp->graph(0)->setPen(QPen(Qt::lightGray));
    ui->plotResp->yAxis->setRange(-1.2, 1.2);
    ui->plotResp->xAxis->setRange(0, windowSeconds_);
}

void MainWindow::pushSample(QVector<double>& x, QVector<double>& y, double t, double v)
{
    x.push_back(t);
    y.push_back(v);

    if (x.size() > maxPoints_) {
        int extra = x.size() - maxPoints_;
        x.remove(0, extra);
        y.remove(0, extra);
    }
}

void MainWindow::onTick()
{
    const double dt = timer_.interval() / 1000.0;
    t_ += dt;

    // --- Fake signals for now (replace with teammates' data later) ---
    // ECG-ish: small sinus + repeating sharp peak (once per second)
    double ecgBase = 0.08 * std::sin(2.0 * M_PI * 1.0 * t_);
    double phase = std::fmod(t_, 1.0);
    double spike = std::exp(-std::pow(phase - 0.12, 2) / 0.0007);
    double ecg = ecgBase + 1.25 * spike;

    // Pleth (SpO2 waveform)
    double spo2 = 0.55 + 0.4 * std::sin(2.0 * M_PI * 1.2 * t_);

    // Resp
    double resp = 0.6 * std::sin(2.0 * M_PI * 0.3 * t_);
    // ---------------------------------------------------------------

    pushSample(xECG_,  yECG_,  t_, ecg);
    pushSample(xSpO2_, ySpO2_, t_, spo2);
    pushSample(xResp_, yResp_, t_, resp);

    const double left = t_ - windowSeconds_;
    const double right = t_;

    // Update plots ONLY if visible (performance)
    if (ui->plotECG->isVisible()) {
        ui->plotECG->graph(0)->setData(xECG_, yECG_);
        ui->plotECG->xAxis->setRange(left, right);
        ui->plotECG->replot(QCustomPlot::rpQueuedReplot);
    }
    if (ui->plotSpO2->isVisible()) {
        ui->plotSpO2->graph(0)->setData(xSpO2_, ySpO2_);
        ui->plotSpO2->xAxis->setRange(left, right);
        ui->plotSpO2->replot(QCustomPlot::rpQueuedReplot);
    }
    if (ui->plotResp->isVisible()) {
        ui->plotResp->graph(0)->setData(xResp_, yResp_);
        ui->plotResp->xAxis->setRange(left, right);
        ui->plotResp->replot(QCustomPlot::rpQueuedReplot);
    }

    // Update right-side numbers (use your label names if different)
    // If your labels are named exactly like earlier:
    // ui->lblHRValue->setText("130"); etc.
    // (Only do this if you have those exact objectNames.)
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
