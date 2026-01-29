// ============================================================================
// CureCraft Patient Monitor - Real-time Charting Application
// ============================================================================

class PatientMonitor {
    constructor() {
        // Authentication Guard
        if (!sessionStorage.getItem('authenticated')) {
            window.location.href = 'login.html';
            return;
        }

        // Configuration
        this.config = {
            windowSeconds: 6.0,          // Display window (seconds)
            maxPoints: 600,              // Maximum data points to store
            updateRate: 20,              // Expected update rate (Hz)
            reconnectDelay: 2000,        // WebSocket reconnect delay (ms)
        };

        // Chart colors (updated to match new design)
        this.chartColors = {
            ecg: '#10b981',
            spo2: '#a78bfa',
            resp: '#60a5fa',
            pleth: '#f59e0b',
        };

        // Chart configurations
        this.charts = {
            ecg: {
                id: 'ecg',
                canvas: document.getElementById('ecgCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: this.chartColors.ecg,
                range: { min: 0.0, max: 1.2 },  // Adjusted for realistic ECG waveform
                visible: true,
                container: document.getElementById('ecgContainer'),
                lastRenderTime: 0,
                targetFPS: 30,  // Limit to 30 FPS to prevent multi-tab speed-up
            },
            spo2: {
                id: 'spo2',
                canvas: document.getElementById('spo2Canvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: this.chartColors.spo2,
                range: { min: 95.0, max: 100.0 },  // SpO2 percentage range
                visible: true,
                container: document.getElementById('spo2Container'),
                lastRenderTime: 0,
                targetFPS: 30,
            },
            resp: {
                id: 'resp',
                canvas: document.getElementById('respCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: this.chartColors.resp,
                range: { min: 0.0, max: 1.0 },  // Adjusted for respiratory waveform (0-1 normalized)
                visible: true,
                container: document.getElementById('respContainer'),
                lastRenderTime: 0,
                targetFPS: 30,
            },
            pleth: {
                id: 'pleth',
                canvas: document.getElementById('plethCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: this.chartColors.pleth,
                range: { min: 0.0, max: 1.2 },  // Plethysmograph waveform
                visible: true,
                container: document.getElementById('plethContainer'),
                lastRenderTime: 0,
                targetFPS: 30,
            }
        };

        // DOM Elements
        this.dom = {
            // Status indicators
            statusDot: document.getElementById('statusDot'),
            statusText: document.getElementById('statusText'),

            // Vital signs cards - using correct IDs from HTML
            hrValue: document.getElementById('hrValue'),
            hrStatus: document.getElementById('hrStatus'),
            spo2Value: document.getElementById('spo2Value'),
            spo2Status: document.getElementById('spo2Status'),
            respValue: document.getElementById('respValue'),
            respStatus: document.getElementById('respStatus'),
            bpValue: document.getElementById('bpValue'),
            bpStatus: document.getElementById('bpStatus'),
            tempCoreValue: document.getElementById('tempCoreValue'),
            tempCoreStatus: document.getElementById('tempCoreStatus'),
            tempSkinValue: document.getElementById('tempSkinValue'),
            tempSkinStatus: document.getElementById('tempSkinStatus'),

            // Footer
            runtime: document.getElementById('runtime'),
            updateRate: document.getElementById('updateRate'),
            dataPoints: document.getElementById('dataPoints'),

            // Session
            sessionDuration: document.getElementById('sessionDuration'),

            // Buttons
            btnLogout: document.getElementById('btnLogout'),
            btnExport: document.getElementById('btnExport'),
        };

        // State
        this.eventSource = null;
        this.isConnected = false;
        this.animationFrameId = null;
        this.sessionStartTime = Date.now();
        this.totalDataPoints = 0;

        // Vital signs thresholds
        this.thresholds = {
            hr: { min: 60, max: 100, critical: { min: 40, max: 150 } },
            spo2: { min: 95, max: 100, critical: { min: 90, max: 100 } },
            resp: { min: 12, max: 20, critical: { min: 8, max: 30 } },
            bp_systolic: { min: 90, max: 140, critical: { min: 70, max: 180 } },
            temp: { min: 36.5, max: 37.5, critical: { min: 35, max: 39 } },
        };

        // Initialize
        this.init();
    }

    init() {
        // Setup canvases
        Object.values(this.charts).forEach(chart => {
            if (chart.canvas) {
                this.setupCanvas(chart);
            }
        });

        // Setup event listeners
        this.setupEventListeners();

        // Connect to server
        this.connect();

        // Start render loop
        this.startRenderLoop();

        // Start session timer
        this.startSessionTimer();
    }

    setupCanvas(chart) {
        chart.ctx = chart.canvas.getContext('2d', { alpha: false });
        this.resizeCanvas(chart);

        // High DPI support
        const dpr = window.devicePixelRatio || 1;
        const rect = chart.canvas.getBoundingClientRect();
        chart.canvas.width = rect.width * dpr;
        chart.canvas.height = rect.height * dpr;
        chart.ctx.scale(dpr, dpr);
        chart.canvas.style.width = rect.width + 'px';
        chart.canvas.style.height = rect.height + 'px';
    }

    resizeCanvas(chart) {
        const rect = chart.canvas.getBoundingClientRect();
        chart.width = rect.width;
        chart.height = rect.height;
    }

    setupEventListeners() {
        // Logout button
        if (this.dom.btnLogout) {
            this.dom.btnLogout.addEventListener('click', () => this.handleLogout());
        }

        // Export button
        if (this.dom.btnExport) {
            this.dom.btnExport.addEventListener('click', () => this.handleExport());
        }

        // Window resize
        window.addEventListener('resize', () => {
            Object.values(this.charts).forEach(chart => {
                if (chart.canvas) {
                    this.setupCanvas(chart);
                }
            });
        });
    }

    handleLogout() {
        if (confirm('Are you sure you want to logout?')) {
            sessionStorage.removeItem('authenticated');
            window.location.href = 'login.html';
        }
    }

    handleExport() {
        const exportData = {
            timestamp: new Date().toISOString(),
            session_duration: this.getSessionDuration(),
            total_data_points: this.totalDataPoints,
            charts: {}
        };

        // Export last 100 points from each chart
        Object.entries(this.charts).forEach(([key, chart]) => {
            const lastPoints = Math.min(100, chart.data.x.length);
            exportData.charts[key] = {
                timestamps: chart.data.x.slice(-lastPoints),
                values: chart.data.y.slice(-lastPoints)
            };
        });

        // Create and download file
        const blob = new Blob([JSON.stringify(exportData, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `curecraft-export-${Date.now()}.json`;
        document.body.appendChild(a);
        a.click();
        document.body.removeChild(a);
        URL.revokeObjectURL(url);

        console.log('✅ Data exported successfully');
    }

    startSessionTimer() {
        setInterval(() => {
            if (this.dom.sessionDuration) {
                const duration = this.getSessionDuration();
                const minutes = Math.floor(duration / 60);
                const seconds = duration % 60;
                this.dom.sessionDuration.textContent =
                    `${String(minutes).padStart(2, '0')}:${String(seconds).padStart(2, '0')}`;
            }
        }, 1000);
    }

    getSessionDuration() {
        return Math.floor((Date.now() - this.sessionStartTime) / 1000);
    }

    connect() {
        this.updateStatus('Connecting...', false);

        try {
            // Use Server-Sent Events for real-time data streaming
            this.eventSource = new EventSource('/ws');

            this.eventSource.onopen = () => {
                console.log('✅ Connected to server');
                this.updateStatus('Connected', true);
                this.isConnected = true;
            };

            this.eventSource.onmessage = (event) => {
                try {
                    const data = JSON.parse(event.data);
                    this.onDataReceived(data);
                } catch (error) {
                    console.error('Failed to parse data:', error);
                }
            };

            this.eventSource.onerror = (error) => {
                console.error('❌ Connection error:', error);
                this.updateStatus('Disconnected', false);
                this.isConnected = false;

                if (this.eventSource) {
                    this.eventSource.close();
                }

                // Attempt reconnection
                setTimeout(() => {
                    console.log('🔄 Attempting to reconnect...');
                    this.connect();
                }, this.config.reconnectDelay);
            };
        } catch (error) {
            console.error('Failed to connect:', error);
            this.updateStatus('Connection Failed', false);
        }
    }

    onDataReceived(data) {
        const { ecg, spo2, resp, pleth, bp_systolic, bp_diastolic, temp_cavity, temp_skin, timestamp, sensors } = data;

        // Handle sensor attachment
        if (sensors) {
            this.handleSensorStatus(sensors);
        }

        // Add data points to charts
        if (typeof ecg !== 'undefined') {
            this.addDataPoint('ecg', timestamp, ecg);
            this.totalDataPoints++;
        }
        if (typeof spo2 !== 'undefined') {
            this.addDataPoint('spo2', timestamp, spo2);
            this.totalDataPoints++;
        }
        if (typeof resp !== 'undefined') {
            this.addDataPoint('resp', timestamp, resp);
            this.totalDataPoints++;
        }
        if (typeof pleth !== 'undefined') {
            this.addDataPoint('pleth', timestamp, pleth);
            this.totalDataPoints++;
        }

        // Update vital signs summary cards
        this.updateVitalSigns(data);

        // Update footer
        this.updateFooter(timestamp);
    }

    handleSensorStatus(sensors) {
        // Show/hide charts based on sensor attachment
        this.updateChartVisibility('ecg', sensors.ecg);
        this.updateChartVisibility('spo2', sensors.spo2);
        this.updateChartVisibility('pleth', sensors.spo2);  // Pleth depends on SpO2 sensor
        this.updateChartVisibility('resp', sensors.resp);
    }

    updateChartVisibility(chartName, isAttached) {
        const chart = this.charts[chartName];
        if (!chart) return;

        chart.visible = isAttached;
        chart.container.classList.toggle('hidden', !isAttached);
    }

    updateVitalSigns(data) {
        // Calculate and update heart rate (from ECG peaks)
        const hr = this.calculateHeartRate();
        if (hr > 0 && this.dom.hrValue) {
            this.updateVitalCard('hr', hr, this.thresholds.hr);
        }

        // Update SpO2 percentage (backend sends percentage directly, not normalized)
        if (typeof data.spo2 !== 'undefined' && this.dom.spo2Value) {
            const spo2Percent = Math.round(data.spo2);  // Already a percentage (96-99)
            this.updateVitalCard('spo2', spo2Percent, this.thresholds.spo2);
        }

        // Calculate respiratory rate
        const respRate = this.calculateRespiratoryRate();
        if (respRate > 0 && this.dom.respValue) {
            this.updateVitalCard('resp', respRate, this.thresholds.resp);
        }

        // Update blood pressure
        if (this.dom.bpValue) {
            if (data.sensors && data.sensors.nibp && typeof data.bp_systolic !== 'undefined') {
                const sys = data.bp_systolic.toFixed(0);
                const dia = data.bp_diastolic.toFixed(0);
                this.dom.bpValue.textContent = `${sys}/${dia}`;
                this.updateVitalStatus('bp', data.bp_systolic, this.thresholds.bp_systolic);
            } else {
                this.dom.bpValue.textContent = '--/--';
                this.updateVitalStatus('bp', null);
            }
        }

        // Update temperatures
        if (this.dom.tempCoreValue && this.dom.tempSkinValue) {
            if (data.sensors && data.sensors.temp && typeof data.temp_cavity !== 'undefined') {
                this.updateVitalCard('tempCore', data.temp_cavity, this.thresholds.temp);
                this.updateVitalCard('tempSkin', data.temp_skin, this.thresholds.temp);
            } else {
                this.dom.tempCoreValue.textContent = '--.-';
                this.dom.tempSkinValue.textContent = '--.-';
                this.updateVitalStatus('tempCore', null);
                this.updateVitalStatus('tempSkin', null);
            }
        }
    }

    updateVitalCard(name, value, thresholds) {
        const valueEl = this.dom[`${name}Value`];
        if (valueEl) {
            valueEl.textContent = typeof value === 'number' ? Math.round(value) : value;
        }
        this.updateVitalStatus(name, value, thresholds);
    }

    updateVitalStatus(name, value, thresholds) {
        const statusEl = this.dom[`${name}Status`];
        if (!statusEl) return;

        if (value === null || value === undefined) {
            statusEl.textContent = 'No Sensor';
            statusEl.className = 'vital-card-status warning';
            return;
        }

        if (thresholds) {
            if (value < thresholds.critical.min || value > thresholds.critical.max) {
                statusEl.textContent = 'Critical';
                statusEl.className = 'vital-card-status critical';
            } else if (value < thresholds.min || value > thresholds.max) {
                statusEl.textContent = 'Warning';
                statusEl.className = 'vital-card-status warning';
            } else {
                statusEl.textContent = 'Normal';
                statusEl.className = 'vital-card-status normal';
            }
        }
    }

    calculateHeartRate() {
        // Simple peak detection on ECG data (last 2 seconds)
        const chart = this.charts.ecg;
        if (!chart || chart.data.x.length < 40) return 0;

        const data = chart.data.y.slice(-40);  // Last 2 seconds at 20Hz
        const threshold = 0.5;
        let peaks = 0;

        for (let i = 1; i < data.length - 1; i++) {
            if (data[i] > threshold && data[i] > data[i - 1] && data[i] > data[i + 1]) {
                peaks++;
            }
        }

        return Math.round(peaks * 30);  // Convert to bpm (2 sec * 30 = 60 sec)
    }

    calculateRespiratoryRate() {
        // Simple peak detection on respiratory data (last 10 seconds)
        const chart = this.charts.resp;
        if (!chart || chart.data.x.length < 200) return 0;

        const data = chart.data.y.slice(-200);  // Last 10 seconds at 20Hz
        const threshold = 0.2;
        let peaks = 0;

        for (let i = 1; i < data.length - 1; i++) {
            if (data[i] > threshold && data[i] > data[i - 1] && data[i] > data[i + 1]) {
                peaks++;
            }
        }

        return Math.round(peaks * 6);  // Convert to breaths per minute
    }

    addDataPoint(chartName, x, y) {
        const chart = this.charts[chartName];
        if (!chart) return;

        chart.data.x.push(x);
        chart.data.y.push(y);

        // Trim old data points (circular buffer)
        if (chart.data.x.length > this.config.maxPoints * 1.1) {
            const toRemove = Math.floor(this.config.maxPoints * 0.2);
            chart.data.x.splice(0, toRemove);
            chart.data.y.splice(0, toRemove);
        }
    }

    startRenderLoop() {
        const render = (timestamp) => {
            Object.values(this.charts).forEach(chart => {
                if (chart.visible && chart.data.x.length > 0 && chart.canvas) {
                    // Timestamp-based frame limiting to prevent multi-tab speed-up
                    const deltaTime = timestamp - chart.lastRenderTime;
                    const targetInterval = 1000 / chart.targetFPS;  // ms per frame

                    if (deltaTime >= targetInterval) {
                        chart.lastRenderTime = timestamp;
                        this.renderChart(chart);
                    }
                }
            });

            this.animationFrameId = requestAnimationFrame(render);
        };

        render(performance.now());
    }

    renderChart(chart) {
        const ctx = chart.ctx;
        const width = chart.width;
        const height = chart.height;

        // Clear canvas
        ctx.fillStyle = '#0a0e1a';
        ctx.fillRect(0, 0, width, height);

        // Get visible data range
        const data = chart.data;
        if (data.x.length === 0) return;

        const currentTime = data.x[data.x.length - 1];
        const startTime = currentTime - this.config.windowSeconds;

        // Draw grid
        this.drawGrid(ctx, width, height, chart.range);

        // Draw waveform
        ctx.strokeStyle = chart.color;
        ctx.lineWidth = 2;
        ctx.lineCap = 'round';
        ctx.lineJoin = 'round';
        ctx.beginPath();

        let firstPoint = true;

        for (let i = 0; i < data.x.length; i++) {
            const time = data.x[i];
            const value = data.y[i];

            // Only draw points within the visible window
            if (time >= startTime && time <= currentTime) {
                // Map time to x (0 to width)
                const x = ((time - startTime) / this.config.windowSeconds) * width;

                // Map value to y (height to 0)
                const normalizedValue = (value - chart.range.min) / (chart.range.max - chart.range.min);
                const y = height - (normalizedValue * height);

                if (firstPoint) {
                    ctx.moveTo(x, y);
                    firstPoint = false;
                } else {
                    ctx.lineTo(x, y);
                }
            }
        }

        ctx.stroke();
    }

    drawGrid(ctx, width, height, range) {
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
        ctx.lineWidth = 1;

        // Horizontal grid lines
        const numHLines = 5;
        for (let i = 0; i <= numHLines; i++) {
            const y = (i / numHLines) * height;
            ctx.beginPath();
            ctx.moveTo(0, y);
            ctx.lineTo(width, y);
            ctx.stroke();
        }

        // Vertical grid lines
        const numVLines = 6;
        for (let i = 0; i <= numVLines; i++) {
            const x = (i / numVLines) * width;
            ctx.beginPath();
            ctx.moveTo(x, 0);
            ctx.lineTo(x, height);
            ctx.stroke();
        }
    }

    updateStatus(text, connected) {
        this.dom.statusText.textContent = text;

        if (connected) {
            this.dom.statusDot.classList.add('connected');
        } else {
            this.dom.statusDot.classList.remove('connected');
        }
    }

    updateFooter(timestamp) {
        // Update runtime
        if (this.dom.runtime) {
            this.dom.runtime.textContent = timestamp.toFixed(1);
        }

        // Update rate display
        if (this.dom.updateRate) {
            this.dom.updateRate.textContent = this.config.updateRate;
        }

        // Update data points
        if (this.dom.dataPoints) {
            this.dom.dataPoints.textContent = this.totalDataPoints.toLocaleString();
        }
    }
}

// ============================================================================
// Initialize application when DOM is ready
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
    // Check if we are on login page, skip init if so
    if (window.location.pathname.endsWith('login.html')) return;

    console.log('🏥 CureCraft Patient Monitor Initializing...');
    const monitor = new PatientMonitor();
    window.monitor = monitor; // For debugging
    console.log('✅ Monitor ready');
});
