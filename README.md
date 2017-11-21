# POWER CONTROL

Power Control is an academic project, the scope of this product is the monitoring and control of your domestic photovoltaic system.
Especially you can control the auto-consumption of your production.

The Power Control is composed by two different hardware part: one sensor network and a server.

- The sensors network -
The data are collected with two different sensor connected one with the inverter and one with the legal meter.
The first use an ArduinoMKR1200 board, that through the SigFox network and their backend service send the inverter parameters to the server with a post http request.
The communication with the inverter is on a serial bus (9600/N/1) through Modbus protocol. SigFox allow to send only 12 byte of payload on ISM band, so with a 1% duty cycle.
The second use an Espressif ESP8266-01 board, that collect the value of the energy produced by the photovoltaic plant and the energy consumetd by home and it sent them to the server with a post http request.
For read the correct value of current and calculate the corresponding value of power in Watt, it used the Emonlib library. You can found on their website all the info about.
The first sent data every 15 minutes, while the second every 5 minutes. Both use the use Json datatype.

- The server -
The server is hosted in a Raspeberry board, that with two Python3 script gather the data from sensor.
Also on the server hosted the time series databased InfluxDb and the graphical tool Graphana for collect and visualize the data gathered.

In this repo there are both sketch for the board and the Python3 script. Also I upload the relation for the exam but it's in italian.

As already said this project is for academic purpose, so it couldn't be perfectly correct.
Rodolfo
