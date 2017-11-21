'''
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Python3 script for the power_control project.                                                                                    //
// This script receive Json string from the ES8266-01, through a callback (HTTP POST request).                                      //
// The json data is sent to the database and stored.                                                                                //
// Insert your specific parameters for the Influxdb database and address and port for th flask app.                                 //
// This project is for academic purpose, it may not be perfectly correct.                                                           //
//                                                                                                                                  //
// Author: Saraceni Rodolfo                                                                                                         //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
'''
#!/usr/bin/python3

# Import all the module needed
import flask, sys
from flask import Flask, request, json
from influxdb import InfluxDBClient

# Set the parameters of my database
db_server = 'INFLUXDB SERVER ADDRESS'
db_port = 'INFLUXDB PORT'
db_user = 'INFLUXDB USER'
db_pass = 'INFLUXDB PASSWORD'
db_name = 'DATABASE NAME'

# Create an instance of influxdb module
db = InfluxDBClient(db_server, db_port, db_user, db_pass, db_name)

# Create an app of Flask module
app = Flask(__name__)

# Define the URL and the method of the app
@app.route('/URL OF YOUR APP', methods=['POST'])

# Define the function and the operation when it receive any data
def index():
    if request.headers['Content-Type'] =='application/json':    # Set the content-type on json 
        dati = request.get_json(force=True)
    print (dati)    # Print data received for debugging
    sys.stdout.flush()
    db.write_points(dati)    # Send the json string to influxdb in order to save them on database
    print ("--------------WRITE ON DATABASE--------------")
    sys.stdout.flush()
    return ('', 200)    # Return "200" if the communication is correct

# Define the localhost address and a port
def start():
    app.run(host='0.0.0.0', port=PORT, debug=False)

# Start the app and publish message to debugging
if __name__=='__main__':
    start()
    sys.stdout.flush()
