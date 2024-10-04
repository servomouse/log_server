import asyncio
import websockets  # pip install websockets
import json
import signal
import os
import string
import datetime
import threading
import time

ws_port = 8765
buffers = {}
buffer_size = 1 * 1024 * 1024  # 1 MB
buffer_mutex = threading.Lock()

async def handle_connection(websocket, path):
	global buffer_mutex
	async for message in websocket:
		filtered_string = ''.join(filter(lambda x: x in string.printable, message))
		filtered_string = filtered_string.replace("\n", "")
		filtered_string = filtered_string.replace("\t", "")
		# print(f"Received data: {filtered_string}")
		try:
			data = json.loads(filtered_string)
		except Exception as e:
			print(f"{e} ::: {filtered_string}")
			continue
		filename = data['path']
		log_string = data['log_string']
		with buffer_mutex:
			if filename not in buffers:
				buffers[filename] = {}
				buffers[filename]["time"] = 0
				buffers[filename]["data"] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S") + "\n"

			buffers[filename]["data"] += data['log_string'] + "\n"

			if buffers[filename]["time"] == 0:
				buffers[filename]["time"] = time.time()
			if len(buffers[filename]["data"]) >= buffer_size:
				flush_buffer(filename)


def time_thread():
	global buffers, buffer_mutex
	while True:
		with buffer_mutex:
			for filename in buffers:
				if len(buffers[filename]["data"]) == 0:
					continue
				if time.time() - buffers[filename]["time"] > 10:	# flush at least every 10 seconds
					flush_buffer(filename)
					buffers[filename]["time"] = time.time()
		time.sleep(10)


def flush_buffer(filename):
	if filename in buffers and buffers[filename]["data"]:
		directory = os.path.dirname(filename)
		if not os.path.exists(directory):
			os.makedirs(directory)
		with open(filename, 'a') as f:
			f.write(buffers[filename]["data"])
			buffers[filename]["data"] = ""

def flush_all_buffers():
	global buffer_mutex
	with buffer_mutex:
		for filename in buffers:
			flush_buffer(filename)

def signal_handler(sig, frame):
	print('Flushing all buffers and shutting down...')
	flush_all_buffers()
	os._exit(0)

signal.signal(signal.SIGINT, signal_handler)

async def main():
	thread = threading.Thread(target=time_thread)
	thread.start()
	async with websockets.serve(handle_connection, "localhost", ws_port):
		await asyncio.Future()  # Run forever

if __name__ == "__main__":
	asyncio.run(main())
