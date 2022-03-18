from tokenize import String
from kivymd.app import MDApp
from kivymd.uix.screen import MDScreen
from kivymd.uix.toolbar import MDToolbar
from kivymd.uix.label import MDLabel
from kivymd.uix.button import *
from kivy.uix.image import Image
import paho.mqtt.client as mqtt


class AutomaticFanController(MDApp):
    def flip(self):
        if self.state == 0:
            self.state = 1
            self.toolbar.title = "Automatic"
            self.client.publish("/setMode", "1")
        else:
            self.state = 0
            self.toolbar.title = "Manual"
            self.client.publish("/setMode", "0")

    def changeSpeed(self, args):

        if self.state == 0:
            self.fanStatus.text = "FanStatus : " + args
            if (args == "1"):
                self.client.publish("/setSpeed", "70")
            if (args == "2"):
                self.client.publish("/setSpeed", "125")
            if (args == "3"):
                self.client.publish("/setSpeed", "255")
            if (args == "0"):
                self.client.publish("/setSpeed", "0")
            
        elif self.state == 1:
            print("Auto")

    def build(self):
        self.temp_max = 5.0
        self.temp_min = -4.0
        self._mqttConnect()
        self.state = 0
        self.theme_cls.primary_palette = "LightGreen"
        screen = MDScreen()

        self.toolbar = MDToolbar(title='Manual')
        self.toolbar.pos_hint = {'top': 1}
        self.toolbar.right_action_items = [
            ['rotate-3d-variant', lambda x: self.flip()],

        ]

        screen.add_widget(self.toolbar)

        # logo
        screen.add_widget(Image(
            source='images/temperature.jpg',
            pos_hint={'center_x': 0.3, 'center_y': 0.65},
            size_hint=(0.35, 0.35)
        ))
        screen.add_widget(Image(
            source='images/fan.jpg',
            pos_hint={'center_x': 0.7, 'center_y': 0.65},
            size_hint=(0.35, 0.35)
        ))

        self.tempStatus = MDLabel(text="Temperature is: ", halign="center", pos_hint={
            'center_x': 0.3, 'center_y': 0.5}, size_hint=(0.8, 1), font_size=22)
        self.fanStatus = MDLabel(text="FanStatus : ", halign="center", pos_hint={
            'center_x': 0.7, 'center_y': 0.5}, size_hint=(0.8, 1), font_size=22)
        self.speed1 = MDFillRoundFlatButton(text="Speed1", pos_hint={
            'center_x': 0.5, 'center_y': 0.4}, font_size=22, on_press=lambda x: self.changeSpeed('1'))
        self.speed2 = MDFillRoundFlatButton(text="Speed2", pos_hint={
            'center_x': 0.5, 'center_y': 0.3}, font_size=22, on_press=lambda x: self.changeSpeed('2'))
        self.speed3 = MDFillRoundFlatButton(text="Speed3", pos_hint={
            'center_x': 0.5, 'center_y': 0.2}, font_size=22, on_press=lambda x: self.changeSpeed('3'))
        self.turnOff = MDFillRoundFlatButton(text="Turn Off", pos_hint={
            'center_x': 0.5, 'center_y': 0.1}, font_size=22, on_press=lambda x: self.changeSpeed('0'))
        screen.add_widget(self.tempStatus)
        screen.add_widget(self.fanStatus)
        screen.add_widget(self.speed1)
        screen.add_widget(self.speed2)
        screen.add_widget(self.speed3)
        screen.add_widget(self.turnOff)

        return screen

    def _mqttConnect(self):
        self.doit = False
        self.client = mqtt.Client(
            client_id="server", userdata=None, protocol=mqtt.MQTTv311, transport="tcp")
        self.client.on_message = self.mqttcallback
        self.client.username_pw_set(
            username="mqttuser", password="mqttpass")
        self.client.connect('mqtthost',
                            port=16232, keepalive=60)
        self.client.loop_start()
        self.client.subscribe([("temp", 0), ("speed", 0)])
        print("sddsd")
        return

    def mqttcallback(self, client, userdata, message):
        data = message.payload.decode("utf-8").split(':')
        if message.topic=="temp":
            self.tempStatus.text ="Temperature is: "+data[0]
        
        
        if self.doit:
            self.temp_max = self.temp_max+1
        else:
            # skip the first update
            self.doit = True
        return



    def kvcallback(self):
        print("kv callback called")
        return

if __name__ == '__main__':
    AutomaticFanController().run()
