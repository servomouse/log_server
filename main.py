import asyncio
import websockets
import json
import signal
import os

buffers = {}
buffer_size = 1 * 1024 * 1024  # 1 MB

async def handle_connection(websocket, path):
	async for message in websocket:
		data = json.loads(message)
		filename = data['filename']
		log_string = data['log_string']

	if filename not in buffers:
		buffers[filename] = bytearray()

	buffers[filename].extend(log_string.encode('utf-8'))

	if len(buffers[filename]) >= buffer_size:
		flush_buffer(filename)
    
def flush_buffer(filename):
	if filename in buffers and buffers[filename]:
		with open(filename, 'ab') as f:
			f.write(buffers[filename])
			buffers[filename] = bytearray()

def flush_all_buffers():
	for filename in buffers:
		flush_buffer(filename)

def signal_handler(sig, frame):
	print('Flushing all buffers and shutting down...')
	flush_all_buffers()
	os._exit(0)

signal.signal(signal.SIGINT, signal_handler)

async def main():
	async with websockets.serve(handle_connection, "localhost", 8765):
	await asyncio.Future()  # Run forever

if __name__ == "__main__":
	asyncio.run(main())
