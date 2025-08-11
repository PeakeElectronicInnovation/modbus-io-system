// Lightweight Chart Replacement - ~3KB vs 207KB Chart.js
// Simple SVG-based line chart for temperature data

class LightweightChart {
    constructor(canvas, config) {
        this.canvas = canvas;
        this.config = config;
        this.data = config.data;
        this.options = config.options || {};
        this.width = 800;
        this.height = 400;
        this.padding = { top: 20, right: 20, bottom: 60, left: 60 };
        this.chartArea = {
            x: this.padding.left,
            y: this.padding.top,
            width: this.width - this.padding.left - this.padding.right,
            height: this.height - this.padding.top - this.padding.bottom
        };
        
        this.init();
    }
    
    init() {
        // Clear canvas and create SVG
        this.canvas.innerHTML = '';
        this.svg = document.createElementNS('http://www.w3.org/2000/svg', 'svg');
        this.svg.setAttribute('width', '100%');
        this.svg.setAttribute('height', '100%');
        this.svg.setAttribute('viewBox', `0 0 ${this.width} ${this.height}`);
        this.svg.style.background = '#fff';
        this.canvas.appendChild(this.svg);
        
        this.render();
    }
    
    render() {
        // Clear existing content
        this.svg.innerHTML = '';
        
        if (!this.data.datasets || this.data.datasets.length === 0) return;
        
        // Calculate scales
        this.calculateScales();
        
        // Draw grid
        this.drawGrid();
        
        // Draw axes
        this.drawAxes();
        
        // Draw data lines
        this.drawDataLines();
        
        // Add tooltips
        this.addTooltips();
    }
    
    calculateScales() {
        const labels = this.data.labels || [];
        const datasets = this.data.datasets || [];
        
        // X scale (time)
        this.xScale = {
            min: 0,
            max: Math.max(labels.length - 1, 1),
            range: this.chartArea.width
        };
        
        // Y scale (temperature)
        let yMin = Infinity;
        let yMax = -Infinity;
        
        datasets.forEach(dataset => {
            if (dataset.data) {
                dataset.data.forEach(value => {
                    if (value !== null && !isNaN(value)) {
                        yMin = Math.min(yMin, value);
                        yMax = Math.max(yMax, value);
                    }
                });
            }
        });
        
        if (yMin === Infinity) {
            yMin = 0;
            yMax = 100;
        }
        
        // Add padding to Y scale
        const yPadding = (yMax - yMin) * 0.1;
        this.yScale = {
            min: yMin - yPadding,
            max: yMax + yPadding,
            range: this.chartArea.height
        };
    }
    
    drawGrid() {
        const gridGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
        gridGroup.setAttribute('class', 'grid');
        
        // Horizontal grid lines
        for (let i = 0; i <= 5; i++) {
            const y = this.chartArea.y + (this.chartArea.height / 5) * i;
            const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            line.setAttribute('x1', this.chartArea.x);
            line.setAttribute('y1', y);
            line.setAttribute('x2', this.chartArea.x + this.chartArea.width);
            line.setAttribute('y2', y);
            line.setAttribute('stroke', '#e0e0e0');
            line.setAttribute('stroke-width', '1');
            gridGroup.appendChild(line);
        }
        
        // Vertical grid lines
        const labelCount = Math.min(this.data.labels.length, 10);
        for (let i = 0; i <= labelCount; i++) {
            const x = this.chartArea.x + (this.chartArea.width / labelCount) * i;
            const line = document.createElementNS('http://www.w3.org/2000/svg', 'line');
            line.setAttribute('x1', x);
            line.setAttribute('y1', this.chartArea.y);
            line.setAttribute('x2', x);
            line.setAttribute('y2', this.chartArea.y + this.chartArea.height);
            line.setAttribute('stroke', '#e0e0e0');
            line.setAttribute('stroke-width', '1');
            gridGroup.appendChild(line);
        }
        
        this.svg.appendChild(gridGroup);
    }
    
