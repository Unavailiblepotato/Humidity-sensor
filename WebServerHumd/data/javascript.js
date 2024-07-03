
  var ctx = document.getElementById('myChart').getContext('2d');
  var myChart = new Chart(ctx, {
      type: 'line',
      data: {
          labels: Array.from({ length: 100 }, (_, i) => i + 1),
          datasets: [{
              label: 'Humidity',
              data: [],
              borderColor: 'rgba(75, 192, 192, 1)',
              borderWidth: 1,
              tension: 0.4,
              mode: 'index',
              fill: true
          }, {
              label: 'Temperature',
              data: [],
              borderColor: 'rgba(255, 99, 132, 1)',
              borderWidth: 1,
              tension: 0.4,
              mode: 'index',
              fill: true
          }]
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
              myChart.data.datasets[0].data = data.humidity;
              myChart.data.datasets[1].data = data.temperature;
              myChart.update();
          });
  }
  
  setInterval(updateChart, 1000); // Update every 10 seconds
  updateChart(); // Initial update
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("temperature").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 10000 ) ;
  
  setInterval(function ( ) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
        document.getElementById("humidity").innerHTML = this.responseText;
      }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
  }, 10000 ) ;