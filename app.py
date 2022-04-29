from os import stat
from flask import Flask, render_template, request
from flask_sqlalchemy import SQLAlchemy
import paho.mqtt.client as mqtt
import random
import time

broker = 'broker.emqx.io'
port = 1883
# topic = "test"
client_id = f'python-mqtt-{random.randint(0, 1000)}'
client_id2 = f'python-mqtt-{random.randint(0, 1000)}'

username = 'emqx'
password = 'public'

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = "mysql://root:root@127.0.0.1:8889/window62"
app.config["TEMPLATES_AUTO_RELOAD"] = True
app.config['SERVER_NAME'] = "localhost:5555"

db = SQLAlchemy(app)

testData = ""
tempData = "disconnected"
lightData = "disconnected"
humidityData = "disconnected"
dustData = "disconnected"
pm25Data = "disconnected"

class Object(db.Model):
    obj_id = db.Column(db.Integer, primary_key=True)
    obj_name = db.Column(db.String(20))
    obj_description = db.Column(db.String(100), nullable=True)
    obj_status = db.Column(db.String(20))
    room_id = db.Column(db.Integer, nullable=False)

    def __repr__(self):
        return '<Object %r>' % self.obj_name

class Sensor(db.Model):
    sensor_id = db.Column(db.Integer, primary_key=True)
    sensor_name = db.Column(db.String(20))
    sensor_description = db.Column(db.String(100), nullable=True)
    obj_id = db.Column(db.Integer)

    def __repr__(self):
        return '<Object %r>' % self.obj_id
    

class temp_reading(db.Model):
    temp_id = db.Column(db.Integer, primary_key=True)
    temp_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)
    def __repr__(self):
        return '<temp_reading %r>' % self.temp_id

class ldr_reading(db.Model):
    ldr_id = db.Column(db.Integer, primary_key=True)
    ldr_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)
    def __repr__(self):
        return '<ldr_reading %r>' % self.ldr_id

class humidity_reading(db.Model):
    humidity_id = db.Column(db.Integer, primary_key=True)
    humidity_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)
    def __repr__(self):
        return '<ldr_reading %r>' % self.humidity_id

class pm_reading(db.Model):
    pm_id = db.Column(db.Integer, primary_key=True)
    pm_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)
    def __repr__(self):
        return '<ldr_reading %r>' % self.pm_id      

class transaction_obj(db.Model):
    tran_id = db.Column(db.Integer, primary_key=True)
    obj_id = db.Column(db.Integer)
    obj_status = obj_status = db.Column(db.String(20))
    def __repr__(self):
        return '<ldr_reading %r>' % self.tran_id   

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe("WindowLight")
    client.subscribe("WindowTemp")
    client.subscribe("WindowHumidity")
    client.subscribe("WindowPM25")
    client.subscribe("WindowStatus")
    client.subscribe("CurtainStatus")

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload.decode()) + '\n')
    message = str(msg.payload.decode())
    if msg.topic == "WindowTemp":
        val = float(msg.payload.decode())
        temp = temp_reading(sensor_id=1, temp_input=val)
        insert(temp)
        global tempData
        tempData = message
    elif msg.topic == "WindowLight":
        val = float(msg.payload.decode())
        light = ldr_reading(sensor_id=2, ldr_input=val)
        insert(light)
        # print("Light", msg.payload.decode())
        global lightData
        lightData = message
    elif msg.topic == "WindowHumidity":
        val = float(msg.payload.decode())
        humid = humidity_reading(sensor_id=3, humidity_input=val)
        insert(humid)
        print("humid", msg.payload.decode())
        global humidityData
        humidityData = message
    elif msg.topic == "WindowPM25":
        val = float(msg.payload.decode())
        pm = pm_reading(sensor_id=4, pm_input=val)
        insert(pm)
        print("PM25", msg.payload.decode())
        global pm25Data
        pm25Data = message
    elif msg.topic == "WindowStatus":
        print("window status", msg.payload.decode())
        if message == "close" or message == "open":
            status(1, message)
    elif msg.topic == "CurtainStatus":
        print("window status", msg.payload.decode())
        if message == "close" or message == "open":
            status(2, message)
    # global testData 
    # testData = str(msg.payload.decode())

def on_disconnect(client, userdata, rc):
    print("Closing data file...")

client = mqtt.Client(client_id=client_id)
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect
client.username_pw_set(username, password)

client.connect(broker, port, 60)
client.loop_start()

client2 = mqtt.Client(client_id=client_id2)
client2.on_connect = on_connect
# client2.on_message = on_message
# client2.on_disconnect = on_disconnect
client2.username_pw_set(username, password)

client2.connect(broker, port, 60)
client2.loop_start()

def insert(value):
    db.session.add(value)
    db.session.commit()


@app.route('/')
def index():
    if request.method == "GET":
        objects = Object.query.all()
        sensors = []
        for obj in objects:
            sensor = Sensor.query.filter_by(obj_id=obj.obj_id).all()
            sensors.append(sensor)
       
        return render_template("object2.html", objects=objects, sensors=sensors, lightData=lightData, tempData=tempData, pm25Data=pm25Data, humidityData=humidityData)

@app.route('/object/<int:id>')
def sensors(id):
    if request.method == "GET":
        # sensors = Sensor.query.join(Object, Sensor.obj_id==Object.obj_id)
        sensors = Sensor.query.filter_by(obj_id=id).all()
        print(sensors)
        return render_template("sensor.html", sensors=sensors, lightData=lightData, tempData=tempData, testData="")

@app.route("/object/<int:id>/changeStatus")
def changeStatus(id):
    object = Object.query.filter_by(obj_id=id).first()
    print(object.obj_status)
    status = "close"
    if object.obj_status == "close":
        status = "open"
    topic = object.obj_name+"Cmd"
    result = client2.publish(topic, status)
    print("result pub:", result, topic, status)
    object.obj_status = status
    transaction = transaction_obj(obj_id=object.obj_id, obj_status=status)
    db.session.add(transaction)
    db.session.commit()
    return render_template("status.html")

@app.route("/object/<int:id>/status/<status>")
def status(id, status):
    object = Object.query.filter_by(obj_id=id).first()
    with app.app_context():
        if object.obj_status != status:
            topic = object.obj_name+"Cmd"
            result = client2.publish(topic, status)
            print("result pub:", result, topic, status)
            object.obj_status = status
            transaction = transaction_obj(obj_id=object.obj_id, obj_status=status)
            db.session.add(transaction)
            db.session.commit()
        return render_template("status.html")
        

if __name__ == '__main__':
    client = mqtt.Client()
    app.run( debug=True, host='localhost', port=5555)