    drawAxes() {
        // X axis
        const xAxis = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        xAxis.setAttribute('x1', this.chartArea.x);
        xAxis.setAttribute('y1', this.chartArea.y + this.chartArea.height);
        xAxis.setAttribute('x2', this.chartArea.x + this.chartArea.width);
        xAxis.setAttribute('y2', this.chartArea.y + this.chartArea.height);
        xAxis.setAttribute('stroke', '#333');
        xAxis.setAttribute('stroke-width', '2');
        this.svg.appendChild(xAxis);
        
        // Y axis
        const yAxis = document.createElementNS('http://www.w3.org/2000/svg', 'line');
        yAxis.setAttribute('x1', this.chartArea.x);
        yAxis.setAttribute('y1', this.chartArea.y);
        yAxis.setAttribute('x2', this.chartArea.x);
        yAxis.setAttribute('y2', this.chartArea.y + this.chartArea.height);
        yAxis.setAttribute('stroke', '#333');
        yAxis.setAttribute('stroke-width', '2');
        this.svg.appendChild(yAxis);
        
        // X axis labels
        const labelCount = Math.min(this.data.labels.length, 10);
        for (let i = 0; i <= labelCount; i++) {
            const labelIndex = Math.floor((this.data.labels.length - 1) * i / labelCount);
            const label = this.data.labels[labelIndex] || '';
            const x = this.chartArea.x + (this.chartArea.width / labelCount) * i;
            
            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', x);
            text.setAttribute('y', this.chartArea.y + this.chartArea.height + 20);
            text.setAttribute('text-anchor', 'middle');
            text.setAttribute('font-size', '12');
            text.setAttribute('fill', '#666');
            text.textContent = label;
            this.svg.appendChild(text);
        }
        
        // Y axis labels
        for (let i = 0; i <= 5; i++) {
            const value = this.yScale.min + (this.yScale.max - this.yScale.min) * (5 - i) / 5;
            const y = this.chartArea.y + (this.chartArea.height / 5) * i;
            
            const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            text.setAttribute('x', this.chartArea.x - 10);
            text.setAttribute('y', y + 4);
            text.setAttribute('text-anchor', 'end');
            text.setAttribute('font-size', '12');
            text.setAttribute('fill', '#666');
            text.textContent = value.toFixed(1);
            this.svg.appendChild(text);
        }
        
        // Axis titles
        if (this.options.scales?.x?.title?.display) {
            const xTitle = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            xTitle.setAttribute('x', this.chartArea.x + this.chartArea.width / 2);
            xTitle.setAttribute('y', this.height - 10);
            xTitle.setAttribute('text-anchor', 'middle');
            xTitle.setAttribute('font-size', '14');
            xTitle.setAttribute('fill', '#333');
            xTitle.textContent = this.options.scales.x.title.text;
            this.svg.appendChild(xTitle);
        }
        
        if (this.options.scales?.y?.title?.display) {
            const yTitle = document.createElementNS('http://www.w3.org/2000/svg', 'text');
            yTitle.setAttribute('x', 15);
            yTitle.setAttribute('y', this.chartArea.y + this.chartArea.height / 2);
            yTitle.setAttribute('text-anchor', 'middle');
            yTitle.setAttribute('font-size', '14');
            yTitle.setAttribute('fill', '#333');
            yTitle.setAttribute('transform', `rotate(-90, 15, ${this.chartArea.y + this.chartArea.height / 2})`);
            yTitle.textContent = this.options.scales.y.title.text;
            this.svg.appendChild(yTitle);
        }
    }
    
