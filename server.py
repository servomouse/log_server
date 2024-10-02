import asyncio
import websockets  # pip install websockets
import json
import signal
import os
import string

ws_port = 8765
buffers = {}
buffer_size = 1 * 1024 * 1024  # 1 MB

async def handle_connection(websocket, path):
	async for message in websocket:
		filtered_string = ''.join(filter(lambda x: x in string.printable, message))
		# print(f"Received data: {filtered_string}")
		data = json.loads(filtered_string)
		filename = data['path']
		log_string = data['log_string']
		if filename not in buffers:
			buffers[filename] = ""

		buffers[filename] += data['log_string'] + "\n"

		if len(buffers[filename]) >= buffer_size:
			flush_buffer(filename)
    
def flush_buffer(filename):
	if filename in buffers and buffers[filename]:
		directory = os.path.dirname(filename)
		if not os.path.exists(directory):
			os.makedirs(directory)
		with open(filename, 'a') as f:
			f.write(buffers[filename])
			buffers[filename] = ""

def flush_all_buffers():
	for filename in buffers:
		flush_buffer(filename)

def signal_handler(sig, frame):
	print('Flushing all buffers and shutting down...')
	flush_all_buffers()
	os._exit(0)

signal.signal(signal.SIGINT, signal_handler)

async def main():
	async with websockets.serve(handle_connection, "localhost", ws_port):
		await asyncio.Future()  # Run forever

if __name__ == "__main__":
	asyncio.run(main())
