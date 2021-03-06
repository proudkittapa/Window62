import datetime
import uuid

from flask import Flask, render_template, request, flash, url_for, redirect
from flask_sqlalchemy import SQLAlchemy
import paho.mqtt.client as mqtt
import random
from flask_apscheduler import APScheduler
import atexit

broker = 'broker.emqx.io'
port = 1883
client_id = f'python-mqtt-{random.randint(0, 1000)}'
client_id2 = f'python-mqtt-{random.randint(0, 1000)}'

username = 'emqx'
password = 'public'

app = Flask(__name__)
app.secret_key = b'_5#y2L"F4Q8z\n\xec]/'
app.config['SQLALCHEMY_DATABASE_URI'] = "mysql://root:root@127.0.0.1:8889/window62"
# app.config['SQLALCHEMY_DATABASE_URI'] = "mysql://root@127.0.0.1:3306/window62"
app.config["TEMPLATES_AUTO_RELOAD"] = True
app.config['SERVER_NAME'] = "localhost:5555"

db = SQLAlchemy(app)

tempData = "--"
lightData = "--"
humidityData = "--"
dustData = "--"
pm25Data = "--"


class Topic:
    def __init__(self, topic, message):
        self.topic = topic
        self.message = message

    def publish(self):
        result = client2.publish(self.topic, self.message)

class object_condition_setting(db.Model):
    obj_cs_id = db.Column(db.Integer, primary_key=True)
    obj_id = db.Column(db.Integer)
    obj_cs_sensor_id = db.Column(db.Integer)
    obj_cs_value = db.Column(db.Float)
    obj_cs_sign = db.Column(db.String(10))
    obj_cs_status = db.Column(db.String(20))

    def insert(self):
        db.session.add(self)
        db.session.commit()


scheduler = APScheduler()
scheduler.daemonic = False


class ObjectSchedule(Topic):
    def __init__(self, topic, message, sch_id):
        super().__init__(topic, message)
        self.sch_id = sch_id

    def add(self, day, hr, min):
        scheduler.add_job(id=str(self.sch_id), func=self.publish, trigger="cron", day_of_week=day, hour=hr, minute=min)
        scheduler.start()

    def add_interval(self):
        scheduler.add_job(id=str(self.sch_id), func=self.publish, trigger="interval", seconds=3)
        scheduler.start()

    def remove(self):
        scheduler.remove_job(self.sch_id)


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

class object_ts_day(db.Model):
    obj_ts_day_id = db.Column(db.Integer, primary_key=True)
    obj_ts_id = db.Column(db.Integer)
    obj_ts_day = db.Column(db.String(100))

    def insert(self):
        db.session.add(self)
        db.session.commit()

class object_time_setting(db.Model):
    obj_ts_id = db.Column(db.Integer, primary_key=True)
    obj_id = db.Column(db.Integer)
    obj_ts_hour = db.Column(db.Integer)
    obj_ts_min = db.Column(db.Integer)
    obj_ts_created = db.Column(db.DateTime)
    obj_ts_value = db.Column(db.String(10))

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

@app.route("/object/<int:obj_id>/settime/delete/<int:settime_id>", methods=["GET"])
def delete_time_setting(obj_id, settime_id):
    object_ts_day.query.filter(object_ts_day.obj_ts_id == settime_id).delete()
    object_time_setting.query.filter(object_time_setting.obj_ts_id == settime_id).delete()
    db.session.commit()
    try:
        scheduler.remove_job(str(settime_id))
    except:
        flash("The job is deleted")
    return redirect("/object/" + str(obj_id) + "/settime")