    drawDataLines() {
        this.data.datasets.forEach((dataset, datasetIndex) => {
            if (!dataset.data || dataset.hidden) return;
            
            const color = dataset.borderColor || '#333';
            const points = [];
            
            dataset.data.forEach((value, index) => {
                if (value !== null && !isNaN(value)) {
                    const x = this.chartArea.x + (index / Math.max(this.xScale.max, 1)) * this.chartArea.width;
                    const y = this.chartArea.y + this.chartArea.height - 
                             ((value - this.yScale.min) / (this.yScale.max - this.yScale.min)) * this.chartArea.height;
                    points.push({ x, y, value, index });
                }
            });
            
            if (points.length < 2) return;
            
            // Draw line
            const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');
            let pathData = `M ${points[0].x} ${points[0].y}`;
            
            for (let i = 1; i < points.length; i++) {
                if (dataset.tension && dataset.tension > 0) {
                    // Simple curve approximation
                    const prevPoint = points[i - 1];
                    const currPoint = points[i];
                    const cp1x = prevPoint.x + (currPoint.x - prevPoint.x) * 0.3;
                    const cp1y = prevPoint.y;
                    const cp2x = currPoint.x - (currPoint.x - prevPoint.x) * 0.3;
                    const cp2y = currPoint.y;
                    pathData += ` C ${cp1x} ${cp1y}, ${cp2x} ${cp2y}, ${currPoint.x} ${currPoint.y}`;
                } else {
                    pathData += ` L ${points[i].x} ${points[i].y}`;
                }
            }
            
            path.setAttribute('d', pathData);
            path.setAttribute('stroke', color);
            path.setAttribute('stroke-width', dataset.borderWidth || 2);
            path.setAttribute('fill', 'none');
            path.setAttribute('data-dataset', datasetIndex);
            this.svg.appendChild(path);
            
            // Draw points
            if (dataset.pointRadius > 0) {
                points.forEach(point => {
                    const circle = document.createElementNS('http://www.w3.org/2000/svg', 'circle');
                    circle.setAttribute('cx', point.x);
                    circle.setAttribute('cy', point.y);
                    circle.setAttribute('r', dataset.pointRadius || 1);
                    circle.setAttribute('fill', color);
                    circle.setAttribute('data-dataset', datasetIndex);
                    circle.setAttribute('data-index', point.index);
                    circle.setAttribute('data-value', point.value);
                    this.svg.appendChild(circle);
                });
            }
        });
    }
    
    addTooltips() {
        // Simple tooltip on hover
        const tooltip = document.createElement('div');
        tooltip.style.position = 'absolute';
        tooltip.style.background = 'rgba(0,0,0,0.8)';
        tooltip.style.color = 'white';
        tooltip.style.padding = '8px';
        tooltip.style.borderRadius = '4px';
        tooltip.style.fontSize = '12px';
        tooltip.style.pointerEvents = 'none';
        tooltip.style.display = 'none';
        tooltip.style.zIndex = '1000';
        document.body.appendChild(tooltip);
        
        this.svg.addEventListener('mousemove', (e) => {
            const rect = this.svg.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            
            // Find nearest data point
            let nearestPoint = null;
            let minDistance = Infinity;
            
            this.data.datasets.forEach((dataset, datasetIndex) => {
                if (!dataset.data || dataset.hidden) return;
                
                dataset.data.forEach((value, index) => {
                    if (value !== null && !isNaN(value)) {
                        const pointX = this.chartArea.x + (index / Math.max(this.xScale.max, 1)) * this.chartArea.width;
                        const pointY = this.chartArea.y + this.chartArea.height - 
                                     ((value - this.yScale.min) / (this.yScale.max - this.yScale.min)) * this.chartArea.height;
                        
                        const distance = Math.sqrt((x - pointX) ** 2 + (y - pointY) ** 2);
                        if (distance < minDistance && distance < 20) {
                            minDistance = distance;
                            nearestPoint = {
                                dataset: datasetIndex,
                                index,
                                value,
                                label: this.data.labels[index],
                                datasetLabel: dataset.label
                            };
                        }
                    }
                });
            });
            
            if (nearestPoint) {
                tooltip.innerHTML = `
                    <div>${nearestPoint.datasetLabel}: ${nearestPoint.value.toFixed(1)}°C</div>
                    <div>${nearestPoint.label} ago</div>
                `;
                tooltip.style.left = (e.clientX + 10) + 'px';
                tooltip.style.top = (e.clientY - 10) + 'px';
                tooltip.style.display = 'block';
            } else {
                tooltip.style.display = 'none';
            }
        });
        
        this.svg.addEventListener('mouseleave', () => {
            tooltip.style.display = 'none';
        });
    }
    
    update() {
        this.render();
    }
    
    isDatasetVisible(index) {
        return !this.data.datasets[index]?.hidden;
    }
    
    setDatasetVisibility(index, visible) {
        if (this.data.datasets[index]) {
            this.data.datasets[index].hidden = !visible;
            this.render();
        }
    }
}

// Global Chart constructor replacement
window.Chart = LightweightChart;
