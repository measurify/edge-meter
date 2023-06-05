const Connector = require('./connector.js')
const connector = new Connector(['imu', 'orientation', 'environment', 'timing']);

run = async function(){
	console.log('Collector started...'); 

	connector.on('connected', id => {
		console.log(`Connected`)

		connector.write('timing', 2000);

		connector.on('imu', data => { 
			//console.log('IMU values:', data) 
		});

		connector.on('environment', data => { 
			//console.log('Environment values:', data); 
		});

		connector.on('orientation', data => { 
			console.log('Orientation values:', data); 
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
