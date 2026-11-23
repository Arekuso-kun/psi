"""ASCII event commands and bounded JSON state decoding; no eval or expression execution."""
import json
import re

TOKENS=set('0123456789.+-*/=') | {'C','DEL','SIGN','SET','BACK','MUTE','CAL','VOL+','VOL-'}
def key_packet(token):
    if token==',':token='.'
    if token not in TOKENS:raise ValueError('Tastă invalidă')
    return f'KEY,{token}\n'.encode('ascii')
def nav_packet(direction):
    if direction not in ('L','R','U','D'):raise ValueError('Direcție invalidă')
    return f'NAV,{direction}\n'.encode('ascii')
def volume_packet(volume):
    if type(volume) is not int or not 0<=volume<=100:raise ValueError('Volumul trebuie să fie 0..100')
    return f'VOL,{volume}\n'.encode('ascii')

class Decoder:
    def __init__(self):self.buffer=bytearray();self.discard=False
    def feed(self,data):
        messages=[]
        for byte in data:
            if byte==10:
                if self.buffer and not self.discard:
                    try:
                        obj=json.loads(self.buffer.decode('ascii'))
                        if isinstance(obj,dict) and obj.get('type') in ('hello','state','pong','error'):messages.append(obj)
                    except (ValueError,UnicodeError):pass
                self.buffer.clear();self.discard=False
            elif not self.discard and byte!=13:
                if len(self.buffer)>=1024:self.buffer.clear();self.discard=True
                else:self.buffer.append(byte)
        return messages

def valid_state(s):
    if s.get('type')!='state' or s.get('page') not in ('calculator','settings','calibration'):return False
    if not isinstance(s.get('expression'),str) or not re.fullmatch(r'[0-9.eE+*/\-]{0,63}',s['expression']):return False
    if not isinstance(s.get('result'),str) or not re.fullmatch(r'[0-9.eE+\-]{1,31}',s['result']):return False
    limits={'error':4,'selected':19,'volume':100,'pc':1,'touch':1,'calibrated':1,'cal_step':3,'audio':1}
    return all(type(s.get(key)) is int and 0<=s[key]<=limit for key,limit in limits.items())

ERRORS=['','Expresie incompletă','Împărțire la zero','Număr în afara limitelor','Expresie prea lungă']
BUTTONS=[(10,118,52,32,'C','C'),(66,118,52,32,'DEL','DEL'),(122,118,52,32,'SIGN','±'),(178,118,52,32,'/','÷'),
         (10,155,52,32,'7','7'),(66,155,52,32,'8','8'),(122,155,52,32,'9','9'),(178,155,52,32,'*','×'),
         (10,192,52,32,'4','4'),(66,192,52,32,'5','5'),(122,192,52,32,'6','6'),(178,192,52,32,'-','−'),
         (10,229,52,32,'1','1'),(66,229,52,32,'2','2'),(122,229,52,32,'3','3'),(178,229,52,32,'+','+'),
         (10,266,52,32,'0','0'),(66,266,52,32,'.','.'),(122,266,108,32,'=','='),(190,5,40,25,'SET','SET')]
SETTINGS=[(10,108,106,34,'VOL-','−10'),(124,108,106,34,'VOL+','+10'),(10,158,220,36,'MUTE','Mute / sunet'),
          (10,207,220,36,'CAL','Calibrare touch'),(10,256,220,36,'BACK','Înapoi')]

def hit(page,x,y):
    buttons=BUTTONS if page=='calculator' else SETTINGS if page=='settings' else []
    for bx,by,w,h,token,_ in buttons:
        if bx<=x<bx+w and by<=y<by+h:return token
    return None
