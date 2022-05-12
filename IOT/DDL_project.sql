drop database window62_db ;
create database window62_db ;

use window62_db ;

-- table: room
CREATE TABLE room(
	room_id int primary key auto_increment, 
	room_name varchar(100)	NOT NULL
);

-- table: object
CREATE TABLE object(
	obj_id int primary key auto_increment, 
	obj_name varchar(20)	NOT NULL,
    obj_description varchar(100)  NULL,
    obj_status varchar(10)	NOT NULL,
    room_id int not null,
    FOREIGN KEY (room_id) REFERENCES room(room_id)
);

-- table: sensor
CREATE TABLE sensor(
	sensor_id int primary key auto_increment, 
	sensor_name varchar(20)	NOT NULL,
    sensor_description varchar(100)  NULL,
    obj_id int not null,
    FOREIGN KEY (obj_id) REFERENCES object(obj_id),
	sensor_unit varchar(10) NULL default "no unit"
);

-- table: motor
CREATE TABLE motor(
	motor_id int primary key auto_increment, 
	motor_name varchar(20)	NOT NULL,
    motor_description varchar(100)	NULL,
    obj_id int not null,
    FOREIGN KEY (obj_id) REFERENCES object(obj_id)
);

CREATE TABLE sensor_reading(
	sensor_reading_id int primary key auto_increment, 
    sensor_reading_input double not null, 
    insertTimestamp datetime default current_timestamp not null,
    sensor_id int not null, 
    FOREIGN KEY (sensor_id) REFERENCES sensor(sensor_id)
);


CREATE TABLE motor_result(
	motor_result_id int primary key auto_increment, 
	motor_id int not null, 
    motor_data varchar(100) 	not null, 
    insertTimestamp datetime default current_timestamp not null,
    FOREIGN KEY (motor_id) REFERENCES motor(motor_id)
);


CREATE TABLE transaction_obj(
	tran_id int primary key auto_increment, 
	obj_id int	not null, 
    obj_status varchar(10)	not null, 
    insertTimestamp datetime default current_timestamp not null,
    FOREIGN KEY (obj_id) REFERENCES object(obj_id)
);

CREATE TABLE object_condition_setting (
	obj_cs_id int primary key auto_increment,
	obj_id int	not null, 
    obj_cs_sensor_id int not null,
    FOREIGN KEY (obj_cs_sensor_id) REFERENCES sensor(sensor_id),
	FOREIGN KEY (obj_id) REFERENCES object(obj_id),
    obj_cs_value double not null,
	obj_cs_sign varchar(10) not null, 
    obj_cs_status varchar(20) not null,
    CONSTRAINT cs_unique UNIQUE(obj_cs_sign, obj_cs_value, obj_cs_sensor_id)
);

CREATE TABLE object_time_setting (
	obj_ts_id int primary key auto_increment,
	obj_id int	not null, 
	FOREIGN KEY (obj_id) REFERENCES object(obj_id),
    obj_ts_hour int not null default 0,
    obj_ts_min int not null default 0,
    obj_ts_created datetime default current_timestamp,
    obj_ts_value varchar(10)
);

CREATE TABLE object_ts_day(
	obj_ts_day_id int primary key auto_increment,
    obj_ts_id int not null,
    FOREIGN KEY (obj_ts_id) REFERENCES object_time_setting(obj_ts_id),
    obj_ts_day varchar(100) default ""
);