// https://github.com/njanssen/node-arduino-nano-33-ble

const Connector = require('./connector.js')
const connector = new Connector(['imu', 'timing']);

run = async function(){
	console.log('Collector started...'); 

	connector.on('connected', id => {
		console.log(`Connected`)

		connector.write('timing', 2000);

		connector.on('imu', data => { 
			//console.log('.'); 
			console.log('IMU values:', data) 
		});

		connector.on('environment', data => { 
			//console.log('.'); 
			//console.log('Environment values:', data); 
		});
	});

	connector.on('error', async err => { 
		console.error(err.message); 
		await connector.connect();
	});

	connector.on('disconnected', async id => { 
		console.log(`Disconnected`) 
		await connector.connect();
	});

	await connector.connect();
}

run();
