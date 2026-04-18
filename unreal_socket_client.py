# unreal_socket_client.py
import socket
import json

HOST = '127.0.0.1'  # IP Unreal Engine
PORT = 9000         # port musi pasować do UE5 (wczesniej bylo 8000)

def receive_game_state(sock):
    data, _ = sock.recvfrom(4096)
    return json.loads(data.decode())

def send_action(sock, action_dict):
    msg = json.dumps(action_dict)
    sock.sendto(msg.encode('utf-8'), (HOST, PORT))


import struct

# Format: 7 floatów (f), mały endian (<)
# fffffff = 7 * 4 bajty = 28 bajtów
STATE_FORMAT = "<fffffff" 
ACTION_FORMAT = "<ff"

def receive_game_state2(sock):
    data, addr = sock.recvfrom(1024)
    # Rozpakuj bajty bezpośrednio do listy wartości
    unpacked = struct.unpack(STATE_FORMAT, data)
    return {
        "error_x": unpacked[0],
        "error_y": unpacked[1],
        "v_x": unpacked[2],
        "v_y": unpacked[3],
        "bullet_distance": unpacked[4],
        "target_distance": unpacked[5],
        "shoot_cooldown": unpacked[6]
    }

def send_action2(sock, rotateLR, rotateUD):
    # Pakuj dwa floaty do bajtów
    data = struct.pack(ACTION_FORMAT, float(rotateLR), float(rotateUD))
    sock.sendto(data, (HOST, PORT))