use window62;

insert into room (room_name) values ("room1");
insert into object (object.obj_name, object.obj_status, object.room_id) values("Window", "open", 1);
insert into object (object.obj_name, object.obj_status, object.room_id) values("Curtain", "open", 1);
insert into object (object.obj_name, object.obj_status, object.room_id) values("Curtain Bind", "open", 1);

insert into sensor (sensor_name, obj_id, sensor_unit) values 
	("Temperature", 1, "°C"),
	("LDR", 2, "%"),
    ("LDR", 3, "%"),
	("Humidity", 1, "%"),
	("Dust", 1, "mg/m³");


-- insert into temp_reading (temp_input, insertTimestamp, sensor_id) values(29, NOW(), 1);
-- insert into temp_reading (temp_input, insertTimestamp, sensor_id) values(3, NOW(), 1);

-- insert into ldr_reading (ldr_input, insertTimestamp, sensor_id) values(33, NOW(), 2);
-- insert into ldr_reading (ldr_input, insertTimestamp, sensor_id) values(36, NOW(), 2);
-- insert into ldr_reading (ldr_input, insertTimestamp, sensor_id) values(23, NOW(), 2
select * from object_time_setting;
