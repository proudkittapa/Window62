import uuid

from flask import Flask, render_template, request, flash, url_for, redirect
from flask_sqlalchemy import SQLAlchemy
import paho.mqtt.client as mqtt
import random
from flask_apscheduler import APScheduler
import atexit

broker = 'broker.emqx.io'
port = 1883
# topic = "test"
client_id = f'python-mqtt-{random.randint(0, 1000)}'
client_id2 = f'python-mqtt-{random.randint(0, 1000)}'

username = 'emqx'
password = 'public'

app = Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'
app.config['SQLALCHEMY_DATABASE_URI'] = "mysql://root:root@127.0.0.1:8889/window62"
app.config["TEMPLATES_AUTO_RELOAD"] = True
app.config['SERVER_NAME'] = "localhost:5555"

db = SQLAlchemy(app)

tempData = "disconnected"
lightData = "disconnected"
humidityData = "disconnected"
dustData = "disconnected"
pm25Data = "disconnected"

scheduler = APScheduler()
scheduler.daemonic = False


class Topic:
    def __init__(self, topic, message):
        self.topic = topic
        self.message = message

    def publish(self):
        result = client2.publish(self.topic, self.message)
        print("result pub:", result, self.topic, self.message)

    # def add_schedule(self):
    #     scheduler.add_job(id="sch", func=self.publish, trigger="interval", seconds=3)
    #     scheduler.start()


class object_setup(db.Model):
    obj_setup_id = db.Column(db.Integer, primary_key=True)
    obj_id = db.Column(db.Integer)
    obj_setup_value = db.Column(db.Float)
    obj_setup_sign = db.Column(db.String(10))
    obj_setup_status = db.Column(db.String(20))

    def __init__(self):
        self.msg = None
        self.topic = None

    def insert(self):
        db.session.add(self)
        db.session.commit()

    def publish(self):
        result = client2.publish(self.topic, self.message)
        print("result pub:", result, self.topic, self.message)

    def add_topic(self, topic, msg):
        self.topic = topic
        self.msg = msg


class ObjectSchedule(Topic):
    def __init__(self, topic, message):
        super().__init__(topic, message)
        self.sch_id = str(uuid.uuid4())
        # self.sch_id = sch_id

    def add(self, day, hour, minute):
        scheduler.add_job(id="sch"+self.sch_id, func=self.publish, trigger="cron", day_of_week=day, hour=hour, minute=minute)
        scheduler.start()

    def remove(self):
        scheduler.remove_job("sch"+self.sch_id)


class Object(db.Model):
    obj_id = db.Column(db.Integer, primary_key=True)
    obj_name = db.Column(db.String(20))
    obj_description = db.Column(db.String(100), nullable=True)
    obj_status = db.Column(db.String(20))
    room_id = db.Column(db.Integer, nullable=False)

    def __repr__(self):
        return '<Object %r>' % self.obj_name

    def insert(self):
        db.session.add(self)
        db.session.commit()


class Sensor(db.Model):
    sensor_id = db.Column(db.Integer, primary_key=True)
    sensor_name = db.Column(db.String(20))
    sensor_description = db.Column(db.String(100), nullable=True)
    obj_id = db.Column(db.Integer)
    sensor_unit = db.Column(db.String(10), nullable=True)

    def __repr__(self):
        return '<Object %r>' % self.obj_id

    def insert(self):
        db.session.add(self)
        db.session.commit()


class temp_reading(db.Model):
    temp_id = db.Column(db.Integer, primary_key=True)
    temp_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)

    def __repr__(self):
        return '<temp_reading %r>' % self.temp_id

    def insert(self):
        db.session.add(self)
        db.session.commit()


class ldr_reading(db.Model):
    ldr_id = db.Column(db.Integer, primary_key=True)
    ldr_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)

    def __repr__(self):
        return '<ldr_reading %r>' % self.ldr_id

    def insert(self):
        db.session.add(self)
        db.session.commit()


