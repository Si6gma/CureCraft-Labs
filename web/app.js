// ============================================================================
// CureCraft Patient Monitor - Real-time Charting Application
// ============================================================================

class PatientMonitor {
    constructor() {
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
                canvas: document.getElementById('ecgCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#00ff88',
                range: { min: -0.2, max: 1.8 },
                visible: true,
                container: document.getElementById('ecgContainer'),
            },
            spo2: {
                canvas: document.getElementById('spo2Canvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#c850ff',
                range: { min: 0.0, max: 1.2 },
                visible: true,
                container: document.getElementById('spo2Container'),
            },
            resp: {
                canvas: document.getElementById('respCanvas'),
                ctx: null,
                data: { x: [], y: [] },
                color: '#c0c0c0',
                range: { min: -1.2, max: 1.2 },
                visible: true,
                container: document.getElementById('respContainer'),
            },
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
            this.setupCanvas(chart);
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
        // Toggle buttons
        document.getElementById('btnToggleECG').addEventListener('click', () => {
            this.toggleSignal('ecg');
        });
        document.getElementById('btnToggleSpO2').addEventListener('click', () => {
            this.toggleSignal('spo2');
        });
        document.getElementById('btnToggleResp').addEventListener('click', () => {
            this.toggleSignal('resp');
        });

        // Window resize
        window.addEventListener('resize', () => {
            Object.values(this.charts).forEach(chart => {
                this.setupCanvas(chart);
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
        const { ecg, spo2, resp, timestamp } = data;

        // Add data points to each chart
        this.addDataPoint('ecg', timestamp, ecg);
        this.addDataPoint('spo2', timestamp, spo2);
        this.addDataPoint('resp', timestamp, resp);

        // Update footer info
        this.updateFooter(timestamp);
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
            Object.entries(this.charts).forEach(([key, chart]) => {
                if (chart.visible && chart.data.x.length > 0) {
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

        for (let i = 0; i < data.x.length; i++) {
            const time = data.x[i];
            const value = data.y[i];

            // Only draw points within the visible window
            if (time >= startTime && time <= currentTime) {
                const x = ((time - startTime) / this.config.windowSeconds) * width;
                const y = height - ((value - chart.range.min) / (chart.range.max - chart.range.min)) * height;

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
        const statusDot = document.getElementById('statusDot');
        const statusText = document.getElementById('statusText');

        statusText.textContent = text;
        
        if (connected) {
            statusDot.classList.add('connected');
        } else {
            statusDot.classList.remove('connected');
        }
    }

    updateFooter(timestamp) {
        // Update runtime
        const runtime = document.getElementById('runtime');
        if (runtime) {
            runtime.textContent = timestamp.toFixed(1);
        }

        // Update rate display (static for now)
        const updateRate = document.getElementById('updateRate');
        if (updateRate) {
            updateRate.textContent = this.config.updateRate;
        }

        // Client count would be updated via separate API call if needed
    }
}

// ============================================================================
// Initialize application when DOM is ready
// ============================================================================

document.addEventListener('DOMContentLoaded', () => {
    console.log('🏥 CureCraft Patient Monitor Initializing...');
    const monitor = new PatientMonitor();
    window.monitor = monitor; // For debugging
    console.log('✅ Monitor ready');
});
