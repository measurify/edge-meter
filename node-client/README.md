# Measurify Collector

Measurify Collector is Node.js module for the Measurify Meter board. The module makes easy to listen to data from one or more sensors from the Meter board using a BLE connection and send the collected data to a Measurify server.

In oder to use the module, you have to import it, define the charactercist you are interested in and instantiate a new Connector object:

'''
const Connector = require('./connector.js')
const options = { enable: ['imu', 'env'] }
const connector = new Connector(options);
'''

The you have to connect to the Meter and waiting for it:

'''
await connector.connect();
'''

Finaly you have to specify the call backs for the most important events: connection, error and disconnection. In particular, after connetion you can specify callcacks for the different characteristics value changes notification:


connector.on('connected', id => {
	
	connector.on('imu', data => { 
	    console.log('IMU values:', data) 
	});

	connector.on('env', data => { 
		console.log('Environment values:', data); 
	});
});

connector.on('error', async err => { 
	console.error(err.message); 
});

connector.on('disconnected', async id => { 
	console.log(`Disconnected`) 
});
'''


You can control also the rate ar which the Meter will polling data from sensor:

'''
connector.write('timing', 5000);
'''
