const https = require('https');

const now = new Date;
const date = now.toISOString().split('T')[0];
const hour = now.getHours().toString();
const req = https.get(`https://api.porssisahko.net/v1/price.json?date=${date}&hour=${hour}`, (res) => {
    let data = '';
    let price = '';
    res.on('data', (d) => {
        data += d;
    })
    res.on('end', () => {
        price = JSON.parse(data).price.toFixed();
        console.log(price+' cent/kWh');
    })

}).on('error', (err) => {
    console.log("Error: " + err.message);
});

