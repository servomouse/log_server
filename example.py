import asyncio
import websockets
import json

async def send_dictionary():
	uri = "ws://localhost:8765"  # Hardcoded localhost port
	async with websockets.connect(uri) as websocket:
		data = {"path": "logs/test_log.log", "log_string": "Test log string!"}  # Your dictionary
		await websocket.send(json.dumps(data))
		print(f"Sent: {data}")

if __name__ == "__main__":
	asyncio.get_event_loop().run_until_complete(send_dictionary())

