"""Space Invaders PC controller; --demo runs the very same C game/renderer locally."""
from pathlib import Path
import argparse
import array
import ctypes
import time
import tkinter as tk
from tkinter import ttk, messagebox
from protocol import LEFT, RIGHT, FIRE, VOL_UP, VOL_DOWN, PAUSE, START
from protocol import Decoder, keys_packet, volume_packet, valid_state

ROOT = Path(__file__).resolve().parents[1]
STATES = ['title', 'playing', 'paused', 'wave', 'game_over']
LABELS = {'title':'Pregătit', 'playing':'În joc', 'paused':'Pauză', 'wave':'Val eliminat', 'game_over':'Game Over'}
KEYS = {'a':LEFT, 'left':LEFT, 'd':RIGHT, 'right':RIGHT, 'space':FIRE,
        'plus':VOL_UP, 'equal':VOL_UP, 'kp_add':VOL_UP, 'minus':VOL_DOWN,
        'kp_subtract':VOL_DOWN, 'p':PAUSE, 'return':START, 'r':START}

class Simulator:
    def __init__(self):
        path = ROOT / 'build/host/spaceinvaders_sim.dll'
        if not path.exists():
            raise RuntimeError('Lipsește simulatorul. Rulează: python tools/build.py --host')
        self.dll = ctypes.CDLL(str(path))
        self.dll.sim_step.argtypes = [ctypes.c_uint, ctypes.c_uint, ctypes.c_uint]
        self.dll.sim_value.argtypes = [ctypes.c_uint]
        self.dll.sim_value.restype = ctypes.c_uint
        self.dll.sim_volume.argtypes = [ctypes.c_int]
        self.dll.sim_frame.restype = ctypes.POINTER(ctypes.c_uint16)
        self.dll.sim_init()
        self.palette = [bytes((((v >> 11) & 31)*255//31, ((v >> 5) & 63)*255//63, (v & 31)*255//31)) for v in range(65536)]

    def step(self, pc, local, dt):
        self.dll.sim_step(pc, local, dt)

    def status(self):
        values = [self.dll.sim_value(i) for i in range(6)]
        return dict(type='state', state=STATES[values[4]], score=values[0], lives=values[1],
                    level=values[2], volume=values[3], best=values[5], pc=1, audio=0, uart_errors=0)

    def image(self):
        data = ctypes.string_at(self.dll.sim_frame(), 240*320*2)
        pixels = array.array('H', data)
        return b'P6\n240 320\n255\n' + b''.join(self.palette[p] for p in pixels)

class Controller:
    def __init__(self, root, demo=False, port=None):
        self.root = root
        self.demo = Simulator() if demo else None
        self.serial = None
        self.decoder = Decoder()
        self.pressed = {}
        self.local_mask = 0
        self.pulse_mask = 0
        self.pulse_until = 0
        self.desired_volume = None
        self.last_send = self.last_received = self.last_draw = 0
        self.last_tick = time.monotonic()
        self.last_mask = -1
        self.updating_volume = False
        self.photo = None
        self.log_lines = []
        self.build_ui()
        root.bind('<KeyPress>', self.key_down)
        root.bind('<KeyRelease>', self.key_up)
        root.bind('<FocusOut>', lambda event: root.after_idle(self.check_focus))
        root.protocol('WM_DELETE_WINDOW', self.close)
        if not demo:
            self.refresh_ports()
            if port:
                self.port.set(port)
                root.after(100, self.connect)
        self.log('Simulare locală: aceeași logică și imagine C; fără UART sau sunet hardware.' if demo
                 else 'Selectează portul COM. Comunicație 115200 baud, 8N1.')
        self.tick()

    def build_ui(self):
        root = self.root
        root.title('Space Invaders | STM32F429')
        root.configure(bg='#0b1220')
        root.geometry('900x840')
        root.minsize(850,820)
        style=ttk.Style(root)
        style.theme_use('clam')
        style.configure('.', background='#111d30', foreground='#e6edf5', font=('Segoe UI',10))
        style.configure('TFrame', background='#111d30')
        style.configure('TLabel', background='#111d30', foreground='#e6edf5')
        style.configure('Muted.TLabel', foreground='#9aaec5')
        style.configure('Title.TLabel', font=('Segoe UI',22,'bold'), foreground='#74e9ed')
        style.configure('TButton', padding=8, background='#233b56')
        style.map('TButton', background=[('active','#355477')])
        style.configure('TCombobox', fieldbackground='#e9eef4', foreground='#101827')
        wrap=ttk.Frame(root,padding=22);wrap.pack(fill='both',expand=True,padx=16,pady=16)
        ttk.Label(wrap,text='SPACE INVADERS',style='Title.TLabel').pack(anchor='w')
        ttk.Label(wrap,text='SIMULARE LOCALĂ' if self.demo else 'CONTROL PC  •  STM32F429I-DISCO',style='Muted.TLabel').pack(anchor='w',pady=(2,18))
        connection=ttk.Frame(wrap);connection.pack(fill='x',pady=(0,16))
        self.port=tk.StringVar()
        self.port_box=ttk.Combobox(connection,textvariable=self.port,width=20)
        self.port_box.pack(side='left')
        self.refresh_button=ttk.Button(connection,text='Actualizează',command=self.refresh_ports)
        self.refresh_button.pack(side='left',padx=8)
        self.connect_button=ttk.Button(connection,text='Conectează',command=self.connect)
        self.connect_button.pack(side='left')
        self.connection_text=tk.StringVar(value='Simulator' if self.demo else 'Deconectat')
        ttk.Label(connection,textvariable=self.connection_text,style='Muted.TLabel').pack(side='right')
        if self.demo:
            for widget in (self.port_box,self.refresh_button,self.connect_button): widget.configure(state='disabled')
        body=ttk.Frame(wrap);body.pack(fill='both',expand=True)
        left=ttk.Frame(body);left.pack(side='left',fill='both',expand=True,padx=(0,22))
        self.stats={}
        for key,label in [('state','Stare'),('score','Scor'),('best','Record în RAM'),('lives','Vieți'),('level','Val')]:
            row=ttk.Frame(left);row.pack(fill='x',pady=6)
            ttk.Label(row,text=label,style='Muted.TLabel').pack(side='left')
            var=tk.StringVar(value='—');self.stats[key]=var
            ttk.Label(row,textvariable=var,font=('Consolas',14,'bold')).pack(side='right')
        ttk.Separator(left).pack(fill='x',pady=14)
        self.volume_text=tk.StringVar(value='Volum: 20 / 100')
        ttk.Label(left,textvariable=self.volume_text).pack(anchor='w')
        self.volume=tk.DoubleVar(value=20)
        self.slider=ttk.Scale(left,from_=0,to=100,variable=self.volume,command=self.volume_changed)
        self.slider.pack(fill='x',pady=(8,14))
        buttons=ttk.Frame(left);buttons.pack(fill='x')
        ttk.Button(buttons,text='Start / Restart',command=lambda:self.pulse(START)).pack(side='left')
        ttk.Button(buttons,text='Pauză',command=lambda:self.pulse(PAUSE)).pack(side='left',padx=8)
        ttk.Label(left,text='A / D sau săgeți — deplasare\nSpace — foc\n+ / − — volum\nP — pauză    Enter — start\n\nJoystick-ul și butonul USER rămân\ndisponibile permanent pe placă.',style='Muted.TLabel',justify='left').pack(anchor='w',pady=20)
        self.link_detail=tk.StringVar(value='')
        ttk.Label(left,textvariable=self.link_detail,style='Muted.TLabel',wraplength=370).pack(anchor='w')
        right=ttk.Frame(body);right.pack(side='right',anchor='n')
        ttk.Label(right,text='LCD 240 × 320' if self.demo else 'AFIȘARE PE PLACĂ',style='Muted.TLabel').pack(pady=(0,8))
        self.canvas=tk.Canvas(right,width=240,height=320,bg='#080f1c',highlightthickness=2,highlightbackground='#36506c')
        self.canvas.pack()
        self.image_id=self.canvas.create_image(0,0,anchor='nw')
        if not self.demo:
            self.canvas.create_text(120,125,text='Jocul rulează pe STM32\nși se afișează pe LCD.',fill='#9aaec5',font=('Segoe UI',11),justify='center')
            self.canvas.create_text(120,210,text='PC → comenzi\nPlacă → scor, vieți, volum',fill='#74e9ed',font=('Segoe UI',10),justify='center')
        else:
            ttk.Label(right,text='Joystick virtual pentru probă',style='Muted.TLabel').pack(pady=(10,4))
            grid=ttk.Frame(right);grid.pack()
            for label,flag,row,col in [('VOL +',VOL_UP,0,1),('STÂNGA',LEFT,1,0),('PAUZĂ',PAUSE,1,1),('DREAPTA',RIGHT,1,2),('VOL −',VOL_DOWN,2,1),('FOC',FIRE,2,2)]:
                button=ttk.Button(grid,text=label,width=7)
                button.grid(row=row,column=col,padx=1,pady=1)
                button.bind('<ButtonPress-1>',lambda e,f=flag:self.local_down(f))
                button.bind('<ButtonRelease-1>',lambda e,f=flag:self.local_up(f))
                button.bind('<Leave>',lambda e,f=flag:self.local_up(f))
        self.log_text=tk.Text(wrap,height=5,bg='#0b1220',fg='#9aaec5',font=('Consolas',9),relief='flat',state='disabled',wrap='word')
        self.log_text.pack(fill='x',pady=(16,0))

    def log(self, text):
        self.log_lines.append(time.strftime('%H:%M:%S')+'  '+text)
        self.log_lines=self.log_lines[-25:]
        self.log_text.configure(state='normal')
        self.log_text.delete('1.0','end');self.log_text.insert('end','\n'.join(self.log_lines));self.log_text.see('end')
        self.log_text.configure(state='disabled')

    def refresh_ports(self):
        try:
            from serial.tools import list_ports
            ports=[p.device for p in list_ports.comports()]
            self.port_box['values']=ports
            if ports and not self.port.get(): self.port.set(ports[0])
        except ImportError:
            self.connection_text.set('Lipsește pyserial')

    def connect(self):
        if self.serial:
            self.disconnect();return
        try:
            import serial
            port=self.port.get().strip()
            if not port: raise ValueError('Selectează portul COM al plăcii.')
            self.serial=serial.Serial(port,115200,timeout=0,write_timeout=0.1)
            self.serial.reset_input_buffer()
            self.decoder=Decoder();self.pressed.clear();self.last_mask=-1
            self.last_received=time.monotonic()
            self.serial.write(b'K,00\nSTATE?\n')
            self.connect_button.configure(text='Deconectează')
            self.log('Port deschis: '+port+'. Aștept răspunsul firmware-ului.')
        except Exception as exc:
            self.disconnect()
            messagebox.showerror('Conectare',str(exc)+'\n\nPentru pyserial: python -m pip install -r PC/requirements.txt')

    def disconnect(self):
        if self.serial:
            try: self.serial.write(b'K,00\n')
            except Exception: pass
            try: self.serial.close()
            except Exception: pass
        self.serial=None;self.pressed.clear();self.pulse_mask=0;self.desired_volume=None
        self.connection_text.set('Deconectat');self.connect_button.configure(text='Conectează')

    def send(self, packet):
        if self.serial: self.serial.write(packet)

    def key_down(self,event):
        if isinstance(event.widget,(ttk.Entry,ttk.Combobox,ttk.Scale)): return
        flag=KEYS.get(event.keysym.lower())
        if flag:
            self.pressed[event.keycode]=flag
            return 'break'

    def key_up(self,event):
        if event.keycode in self.pressed:
            self.pressed.pop(event.keycode,None)
            return 'break'

    def check_focus(self):
        if self.root.focus_displayof() is None:
            self.pressed.clear();self.local_mask=0;self.pulse_mask=0

    def local_down(self,flag): self.local_mask |= flag
    def local_up(self,flag): self.local_mask &= ~flag
    def pulse(self,flag):
        self.pulse_mask |= flag;self.pulse_until=time.monotonic()+0.1

    def volume_changed(self,value):
        if self.updating_volume: return
        volume=max(0,min(100,round(float(value))))
        self.volume_text.set(f'Volum: {volume} / 100')
        if self.demo: self.demo.dll.sim_volume(volume)
        elif self.serial: self.desired_volume=volume

    def show_status(self,message):
        if not valid_state(message): return
        self.last_received=time.monotonic()
        for key,var in self.stats.items(): var.set(LABELS[message[key]] if key=='state' else str(message[key]))
        if self.desired_volume is None:
            self.updating_volume=True
            self.volume.set(message['volume']);self.updating_volume=False
            self.volume_text.set(f"Volum: {message['volume']} / 100")
        self.link_detail.set(('Simularea nu verifică LCD-ul fizic, UART-ul sau sunetul.' if self.demo
                              else f"Erori UART: {message.get('uart_errors',0)}   |   Audio: {'activ' if message['audio'] else 'dezactivat'}"))
        self.connection_text.set('Simulator' if self.demo else 'Placă conectată')

    def tick(self):
        now=time.monotonic()
        if now>=self.pulse_until: self.pulse_mask=0
        mask=self.pulse_mask
        for flag in self.pressed.values(): mask |= flag
        dt=max(1,min(50,round((now-self.last_tick)*1000)));self.last_tick=now
        try:
            if self.demo:
                self.demo.step(mask,self.local_mask,dt)
                self.show_status(self.demo.status())
                if now-self.last_draw>=0.033:
                    self.photo=tk.PhotoImage(data=self.demo.image(),format='PPM')
                    self.canvas.itemconfigure(self.image_id,image=self.photo);self.last_draw=now
            elif self.serial:
                if mask!=self.last_mask or now-self.last_send>=0.1:
                    self.send(keys_packet(mask));self.last_mask=mask;self.last_send=now
                if self.desired_volume is not None:
                    self.send(volume_packet(self.desired_volume));self.desired_volume=None
                data=self.serial.read(min(4096,self.serial.in_waiting))
                for message in self.decoder.feed(data):
                    if message['type']=='state': self.show_status(message)
                    elif message['type'] in ('hello','error'): self.log(str(message))
                if now-self.last_received>1.5:
                    self.connection_text.set('Fără răspuns de la placă')
        except Exception as exc:
            self.log('Eroare: '+str(exc));self.disconnect()
        self.root.after(16,self.tick)

    def close(self):
        self.disconnect();self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--demo',action='store_true')
    parser.add_argument('--port')
    parser.add_argument('--smoke-test',action='store_true',help=argparse.SUPPRESS)
    args=parser.parse_args()
    root=tk.Tk()
    if args.smoke_test: root.withdraw()
    try: app=Controller(root,args.demo,args.port)
    except Exception as exc:
        if args.smoke_test: raise
        messagebox.showerror('Space Invaders',str(exc));root.destroy();return
    if args.smoke_test: root.after(200,app.close)
    root.mainloop()

if __name__=='__main__': main()
