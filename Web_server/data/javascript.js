var ctx = document.getElementById('myChart').getContext('2d');
var myChart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: Array.from({ length: 100 }, (_, i) => i + 1),
        datasets: []
    },
    options: {
        scales: {
            x: {
                type: 'linear',
                position: 'bottom'
            }
        },
        interaction: {
            intersect: false,
            mode: 'index',
        },
        animation: {
            duration: 1, // Smooth transition duration in milliseconds
            easing: 'easeInOutQuad' // Easing function for the animation
        }
    },
    tooltips: {
        mode: 'index',
        intersect: false,
        callbacks: {
            label: function(tooltipItem, data) {
                var label = data.datasets[tooltipItem.datasetIndex].label || '';
                if (label) {
                    label += ': ';
                }
                label += Math.round(tooltipItem.yLabel * 100) / 100;
                return label;
            }
        }
    },
});

var selectedNodes = {};
var allNodesSelected = false;

function toggleNode(node) {
    selectedNodes[node] = !selectedNodes[node];
    updateChart();
}

function updateToggleAllButton() {
    const allNodes = Object.keys(selectedNodes);
    const allSelected = allNodes.every(node => selectedNodes[node]);
    const button = document.getElementById('toggle-all-btn');
    button.textContent = allSelected ? 'Deselect All' : 'Select All';
    allNodesSelected = allSelected;
}

function toggleAllNodes() {
    const allNodes = Object.keys(selectedNodes);
    allNodesSelected = !allNodesSelected;
    allNodes.forEach(node => {
        selectedNodes[node] = allNodesSelected;
    });
    updateChart();
    updateToggleAllButton();
}

function updateChart() {
    fetch('/data')
        .then(response => response.json())
        .then(data => {
            const humidityDatasets = [];
            const temperatureDatasets = [];
            const nodeCardsContainer = document.getElementById('node-cards');
            const buttonsContainer = document.getElementById('buttons-container');
            
            if (!nodeCardsContainer || !buttonsContainer) {
                console.error('Required containers not found in the DOM.');
                return;
            }

            nodeCardsContainer.innerHTML = ''; // Clear existing cards
            buttonsContainer.innerHTML = ''; // Clear existing buttons

            for (const node in data) {
                if (data[node].humidity.length > 0) {
                    if (selectedNodes[node] === undefined) {
                        selectedNodes[node] = true;
                    }

                    // Create a button for each node
                    const button = document.createElement('button');
                    button.className = `btn ${selectedNodes[node] ? 'selected' : ''}`;
                    button.textContent = node;
                    button.onclick = () => {
                        toggleNode(node);
                        updateToggleAllButton();
                    };
                    buttonsContainer.appendChild(button);

                    // Create a card for each node
                    const card = document.createElement('div');
                    card.className = 'col s12 m3';
                    card.innerHTML = `
                        <div class="card">
                            <div class="card-content">
                                <span class="card-title">${node}</span>
                                <p>Humidity: ${data[node].humidity[data[node].humidity.length - 1].toFixed(2)}%</p>
                                <p>Temperature: ${data[node].temperature[data[node].temperature.length - 1].toFixed(2)}&degF</p>
                            </div>
                        </div>
                    `;
                    nodeCardsContainer.appendChild(card);

                    if (selectedNodes[node]) {
                        humidityDatasets.push({
                            label: `Humidity ${node}`,
                            data: data[node].humidity,
                            borderColor: 'rgba(75, 192, 192, 1)',
                            borderWidth: 1,
                            tension: 0.4,
                            fill: true
                        });
                        temperatureDatasets.push({
                            label: `Temperature ${node}`,
                            data: data[node].temperature,
                            borderColor: 'rgba(255, 99, 132, 1)',
                            borderWidth: 1,
                            tension: 0.4,
                            fill: true
                        });
                    }
                }
            }

            myChart.data.datasets = [...humidityDatasets, ...temperatureDatasets];
            myChart.update();
            updateToggleAllButton();
        })
        .catch(error => console.error('Error updating chart:', error));
}

document.getElementById('toggle-all-btn').onclick = toggleAllNodes;

setInterval(updateChart, 10000); // Update every 10 seconds
updateChart(); // Initial update