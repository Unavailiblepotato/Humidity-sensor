document.addEventListener('DOMContentLoaded', function() {
    var ctx1 = document.getElementById('humidityChart').getContext('2d');
    var ctx2 = document.getElementById('temperatureChart').getContext('2d');

    var humidityChart = new Chart(ctx1, {
        type: 'line',
        data: {
            labels: Array.from({ length: 100 }, (_, i) => i + 1),
            datasets: []
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    type: 'linear',
                    position: 'bottom'
                },
                y: {
                    title: {
                        display: true,
                        text: 'Humidity (%)'
                    }
                }
            },
            plugins: {
                title: {
                    display: true,
                    text: 'Humidity Over Time'
                },
                tootlip: {
                    mode: 'index'
                }
            }
        }
    });

    var temperatureChart = new Chart(ctx2, {
        type: 'line',
        data: {
            labels: Array.from({ length: 100 }, (_, i) => i + 1),
            datasets: []
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            scales: {
                x: {
                    type: 'linear',
                    position: 'bottom'
                },
                y: {
                    title: {
                        display: true,
                        text: 'Temperature (&degF)'
                    }
                }
            },
            plugins: {
                title: {
                    display: true,
                    text: 'Temperature Over Time'
                },
                tootlip: {
                    mode: 'index'
                }
            }
        }
    });

    var selectedNodes = {};
    var allNodesSelected = false;
    var colorPalette = [
        'rgba(255, 99, 132, 1)', 'rgba(54, 162, 235, 1)', 'rgba(255, 206, 86, 1)', 'rgba(75, 192, 192, 1)',
        'rgba(153, 102, 255, 1)', 'rgba(255, 159, 64, 1)', 'rgba(199, 199, 199, 1)', 'rgba(83, 102, 255, 1)',
        'rgba(40, 159, 64, 1)', 'rgba(210, 199, 199, 1)', 'rgba(78, 52, 199, 1)', 'rgba(172, 159, 64, 1)'
    ];

    function toggleNode(node) {
        selectedNodes[node] = !selectedNodes[node];
        updateCharts();
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
        updateCharts();
        updateToggleAllButton();
    }

    function updateCharts() {
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

                nodeCardsContainer.innerHTML = '';
                buttonsContainer.innerHTML = '<button id="toggle-all-btn" class="btn">Select All</button>';

                let colorIndex = 0;
                for (const node in data) {
                    if (data[node].humidity.length > 0) {
                        if (selectedNodes[node] === undefined) {
                            selectedNodes[node] = true;
                        }

                        const button = document.createElement('button');
                        button.className = `btn ${selectedNodes[node] ? 'selected' : ''}`;
                        button.textContent = node;
                        button.onclick = () => {
                            toggleNode(node);
                            updateToggleAllButton();
                        };
                        buttonsContainer.appendChild(button);

                        const card = document.createElement('div');
                        card.className = 'card';
                        card.innerHTML = `
                            <div class="card-content">
                                <span class="card-title">${node}</span>
                                <p>Humidity: ${data[node].humidity[data[node].humidity.length - 1].toFixed(2)}%</p>
                                <p>Temperature: ${data[node].temperature[data[node].temperature.length - 1].toFixed(2)}&deg;F</p>
                            </div>
                        `;
                        nodeCardsContainer.appendChild(card);

                        if (selectedNodes[node]) {
                            const color = colorPalette[colorIndex % colorPalette.length];
                            humidityDatasets.push({
                                label: `Humidity ${node}`,
                                data: data[node].humidity,
                                borderColor: color,
                                backgroundColor: color.replace('1)', '0.1)'),
                                borderWidth: 2,
                                tension: 0.4,
                                fill: true
                            });
                            temperatureDatasets.push({
                                label: `Temperature ${node}`,
                                data: data[node].temperature,
                                borderColor: color,
                                backgroundColor: color.replace('1)', '0.1)'),
                                borderWidth: 2,
                                tension: 0.4,
                                fill: true
                            });
                            colorIndex++;
                        }
                    }
                }

                humidityChart.data.datasets = humidityDatasets;
                temperatureChart.data.datasets = temperatureDatasets;
                humidityChart.update();
                temperatureChart.update();
                updateToggleAllButton();
            })
            .catch(error => console.error('Error updating charts:', error));
    }

    document.getElementById('toggle-all-btn').onclick = toggleAllNodes;

    updateCharts();
    setInterval(updateCharts, 10000);
});