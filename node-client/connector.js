// https://github.com/njanssen/node-arduino-nano-33-ble

'use strict'

const EventEmitter = require('events')
const { Bluetooth } = require('webbluetooth')

const service_uuid = '8e7c2dae-0000-4b0d-b516-f525649c49ca'
const sampling_period_uuid = '8e7c2dae-0001-4b0d-b516-f525649c49ca'
const imu_uuid = '8e7c2dae-0002-4b0d-b516-f525649c49ca'
const environment_uuid = '8e7c2dae-0003-4b0d-b516-f525649c49ca'
const orientation_uuid = '8e7c2dae-0004-4b0d-b516-f525649c49ca'

class Connector extends EventEmitter {
	bluethoot = null;
	device = null;
	server = null;
	service = null;

	characteristics = [
		{ name: 'sampling_period', uuid: sampling_period_uuid, properties: ['BLEWrite'], structure: ['Int16'], data: { sampling_period: 0 } },
		
		{ name: 'imu', uuid: imu_uuid, properties: ['BLENotify'],
		  structure: ['Float32', 'Float32', 'Float32', 'Float32', 'Float32', 'Float32', 'Float32', 'Float32', 'Float32'],
		  data: { a_x: [], a_y: [], a_z: [], g_x: [], g_y: [], g_z: [], m_x: [], m_y: [], m_z: [] } },
		
		{ name: 'environment', uuid: environment_uuid, properties: ['BLENotify'],
		  structure: ['Float32', 'Float32', 'Float32', 'Float32', 'Float32', 'Float32', 'Float32'],
		  data: { proximity: [], temperature: [], humidity: [], light: [], red: [], green: [], blue: [] } },
		
		{ name: 'orientation', uuid: orientation_uuid, properties: ['BLENotify'],
		  structure: ['Float32', 'Float32', 'Float32'],
		  data: { heading: [], pitch: [], roll: [] } },
	];
	
	enables = [];

	constructor(enables) {
		super();
		this.enables = enables
		this.bluetooth = new Bluetooth()		
	}

	connect = async function(){
		while(!(await this.bluetooth.getAvailability())) { console.log('No Bluetooth interface available, retry...') }
		
		while (!this.device) {
			try { this.device = await this.bluetooth.requestDevice({ filters: [ { services: [service_uuid] } ] })
			} catch (err) { console.log('No device available (' + err + '), retry...');}
		}
		
		//this.device.on(BluetoothDevice.EVENT_DISCONNECTED, event => this.onDisconnected(event) )

		this.server = await this.device.gatt.connect();

		this.service = await this.server.getPrimaryService(service_uuid);
		
		for (const characteristic of this.characteristics) {
			try {
				if (!this.enables.includes(characteristic.name)) continue;
				characteristic.handle = await this.service.getCharacteristic(characteristic.uuid);
				if (characteristic.properties.includes('BLENotify')) {
					characteristic.handle.on('characteristicvaluechanged', event => { this.handleIncoming(characteristic, event.target.value) });
					try { await characteristic.handle.startNotifications(); } catch(err) { }
				}
			} 
			catch (err) { console.log('Characteristic ' + characteristic.name + ' not available in Meter (' + err +')'); }
		}
		this.emit('connected', this.device.id)
		return true;
	}

	disconnect = () => { if(this.server) this.server.disconnect() }
	
	isConnected = () => {  return !this.server ? false : this.server.connected }

	write = function(characteristic_name, data) {
		try {
			const buffer = Buffer.allocUnsafe(2);
			buffer.writeUInt16BE(data); 
			const characteristic = this.characteristics.find(characteristic => characteristic.name === characteristic_name);
			if(!characteristic.handle) { console.log('The characteristic ' + characteristic_name + ' is not enabled!'); }
			else { 
				console.log(characteristic);
			}
		} catch(err) {console.log('Cannot set sampling period ' + data + ' (' + err +')');}
	}

	handleIncoming = (characteristic, dataReceived) => {
		const data = characteristic.data;
		const columns = Object.keys(data);

		const typeMap = {
			Uint8: { fn: DataView.prototype.getUint8, bytes: 1 },
			Uint16: { fn: DataView.prototype.getUint16, bytes: 2 },
			Float32: { fn: DataView.prototype.getFloat32, bytes: 4 }
		}

		let packetPointer = 0
		let i = 0

		let values = {}

		characteristic.structure.forEach(dataType => {
			var dataViewFn = typeMap[dataType].fn.bind(dataReceived);
			var unpackedValue = dataViewFn(packetPointer, true);
			data[columns[i]].push(unpackedValue);
			if (data[columns[i]].length > 64) { data[columns[i]].shift() };
			packetPointer += typeMap[dataType].bytes;
			values[columns[i]] = unpackedValue;
			i++;
		});

		this.emit(characteristic.name, values)
	}

	onDisconnected = event => {
		const device = event.target
		for (const characteristic of this.characteristics) {
			if (!this.enables.includes(characteristic.name)) continue;
		}
		this.emit('disconnected', device.id)
	}
}

module.exports = Connector