@app.route("/object/<int:obj_id>/settime", methods=["GET", "POST", "DELETE"])
def save_time_setting(obj_id):
    objects_setting = object_time_setting.query.filter_by(obj_id=obj_id).all()
    object_query = Object.query.filter_by(obj_id=obj_id).first()
    sensors = Sensor.query.filter_by(obj_id=obj_id).all()
    name = object_query.obj_name
    if request.method == "GET":
        return render_template("settime.html", id=obj_id, objects=objects_setting, name=name, sensors=sensors, get_days_by_ts_id= get_days_by_ts_id)
    elif request.method == "POST":
        req = request.form
        hour = 0
        min = 0
        if req.get("hour") != "":
            hour = int(req.get("hour"))
        if req.get("min") != "":
            min = int(req.get("min"))
        setting_status = req.get("status")
        days = []
        for k, v in request.form.items():
            if k == "hour" or k == "min" or k == "status":
                pass
            else:
                days.append(v)
        setting = object_time_setting(obj_id=int(obj_id), obj_ts_value=setting_status, obj_ts_hour=hour, obj_ts_min=min)
        try:
            setting.insert()
            dayStr = "["
            for day in days:
                setting_day = object_ts_day(obj_ts_id=int(setting.obj_ts_id), obj_ts_day=day)
                setting_day.insert()
                dayStr += day+","
            
            dayStr = dayStr[:-1]+"]"
            new_schedule = ObjectSchedule("test", "message", setting.obj_ts_id)
            new_schedule.add(dayStr, setting.obj_ts_hour, setting.obj_ts_min)
        except Exception as error:
            flash("The condition already existed")
        
        return redirect("/object/" + str(obj_id) + "/settime")


@app.route("/object/<int:obj_id_para>/setup", methods=["GET", "POST", "DELETE"])
def save_condition_setting(obj_id_para):
    objects = object_condition_setting.query.filter_by(obj_id=obj_id_para).all()
    object_name = Object.query.filter_by(obj_id=obj_id_para).first()
    sensors = Sensor.query.filter_by(obj_id=obj_id_para).all()
    name = object_name.obj_name
    if request.method == "POST":
        req = request.form
        setup = object_condition_setting(obj_id=obj_id_para, obj_cs_value=req.get("value"), obj_cs_sign=req.get("sign"),
                             obj_cs_status=req.get("status"), obj_cs_sensor_id=req.get("sensor_id"))
        # check conditions
        allow = True
        for obj in objects:
            if (obj.obj_id != setup.obj_id) and (
                    (obj.obj_cs_sign == "more" and float(setup.obj_cs_value) >= obj.obj_cs_value) or (
                    obj.obj_cs_sign == "less" and float(setup.obj_cs_value) <= obj.obj_cs_value)):
                allow = False
                flash("The condition is not possible")
        if allow:
            try:
                setup.insert()
                topic = "Conditions"
                sensor_name = get_sensor_name_by_setup_id(setup.obj_cs_sensor_id)
                setup_str = str(setup.obj_cs_id) + "," + sensor_name + "," + setup.obj_cs_sign + "," + str(
                    setup.obj_cs_value)
                save_topic = Topic(topic, setup_str)
                save_topic.publish()
                return redirect("/object/" + str(obj_id_para) + "/setup")
            except Exception as error:
                db.session.rollback()
                flash("The condition already existed")
                return redirect("/object/" + str(obj_id_para) + "/setup")
    return render_template("setcondition.html", id=obj_id_para, objects=objects, name=name, sensors=sensors,
                           get_sensor_name_by_setup_id=get_sensor_name_by_setup_id,
                           get_sensor_unit_by_setup_id=get_sensor_unit_by_setup_id, unit="")

def get_sensor_name_by_setup_id(setup_sensor_id):
    sensor = Sensor.query.filter_by(sensor_id=setup_sensor_id).first()
    return sensor.sensor_name


def get_sensor_unit_by_setup_id(setup_sensor_id):
    sensor = Sensor.query.filter_by(sensor_id=setup_sensor_id).first()
    return sensor.sensor_unit

def get_days_by_ts_id(ts_id):
    ts_days = object_ts_day.query.filter_by(obj_ts_id=int(ts_id)).all()
    return ts_days

@app.route("/object/<int:obj_id>/setup/delete/<int:setup_id>", methods=["GET"])
def delete_condition_setting(obj_id, setup_id):
    object_condition_setting.query.filter(object_condition_setting.obj_cs_id == setup_id).delete()
    db.session.commit()
    topic = "Conditions"
    msg = str(setup_id) + ",Cancel"
    Topic(topic, msg).publish()
    return redirect("/object/" + str(obj_id) + "/setup")


if __name__ == '__main__':
    client = mqtt.Client()
    app.run(debug=True, host='localhost', port=5555)
