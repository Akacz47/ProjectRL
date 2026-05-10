import socket
import struct

HOST = '127.0.0.1'
PORT = 9000

# Format: 6 floatów (f), mały endian (<)
# fffffff = 6 * 4 bajty = 28 bajtów
STATE_FORMAT = "<ffffff" 
ACTION_FORMAT = "<ff"

def receive_game_state(sock):
    data, addr = sock.recvfrom(1024)
    # Rozpakuj bajty bezpośrednio do listy wartości
    unpacked = struct.unpack(STATE_FORMAT, data)
    return {
        "error_x": unpacked[0],
        "error_y": unpacked[1],
        "v_x": unpacked[2],
        "v_y": unpacked[3],
        "bullet_distance": unpacked[4],
        "target_distance": unpacked[5]
    }

def send_action(sock, rotateLR, rotateUD):
    data = struct.pack(ACTION_FORMAT, float(rotateLR), float(rotateUD))
    sock.sendto(data, (HOST, PORT))