class humidity_reading(db.Model):
    humidity_id = db.Column(db.Integer, primary_key=True)
    humidity_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)

    def __repr__(self):
        return '<ldr_reading %r>' % self.humidity_id

    def insert(self):
        db.session.add(self)
        db.session.commit()


class pm_reading(db.Model):
    pm_id = db.Column(db.Integer, primary_key=True)
    pm_input = db.Column(db.Float)
    sensor_id = db.Column(db.Integer)

    def __repr__(self):
        return '<ldr_reading %r>' % self.pm_id

    def insert(self):
        db.session.add(self)
        db.session.commit()


class transaction_obj(db.Model):
    tran_id = db.Column(db.Integer, primary_key=True)
    obj_id = db.Column(db.Integer)
    obj_status = obj_status = db.Column(db.String(20))

    def __repr__(self):
        return '<ldr_reading %r>' % self.tran_id

    def insert(self):
        db.session.add(self)
        db.session.commit()


class object_time_setting(db.Model):
    obj_ts_id = db.Column(db.Integer, primary_key=True)
    obj_id = db.Column(db.Integer)
    obj_ts_time = db.Column(db.DateTime)
    obj_ts_day = db.Column(db.String(20))
    obj_ts_created = db.Column(db.DateTime)

    def insert(self):
        db.session.add(self)
        db.session.commit()


def on_connect(client, userdata, flags, rc):
    # print("Connected with result code " + str(rc), userdata)
    client.subscribe("WindowLight")
    client.subscribe("WindowTemp")
    client.subscribe("WindowHumidity")
    client.subscribe("WindowPM25")
    client.subscribe("ConfirmWindowCmd")
    client.subscribe("ConfirmCurtainCmd")
    client.subscribe("Start")


def on_message(client, userdata, msg):
    # print(msg.topic+" "+str(msg.payload.decode()) + '\n')
    message = str(msg.payload.decode())
    if msg.topic == "WindowTemp":
        val = float(msg.payload.decode())
        temp_reading(sensor_id=1, temp_input=val).insert()
        global tempData
        tempData = message
    elif msg.topic == "WindowLight":
        val = float(msg.payload.decode())
        ldr_reading(sensor_id=2, ldr_input=val).insert()
        print("Light", message)
        global lightData
        lightData = message
    elif msg.topic == "WindowHumidity":
        val = float(msg.payload.decode())
        humidity_reading(sensor_id=3, humidity_input=val).insert()
        print("humid", message)
        global humidityData
        humidityData = message
    elif msg.topic == "WindowPM25":
        print("topic", msg.topic)
        val = float(msg.payload.decode())
        pm_reading(sensor_id=4, pm_input=val).insert()
        print("PM25", message)
        global pm25Data
        pm25Data = message
    elif msg.topic == "Start":
        print("start here")
        start()


def on_disconnect(client, userdata, rc):
    # print("Closing data file...")
    pass


client = mqtt.Client(client_id=client_id)
client.on_connect = on_connect
client.on_message = on_message
client.on_disconnect = on_disconnect
client.username_pw_set(username, password)

client.connect(broker, port, 60)
client.loop_start()

client2 = mqtt.Client(client_id=client_id2)
client2.on_connect = on_connect
client2.username_pw_set(username, password)

client2.connect(broker, port, 60)
client2.loop_start()


@app.route('/')
def index():
    if request.method == "GET":
        objects = Object.query.all()
        sensors = []
        for obj in objects:
            try:
                sensor = Sensor.query.filter_by(obj_id=obj.obj_id).all()
                sensors.append(sensor)
            except:
                return "There was an issue adding task"

        return render_template("page1.html", objects=objects, sensors=sensors, lightData=lightData, tempData=tempData,
                               pm25Data=pm25Data, humidityData=humidityData)


@app.route('/object/<int:obj_id>')
def obj_sensors_control(obj_id):
    if request.method == "GET":
        # sensors = Sensor.query.join(Object, Sensor.obj_id==Object.obj_id)
        sensors_query = Sensor.query.filter_by(obj_id=obj_id).all()
        object_query = Object.query.filter_by(obj_id=obj_id).first()
        return render_template("control.html", sensors=sensors_query, object=object_query, lightData=lightData,
                               tempData=tempData, pm25Data=pm25Data, humidityData=humidityData)


