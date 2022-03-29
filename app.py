from flask import Flask, render_template, request
from flask_sqlalchemy import SQLAlchemy
import paho.mqtt.client as mqtt
import random
import time
from apscheduler.schedulers.background import BackgroundScheduler
from flask_socketio import SocketIO, emit

broker = 'broker.emqx.io'
port = 1883
topic = "test"
client_id = f'python-mqtt-{random.randint(0, 1000)}'
username = 'emqx'
password = 'public'

testData = ""

def on_connect(client, userdata, flags, rc):
    print("Connected with result code " + str(rc))
    client.subscribe(topic)

def on_message(client, userdata, msg):
    print(msg.topic+" "+str(msg.payload.decode()) + '\n')
    global testData 
    testData = str(msg.payload.decode())
    socketio.emit('update',testData, broadcast=True)

#schedule job

def get_data():
    on_message
    return testData

scheduler = BackgroundScheduler()
running_job = scheduler.add_job(get_data, 'interval', seconds=1, max_instances=1)
scheduler.start()

def on_disconnect(client, userdata, rc):
    print("Closing data file...")

client = mqtt.Client(client_id=client_id)
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect
client.username_pw_set(username, password)

client.connect(broker, port, 60)
client.loop_start()

app = Flask(__name__)
app.config['SQLALCHEMY_DATABASE_URI'] = "mysql://root:root@127.0.0.1:8889/window62"
app.config["TEMPLATES_AUTO_RELOAD"] = True
socketio = SocketIO(app)
db = SQLAlchemy(app)

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
        return '<Sensor %r>' % self.sensor_name

@app.route('/')
def index():
    if request.method == "GET":
        objects = Object.query.all()
        return render_template("object.html", objects=objects)

@app.route('/object/<int:id>')
def sensors(id):
    if request.method == "GET":
        # sensors = Sensor.query.join(Object, Sensor.obj_id==Object.obj_id)
        sensors = Sensor.query.filter_by(obj_id=id).all()
        return render_template("sensor.html", sensors=sensors, testData=testData)


if __name__ == '__main__':
    client = mqtt.Client()
    socketio.run(app, debug=True, host='localhost', port=5555)