This is a minimal flask app just to read sensor status from an arduino mega over USB serial.

Python 3 is required for this project.  To install dependencies, 

pip3 install -r requirements.txt

I use this application on a raspberry pi model 3 running the latest stable version of Raspian. To install this application, I drop this folder onto /opt/homemonitor on the raspberry pi.
If you use another location for this directory, you will need to update start_server.sh.  To run on startup, I configured the raspberry pi to have a static IP address and then I used crontab and 
setuid to run start_server.sh at startup.  The reason for using setuid for this is that it runs on port 80 and needs root privileges to bind that port.