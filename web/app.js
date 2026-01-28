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

        // Chart configurations
        this.charts = {
            ecg: {
                id: 'ecg',
                canvas: document.getElementById('ecgCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#00ff88',
                range: { min: -0.5, max: 2.0 },
                visible: true,
                container: document.getElementById('ecgContainer'),
            },
            spo2: {
                id: 'spo2',
                canvas: document.getElementById('spo2Canvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#c850ff',
                range: { min: 0.0, max: 1.2 },
                visible: true,
                container: document.getElementById('spo2Container'),
            },
            resp: {
                id: 'resp',
                canvas: document.getElementById('respCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#c0c0c0',
                range: { min: -1.2, max: 1.2 },
                visible: true,
                container: document.getElementById('respContainer'),
            },
            pleth: {
                id: 'pleth',
                canvas: document.getElementById('plethCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#c850ff', // Same as SpO2
                range: { min: 0.0, max: 1.0 },
                visible: true,
                container: document.getElementById('plethContainer'),
            }
        };

        // DOM Elements for numeric displays
        this.dom = {
            bpValue: document.getElementById('bp-value'),
            tempCavity: document.getElementById('temp-cavity-value'),
            tempSkin: document.getElementById('temp-skin-value'),
            statusDot: document.getElementById('statusDot'),
            statusText: document.getElementById('statusText'),
            runtime: document.getElementById('runtime'),
            updateRate: document.getElementById('updateRate'),
        };

        // State
        this.eventSource = null;
        this.isConnected = false;
        this.animationFrameId = null;

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
        // Toggle buttons (if they exist)
        const toggleIds = ['btnToggleECG', 'btnToggleSpO2', 'btnToggleResp'];
        const signals = ['ecg', 'spo2', 'resp'];

        toggleIds.forEach((id, index) => {
            const btn = document.getElementById(id);
            if (btn) {
                btn.addEventListener('click', () => {
                    this.toggleSignal(signals[index]);
                });
            }
        });

        // Window resize
        window.addEventListener('resize', () => {
            Object.values(this.charts).forEach(chart => {
                if (chart.canvas) {
                    this.setupCanvas(chart);
                }
            });
        });

        // Set initial button states
        Object.entries(this.charts).forEach(([key, chart]) => {
            const btn = document.querySelector(`[data-signal="${key}"]`);
            if (btn) {
                btn.classList.add('active');
            }
        });
    }

    toggleSignal(signalName) {
        const chart = this.charts[signalName];
        if (!chart) return;

        // Manual toggle overrides sensor state temporarily (or just toggles visibility preference)
        // For now, let's keep it simple: manual toggle works, but sensor update might override reasonably
        chart.visible = !chart.visible;
        chart.container.classList.toggle('hidden', !chart.visible);

        const btn = document.querySelector(`[data-signal="${signalName}"]`);
        if (btn) {
            btn.classList.toggle('active', chart.visible);
        }
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

        // Handle sensor attachment (F1 Requirement)
        // Only show charts/data if sensor is attached
        if (sensors) {
            this.handleSensorStatus(sensors);
        }

        // Add data points to charts (only if we have data)
        if (typeof ecg !== 'undefined') this.addDataPoint('ecg', timestamp, ecg);
        if (typeof spo2 !== 'undefined') this.addDataPoint('spo2', timestamp, spo2);
        if (typeof resp !== 'undefined') this.addDataPoint('resp', timestamp, resp);
        if (typeof pleth !== 'undefined') this.addDataPoint('pleth', timestamp, pleth);

        // Update numeric displays
        this.updateNumericDisplays(data);

        // Update footer
        this.updateFooter(timestamp);
    }

    handleSensorStatus(sensors) {
        // This function ensures F1 requirement: display only when sensor attached/detected

        // ECG
        this.updateChartVisibility('ecg', sensors.ecg);

        // SpO2 + Pleth (both depend on SpO2 sensor)
        this.updateChartVisibility('spo2', sensors.spo2);
        this.updateChartVisibility('pleth', sensors.spo2);

        // Respiratory (always derived/available or depends on sensors)
        // For requirement compliance, let's say it depends on resp sensor
        this.updateChartVisibility('resp', sensors.resp);

        // We could also gray out numeric displays if sensors are missing
        // For now, let's leave them updating as "--" if needed, handled in updateNumericDisplays
    }

    updateChartVisibility(chartName, isAttached) {
        const chart = this.charts[chartName];
        if (!chart) return;

        // Show/hide based on sensor attachment
        // We only change visibility if it differs from current intent
        // Note: Manual toggles might conflict here. Let's say sensor attachment is the master switch.

        if (chart.visible !== isAttached) {
            chart.visible = isAttached;
            chart.container.classList.toggle('hidden', !isAttached);

            // Update toggle button if exists
            const btn = document.querySelector(`[data-signal="${chartName}"]`);
            if (btn) {
                btn.classList.toggle('active', isAttached);
            }
        }
    }

    updateNumericDisplays(data) {
        // NIBP
        if (data.sensors && data.sensors.nibp) {
            this.dom.bpValue.textContent = `${data.bp_systolic.toFixed(0)}/${data.bp_diastolic.toFixed(0)}`;
        } else {
            this.dom.bpValue.textContent = '--/--';
        }

        // Temperature (Core)
        if (data.sensors && data.sensors.temp) {
            this.dom.tempCavity.textContent = data.temp_cavity.toFixed(1);
        } else {
            this.dom.tempCavity.textContent = '--.-';
        }

        // Temperature (Skin)
        if (data.sensors && data.sensors.temp) {
            this.dom.tempSkin.textContent = data.temp_skin.toFixed(1);
        } else {
            this.dom.tempSkin.textContent = '--.-';
        }
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
        const render = () => {
            Object.values(this.charts).forEach(chart => {
                if (chart.visible && chart.data.x.length > 0 && chart.canvas) {
                    this.renderChart(chart);
                }
            });

            this.animationFrameId = requestAnimationFrame(render);
        };

        render();
    }

    renderChart(chart) {
        const ctx = chart.ctx;
        const width = chart.width;
        const height = chart.height;

        // Clear canvas
        ctx.fillStyle = '#0a0a0a';
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

        // Optimization: Binary search for start index could be added here
        // For < 1000 points, linear scan is fine on modern JS engines
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
