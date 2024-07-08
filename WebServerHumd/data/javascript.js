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

function updateChart() {
    fetch('/data')
        .then(response => response.json())
        .then(data => {
            const humidityDatasets = [];
            const temperatureDatasets = [];
            const nodeCardsContainer = document.getElementById('node-cards');
            nodeCardsContainer.innerHTML = ''; // Clear existing cards

            for (const node in data) {
                if (data[node].humidity.length > 0) {
                    // Add node data to the datasets
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

                    // Create a card for each node
                    const card = document.createElement('div');
                    card.className = 'col s12 m6';
                    card.innerHTML = `
                        <div class="card">
                            <div class="card-content">
                                <span class="card-title">${node}</span>
                                <p>Humidity: ${data[node].humidity[data[node].humidity.length - 1]}%</p>
                                <p>Temperature: ${data[node].temperature[data[node].temperature.length - 1]}°F</p>
                            </div>
                        </div>
                    `;
                    nodeCardsContainer.appendChild(card);
                }
            }

            myChart.data.datasets = [...humidityDatasets, ...temperatureDatasets];
            myChart.update();
        });
}

setInterval(updateChart, 10000); // Update every 10 seconds
updateChart(); // Initial update