@app.route("/object/<int:obj_id>/changeStatus")
def change_status(obj_id):
    object_query = Object.query.filter_by(obj_id=obj_id).first()
    curr_status = "close"
    if object_query.obj_status == "close":
        curr_status = "open"
    topic = object_query.obj_name + "Cmd"
    Topic(topic, curr_status).publish()
    object_query.obj_status = status
    transaction_obj(obj_id=object_query.obj_id, obj_status=status).insert()
    return render_template("status.html")


def start():
    objects = Object.query.all()
    for obj in objects:
        topic = obj.obj_name + "Cmd"
        Topic(topic, obj.obj_status).publish()


@app.route("/object/<int:obj_id>/status/<curr_status>")
def status(obj_id, curr_status):
    obj = Object.query.filter_by(obj_id=obj_id).first()
    with app.app_context():
        topic = obj.obj_name + "Cmd"
        Topic(topic, curr_status).publish()
        obj.obj_status = curr_status
        transaction_obj(obj_id=obj.obj_id, obj_status=curr_status).insert()
        return render_template("status.html")


@app.route("/object/<int:obj_id>/setup", methods=["GET", "POST", "DELETE"])
def save_setup(obj_id):
    objects = object_setup.query.filter_by(obj_id=obj_id).all()
    object_name = Object.query.filter_by(obj_id=obj_id).first()
    sensors = Sensor.query.filter_by(obj_id=obj_id).all()
    name = object_name.obj_name
    if request.method == "POST":
        req = request.form
        setup = object_setup(obj_id=obj_id, obj_setup_value=req.get("value"), obj_setup_sign=req.get("sign"),
                             obj_setup_status=req.get("status"))
        # check conditions
        allow = True
        for obj in objects:
            # TODO fix logic (didn't check sensors)
            if (obj.obj_id != setup.obj_id) and (
                    (obj.obj_setup_sign == "more" and float(setup.obj_setup_value) >= obj.obj_setup_value) or (
                    obj.obj_setup_sign == "less" and float(setup.obj_setup_value) <= obj.obj_setup_value)):
                flash("The condition is not possible")
                allow = False
                break
        if allow:
            try:
                setup.insert()
                topic = "Conditions"
                sensor_name = get_sensor_name_by_setup_id(setup.obj_id)
                setup_str = str(setup.obj_setup_id) + "," + sensor_name + "," + setup.obj_setup_sign + "," + str(
                    setup.obj_setup_value)
                save_topic = Topic(topic, setup_str)
                save_topic.publish()
                return redirect("/object/" + str(obj_id) + "/setup")
            except:
                db.session.rollback()
                flash("The condition already existed")

    return render_template("setup.html", id=obj_id, objects=objects, name=name, sensors=sensors,
                           get_sensor_name_by_setup_id=get_sensor_name_by_setup_id,
                           get_sensor_unit_by_setup_id=get_sensor_unit_by_setup_id, unit="")


def get_sensor_name_by_setup_id(setup_id):
    sensor = Sensor.query.filter_by(obj_id=setup_id).first()
    return sensor.sensor_name


def get_sensor_unit_by_setup_id(setup_id):
    sensor = Sensor.query.filter_by(obj_id=setup_id).first()
    return sensor.sensor_unit


@app.route("/object/<int:obj_id>/setup/delete/<int:setup_id>", methods=["GET"])
def delete_setup(obj_id, setup_id):
    object_setup.query.filter(object_setup.obj_setup_id == setup_id).delete()
    db.session.commit()
    topic = "Conditions"
    msg = str(setup_id) + ",Cancel"
    Topic(topic, msg).publish()
    return redirect("/object/" + str(obj_id) + "/setup")


if __name__ == '__main__':
    client = mqtt.Client()
    app.run(debug=True, host='localhost', port=5555)
    atexit.register(lambda: scheduler.shutdown())