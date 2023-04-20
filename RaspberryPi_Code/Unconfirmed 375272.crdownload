# home_app.py
# Author: Andy Bard
# This is a minimal app that will allow for local monitoring of 
# the various sensors we have configured over the network
# it includes scheduling and requests for webhooks for potentially
# Checking status/getting updates while on vacation.

from apscheduler.schedulers.background import BackgroundScheduler
from flask import Flask, render_template
import os
import datetime
import requests
import serial
import string

template_dir = os.path.join(os.path.dirname(__file__), "./")

hubSerial = serial.Serial("/dev/ttyACM0") #connect to hub over usb serial

app = Flask(__name__, template_folder=template_dir)
HEARTBEAT_MINS=1 #this will set the background task interval
SUMP_STATUS = -1
POWER_STATUS = 128
UPDATE_INFO = "STARTING"
LAST_HUB_CHECKIN = None
SENSOR_ARR = [0,0]

def call_ifttt():
    ifttt_url="https://ifttt.com/whateverTODOreplacethis" #TODO
    requests.get(ifttt_url)

#This is a scheduled task to run every X minutes as a heartbeat signal
def heartbeat():
    print("TaskIsRunning Successfully")

#this will update the info from the hub
def updateFromHub():
    global UPDATE_INFO
    global SENSOR_ARR
    global LAST_HUB_CHECKIN
    update_info = None
    #We can occasionally run into timing issues where the hub is communicating with a sensor during our
    #request and whatnot. Just make readline returns, and it will smooth
    #The signal so that it accurately gets us sensor readings and our web output doesn't bounce unnecessarily.
    if hubSerial.inWaiting():
        update_info = hubSerial.readline(10) #use a 10 second timeout
    else:
        hubSerial.writelines(bytes("0\n", "UTF-8"))
    if update_info is not None:
        UPDATE_INFO = update_info
        print(update_info)
        tmp = bytes(update_info.strip())
        tmp_arr = tmp.split(b',')
        print(tmp_arr)
        SENSOR_ARR = [int(a) for a in tmp_arr]
        LAST_HUB_CHECKIN = datetime.datetime.now()

scheduler = BackgroundScheduler()
scheduler.add_job(heartbeat, "interval", minutes=HEARTBEAT_MINS)
scheduler.add_job(updateFromHub, "interval", seconds=10) #run the update every 10 seconds
scheduler.start()

@app.route("/")
def main_route():
    return render_template("template.html", debug = LAST_HUB_CHECKIN, sensors=SENSOR_ARR)

if __name__=="__main__":
    app.run(host="0.0.0.0", port=80)
