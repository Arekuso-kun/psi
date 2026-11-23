"""Small, bounded UART protocol. This module has no third-party dependencies."""
import json

LEFT, RIGHT, FIRE, VOL_UP, VOL_DOWN, PAUSE, START = (1, 2, 4, 8, 16, 32, 64)

def keys_packet(mask):
    if not isinstance(mask, int) or not 0 <= mask <= 0x7f:
        raise ValueError('Invalid key mask')
    return f'K,{mask:02X}\n'.encode('ascii')

def volume_packet(volume):
    if not isinstance(volume, int) or not 0 <= volume <= 100:
        raise ValueError('Volume must be 0..100')
    return f'VOL,{volume}\n'.encode('ascii')

class Decoder:
    def __init__(self):
        self.buffer = bytearray()
        self.discard = False

    def feed(self, data):
        messages = []
        for byte in data:
            if byte == 10:
                if not self.discard and self.buffer:
                    try:
                        item = json.loads(self.buffer.decode('ascii'))
                        if isinstance(item, dict) and item.get('type') in ('state', 'hello', 'pong', 'error'):
                            messages.append(item)
                    except (ValueError, UnicodeError):
                        pass
                self.buffer.clear()
                self.discard = False
            elif not self.discard:
                if len(self.buffer) >= 1024:
                    self.buffer.clear()
                    self.discard = True
                elif byte != 13:
                    self.buffer.append(byte)
        return messages

def valid_state(message):
    return (message.get('type') == 'state'
            and message.get('state') in ('title', 'playing', 'paused', 'wave', 'game_over')
            and all(type(message.get(k)) is int and 0 <= message[k] <= limit
                    for k, limit in [('score', 2**32-1), ('best', 2**32-1), ('lives', 3),
                                     ('level', 99), ('volume', 100), ('pc', 1), ('audio', 1)]))
