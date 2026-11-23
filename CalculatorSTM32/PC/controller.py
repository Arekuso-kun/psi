"""PC keyboard/UART client. --demo uses the same C calculator, input and LCD renderer."""
from pathlib import Path
import argparse
import array
import ctypes
import json
import time
import tkinter as tk
from tkinter import ttk,messagebox
from protocol import Decoder,valid_state,key_packet,nav_packet,volume_packet,ERRORS,BUTTONS,SETTINGS,hit

ROOT=Path(__file__).resolve().parents[1]
KEYMAP={'period':'.','comma':'.','decimal':'.','add':'+','plus':'+','subtract':'-','minus':'-',
        'multiply':'*','asterisk':'*','divide':'/','slash':'/','equal':'=','return':'=',
        'backspace':'DEL','delete':'DEL','escape':'BACK','c':'C','v':'SET','f9':'SIGN'}
NAV={'left':'L','right':'R','up':'U','down':'D'}

class Simulator:
    def __init__(self):
        path=ROOT/'build/host/calculator_sim.dll'
        if not path.exists():raise RuntimeError('Lipsește simulatorul. Rulează: python tools/build.py --host')
        self.dll=ctypes.CDLL(str(path))
        self.dll.sim_send.argtypes=[ctypes.c_char_p]
        self.dll.sim_touch.argtypes=[ctypes.c_int,ctypes.c_int,ctypes.c_int]
        self.dll.sim_step.argtypes=[ctypes.c_uint,ctypes.c_uint]
        self.dll.sim_frame.restype=ctypes.POINTER(ctypes.c_uint16)
        self.dll.sim_status.restype=ctypes.c_char_p
        self.dll.sim_init()
        self.palette=[bytes((((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31)) for v in range(65536)]
    def send(self,packet):self.dll.sim_send(packet)
    def touch(self,x,y,down):self.dll.sim_touch(x,y,down)
    def step(self,local,dt):self.dll.sim_step(local,dt)
    def status(self):return json.loads(self.dll.sim_status().decode('ascii'))
    def image(self):
        pixels=array.array('H',ctypes.string_at(self.dll.sim_frame(),240*320*2))
        return b'P6\n240 320\n255\n'+b''.join(self.palette[v] for v in pixels)

class Controller:
    def __init__(self,root,demo=False,port=None):
        self.root=root;self.demo=Simulator() if demo else None
        self.serial=None;self.decoder=Decoder();self.held=set();self.local=0;self.touch_held=False
        self.state=None;self.photo=None;self.log_lines=[];self.pending_volume=None;self.updating_volume=False
        self.scale=1.5
        self.last_tick=time.monotonic();self.last_ping=self.last_draw=self.last_received=0
        self.build_ui()
        root.bind('<KeyPress>',self.key_down);root.bind('<KeyRelease>',self.key_up)
        root.bind('<FocusOut>',lambda event:root.after_idle(self.focus_lost))
        root.protocol('WM_DELETE_WINDOW',self.close)
        if demo:
            self.log('Simulare C: calcule, touch virtual și joystick. Sunetul și perifericele reale nu sunt emulate.')
        else:
            self.refresh_ports();self.log('Selectează portul COM. Placa execută toate calculele.')
            if port:self.port.set(port);root.after(100,self.connect)
        self.tick()

    def build_ui(self):
        r=self.root;r.title('Calculator | STM32F429');r.geometry('900x840');r.minsize(850,830);r.configure(bg='#0b1220')
        style=ttk.Style(r);style.theme_use('clam')
        style.configure('.',background='#111d30',foreground='#edf2fa',font=('Segoe UI',10))
        style.configure('TFrame',background='#111d30');style.configure('TLabel',background='#111d30',foreground='#edf2fa')
        style.configure('Muted.TLabel',foreground='#9aaec5')
        style.configure('Title.TLabel',font=('Segoe UI',22,'bold'),foreground='#65ddde')
        style.configure('TButton',background='#254058',padding=7)
        style.map('TButton',background=[('active','#385c7b')])
        style.configure('TCombobox',foreground='#101827',fieldbackground='#e8eef5')
        wrap=ttk.Frame(r,padding=20);wrap.pack(fill='both',expand=True,padx=14,pady=14)
        ttk.Label(wrap,text='CALCULATOR STM32',style='Title.TLabel').pack(anchor='w')
        ttk.Label(wrap,text='SIMULARE LOCALĂ' if self.demo else 'TOUCH  /  JOYSTICK  /  TASTATURĂ PC',style='Muted.TLabel').pack(anchor='w',pady=(0,10))
        conn=ttk.Frame(wrap);conn.pack(fill='x',pady=(0,14))
        self.port=tk.StringVar();self.port_box=ttk.Combobox(conn,textvariable=self.port,width=16);self.port_box.pack(side='left')
        self.refresh_button=ttk.Button(conn,text='Actualizează',command=self.refresh_ports);self.refresh_button.pack(side='left',padx=7)
        self.connect_button=ttk.Button(conn,text='Conectează',command=self.connect);self.connect_button.pack(side='left')
        self.link=tk.StringVar(value='Simulator' if self.demo else 'Deconectat')
        ttk.Label(conn,textvariable=self.link,style='Muted.TLabel').pack(side='right')
        if self.demo:
            for w in (self.port_box,self.refresh_button,self.connect_button):w.configure(state='disabled')
        body=ttk.Frame(wrap);body.pack(fill='both',expand=True)
        left=ttk.Frame(body);left.pack(side='left',fill='both',expand=True,padx=(0,18))
        self.expression=tk.StringVar(value='');self.result=tk.StringVar(value='0');self.error=tk.StringVar(value='')
        ttk.Label(left,text='Expresie primită de la placă' if not self.demo else 'Expresie',style='Muted.TLabel').pack(anchor='w')
        ttk.Label(left,textvariable=self.expression,font=('Consolas',14),wraplength=300).pack(anchor='w',pady=(5,10))
        ttk.Label(left,textvariable=self.result,font=('Consolas',27,'bold')).pack(anchor='w')
        ttk.Label(left,textvariable=self.error,foreground='#ff8a91',wraplength=300).pack(anchor='w',pady=(5,10))
        self.volume=tk.DoubleVar(value=20);self.volume_label=tk.StringVar(value='Volum: 20 / 100')
        ttk.Label(left,textvariable=self.volume_label).pack(anchor='w',pady=(10,4))
        ttk.Scale(left,from_=0,to=100,variable=self.volume,command=self.volume_changed).pack(fill='x')
        options=ttk.Frame(left);options.pack(fill='x',pady=10)
        ttk.Button(options,text='Setări',command=lambda:self.send_key('SET')).pack(side='left')
        ttk.Button(options,text='Înapoi / C',command=lambda:self.send_key('BACK')).pack(side='left',padx=6)
        ttk.Label(left,text='Cifre, + − * / · Enter pentru rezultat\nBackspace șterge · Esc înapoi / C\nF9 schimbă semnul · V setări\nSăgeți navighează · Space confirmă\nÎn Setări: + / − schimbă volumul.',style='Muted.TLabel',justify='left').pack(anchor='w',pady=12)
        self.details=tk.StringVar(value='')
        ttk.Label(left,textvariable=self.details,wraplength=300,style='Muted.TLabel').pack(anchor='w',pady=8)
        if self.demo:
            ttk.Label(left,text='Joystick virtual',style='Muted.TLabel').pack(anchor='w',pady=(8,4))
            pad=ttk.Frame(left);pad.pack(anchor='w')
            for label,flag,row,col in [('SUS',4,0,1),('STÂNGA',1,1,0),('OK',16,1,1),('DREAPTA',2,1,2),('JOS',8,2,1),('USER',32,2,2)]:
                b=ttk.Button(pad,text=label,width=7);b.grid(row=row,column=col,padx=1,pady=1)
                b.bind('<ButtonPress-1>',lambda e,f=flag:self.local_down(f))
                b.bind('<ButtonRelease-1>',lambda e,f=flag:self.local_up(f))
                b.bind('<Leave>',lambda e,f=flag:self.local_up(f))
        screen=ttk.Frame(body);screen.pack(side='right',anchor='n')
        ttk.Label(screen,text='LCD C + TOUCH VIRTUAL' if self.demo else 'COMENZI PC / STARE PRIMITĂ',style='Muted.TLabel').pack(pady=(0,6))
        self.canvas=tk.Canvas(screen,width=360,height=480,bg='#080f1b',highlightthickness=2,highlightbackground='#35506b')
        self.canvas.pack();self.image_id=self.canvas.create_image(0,0,anchor='nw')
        self.canvas.bind('<ButtonPress-1>',self.mouse_down);self.canvas.bind('<ButtonRelease-1>',self.mouse_up)
        self.canvas.bind('<Leave>',self.mouse_up)
        self.logbox=tk.Text(wrap,height=3,bg='#0b1220',fg='#99adc5',font=('Consolas',9),state='disabled',relief='flat',wrap='word')
        self.logbox.pack(fill='x',pady=(10,0))
        if not self.demo:self.draw_remote()

    def log(self,text):
        self.log_lines=(self.log_lines+[time.strftime('%H:%M:%S')+'  '+text])[-20:]
        self.logbox.configure(state='normal');self.logbox.delete('1.0','end');self.logbox.insert('end','\n'.join(self.log_lines));self.logbox.see('end');self.logbox.configure(state='disabled')

    def refresh_ports(self):
        try:
            from serial.tools import list_ports
            ports=[p.device for p in list_ports.comports()];self.port_box['values']=ports
            if ports and not self.port.get():self.port.set(ports[0])
        except ImportError:self.link.set('Lipsește pyserial')

    def connect(self):
        if self.serial:self.disconnect();return
        try:
            import serial
            if not self.port.get().strip():raise ValueError('Selectează portul COM.')
            self.serial=serial.Serial(self.port.get().strip(),115200,timeout=0,write_timeout=0.1)
            self.serial.reset_input_buffer();self.decoder=Decoder();self.last_received=time.monotonic()
            self.serial.write(b'PING\nSTATE?\n');self.connect_button.configure(text='Deconectează')
            self.link.set('Aștept răspunsul plăcii');self.log('Port deschis: '+self.port.get())
        except Exception as error:
            self.disconnect();messagebox.showerror('Conectare',str(error)+'\n\nInstalare transport serial: python -m pip install -r PC/requirements.txt')

    def disconnect(self):
        if self.serial:
            try:self.serial.close()
            except Exception:pass
        self.serial=None;self.held.clear();self.pending_volume=None
        self.connect_button.configure(text='Conectează');self.link.set('Deconectat')

    def send(self,packet):
        try:
            if self.demo:self.demo.send(packet)
            elif self.serial:self.serial.write(packet)
        except Exception as error:self.log(str(error));self.disconnect()

    def send_key(self,token):self.send(key_packet(token))
    def key_down(self,e):
        if isinstance(e.widget,(ttk.Entry,ttk.Combobox,ttk.Scale)):return
        if e.keycode in self.held:return 'break'
        name=e.keysym.lower().removeprefix('kp_')
        if name in NAV:packet=nav_packet(NAV[name])
        elif name=='space':packet=b'OK\n'
        elif len(name)==1 and '0'<=name<='9':packet=key_packet(name)
        elif name in KEYMAP:packet=key_packet(KEYMAP[name])
        else:return
        self.held.add(e.keycode);self.send(packet);return 'break'

    def key_up(self,e):
        if e.keycode in self.held:self.held.discard(e.keycode);return 'break'

    def focus_lost(self):
        if self.root.focus_displayof() is None:
            self.held.clear();self.local=0;self.mouse_up(None)

    def local_down(self,flag):self.local|=flag
    def local_up(self,flag):self.local&=~flag

    def mouse_down(self,e):
        self.canvas.focus_set();x=max(0,min(239,int(e.x/self.scale)));y=max(0,min(319,int(e.y/self.scale)))
        if self.demo:self.touch_held=True;self.demo.touch(x,y,1)
        elif self.state:
            token=hit(self.state['page'],x,y)
            if token:self.send_key(token)

    def mouse_up(self,e):
        if self.demo and self.touch_held:self.demo.touch(0,0,0)
        self.touch_held=False

    def volume_changed(self,value):
        if self.updating_volume:return
        volume=max(0,min(100,round(float(value))));self.volume_label.set(f'Volum: {volume} / 100');self.pending_volume=volume

    def update_state(self,s):
        if not valid_state(s):return
        self.state=s;self.last_received=time.monotonic()
        self.expression.set(s['expression']);self.result.set(s['result']);self.error.set(ERRORS[s['error']])
        if self.pending_volume is None:
            self.updating_volume=True;self.volume.set(s['volume']);self.updating_volume=False
            self.volume_label.set(f"Volum: {s['volume']} / 100")
        self.link.set('Simulator' if self.demo else 'Placă conectată')
        self.details.set('Simulator fără sunet: calculele și desenarea folosesc același cod C ca placa.' if self.demo else
                         f"Touch: {'disponibil' if s['touch'] else 'indisponibil'}\nCalibrare: {'realizată' if s['calibrated'] else 'implicită'}\nAudio: {'activ' if s['audio'] else 'dezactivat'}\nErori UART: {s.get('uart_errors',0)}")
        if not self.demo:self.draw_remote()

    def draw_remote(self):
        c=self.canvas;c.delete('all');s=self.state
        if not s:
            c.create_text(180,220,text='Conectează placa pentru\ncomenzi și rezultate.',fill='#9aaec5',font=('Segoe UI',13),justify='center');return
        def rect(x,y,w,h,color,outline=''):
            c.create_rectangle(x*self.scale,y*self.scale,(x+w)*self.scale,(y+h)*self.scale,fill=color,outline=outline or color,width=2)
        def text(x,y,string,size=15,color='#e8eff5',anchor='nw'):
            c.create_text(x*self.scale,y*self.scale,text=string,font=('Consolas',max(9,round(size*self.scale/2))),fill=color,anchor=anchor)
        if s['page']=='calculator':
            text(10,10,'CALCULATOR',14,'#65ddde');rect(10,37,220,68,'#15263a')
            text(16,43,s['expression'][-60:-30] if len(s['expression'])>30 else '',11,'#9aaec5')
            text(16,57,s['expression'][-30:],11,'#9aaec5')
            text(224,82,ERRORS[s['error']] if s['error'] else s['result'],13 if s['error'] else 23,'#ff9299' if s['error'] else '#edf3fa','ne')
            buttons=BUTTONS
        elif s['page']=='settings':
            text(10,12,'SETĂRI',23);text(10,48,f"Volum: {s['volume']}",22,'#65ddde');rect(10,82,220,5,'#254058');rect(10,82,220*s['volume']/100,5,'#65ddde');buttons=SETTINGS
        else:
            text(120,25,'CALIBRARE TOUCH',20,'#65ddde','n')
            text(120,125,f"Punct {s['cal_step']+1} / 3",21,anchor='n')
            text(120,175,'Atinge crucea pe LCD-ul\nplăcii și ridică degetul.\n\nEscape — anulare',13,'#9aaec5','n');buttons=[]
        for i,(x,y,w,h,token,label) in enumerate(buttons):
            rect(x,y,w,h,'#65ddde' if token=='=' else '#23364c','#65ddde' if i==s['selected'] else '#23364c')
            text(x+w/2,y+h/2,label,22 if len(label)<4 else 12,'#101b2b' if token=='=' else '#edf3fa','center')
        text(10,307,f"VOL {s['volume']}",10,'#9aaec5');text(150,307,'STARE PRIMITĂ',10,'#9aaec5')

    def tick(self):
        now=time.monotonic();dt=max(1,min(100,round((now-self.last_tick)*1000)));self.last_tick=now
        try:
            if self.pending_volume is not None:self.send(volume_packet(self.pending_volume));self.pending_volume=None
            if now-self.last_ping>=1 and (self.demo or self.serial):self.send(b'PING\n');self.last_ping=now
            if self.demo:
                self.demo.step(self.local,dt);self.update_state(self.demo.status())
                if now-self.last_draw>=0.033:
                    self.photo=tk.PhotoImage(data=self.demo.image(),format='PPM').zoom(3,3).subsample(2,2)
                    self.canvas.itemconfigure(self.image_id,image=self.photo);self.last_draw=now
            elif self.serial:
                for message in self.decoder.feed(self.serial.read(min(4096,self.serial.in_waiting))):
                    if message['type']=='state':self.update_state(message)
                    elif message['type'] in ('hello','error'):self.log(str(message))
                if now-self.last_received>2:self.link.set('Fără răspuns de la placă')
        except Exception as error:
            self.log('Eroare: '+str(error))
            if self.serial:self.disconnect()
        self.root.after(16,self.tick)

    def close(self):self.disconnect();self.root.destroy()

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--demo',action='store_true');parser.add_argument('--port');parser.add_argument('--smoke-test',action='store_true',help=argparse.SUPPRESS)
    args=parser.parse_args();root=tk.Tk()
    if args.smoke_test:root.withdraw()
    try:app=Controller(root,args.demo,args.port)
    except Exception as error:
        if args.smoke_test:raise
        messagebox.showerror('CalculatorSTM32',str(error));root.destroy();return
    if args.smoke_test:root.after(250,app.close)
    root.mainloop()

if __name__=='__main__':main()
