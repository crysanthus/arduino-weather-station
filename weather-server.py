#!flask/bin/python
#
# Environmental Data Sensing and Storing 
# mscs crysanthus@gmail.com
#
import datetime

from flask import Flask, abort, request, make_response, jsonify, render_template, send_from_directory

# db
import sqlite3

# graph
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt

 
# WSGI Server
from gevent.wsgi import WSGIServer

# create db and table in init case
try:
 conn = sqlite3.connect('homestats.db')
 cursr = conn.cursor()
 cursr.execute('''CREATE TABLE IF NOT EXISTS weather_data
  (iid integer primary key, date text, temp text, humd text, lumn text)''') 
 conn.commit()

except:
 abort(500)

app = Flask(__name__, static_url_path='')

# serve images directly, no process
@app.route('/favicon.ico')
def send_favicon():
    return send_from_directory('templates/images', 'favicon.ico')

# serve images directly, no process
@app.route('/images/<path:path>')
def send_images(path):
    return send_from_directory('templates/images', path)
    
# serve graph directly, no process
@app.route('/graphs/<path:path>')
def send_graphs(path):
    return send_from_directory('templates/graphs', path)

# save data
# save GET data -- working part --
@app.route('/api/addstatsg', methods=['GET'])
def add_stats_get():

 try:
  # check to see if API token is correct -- TODO --
  if not request.args['cert']:
   abort(417)
 
  if request.args['cert'] != 'xyz':
    abort(417)
  # check to see if API token is correct -- TODO --
  
  # chk to see if the field have correct values
  if not request.args['temp'] or not request.args['temp'].isnumeric():
   abort(404)

  if not request.args['humd'] or not request.args['humd'].isnumeric():
   abort(404)

  if not request.args['lumn'] or not request.args['lumn'].isnumeric():
   abort(404)
 
  stats = {'date': datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
   'temp': request.args['temp'],
   'humd': request.args['humd'],
   'lumn': request.args['lumn']
   }

 except:
  abort(501)
 
 try:
  conn = sqlite3.connect('homestats.db')
  cursr = conn.cursor()

  cursr.execute('''INSERT INTO weather_data 
   VALUES(NULL, :date, :temp, :humd, :lumn)''', stats)
  conn.commit()
  
 except:
  abort(500)

 return jsonify({'homestats':'data saved'}), 201


# create graph and serve stat html
@app.route('/')
def index():

 # do the graph
 plt.ylabel('Units')
 plt.xlabel('Date/Time')
 
 # get data
 conn = sqlite3.connect('homestats.db')
 cursr = conn.cursor()
 cursr.execute('''SELECT date, temp, humd, lumn FROM weather_data;''')
 xv = [] # x axis
 yv1 = [] # y axis - tempreture in red
 yv2 = [] # y axis - humidity in green
 yv3 = [] # y axis - ambient light in yellow
 
  # add data to graph
 for row in cursr:
  xv.append(datetime.datetime.strptime(row[0],"%Y-%m-%d %H:%M:%S"))
  yv1.append(myFloat(row[1]))
  yv2.append(myFloat(row[2]))
  yv3.append(myFloat(row[3]))

 # legends
 plt.plot(xv, yv1, '-r', label='Tempreture', linewidth=0.50)
 plt.plot(xv, yv2, '-g', label='Humidity', linewidth=0.75)
 plt.plot(xv, yv3, '-y', label='Light', linewidth=0.80)
 # beautify the x-labels
 plt.gcf().autofmt_xdate()
 plt.legend()
 
 # save graph as a .png file to serve via html
 plt.savefig('templates/graphs/statistic.png')
 plt.close()
 
 # -- send html out with graph --
 return render_template('stats.html')

# http errors
@app.errorhandler(404)
def not_found(error):
    return make_response(jsonify({'error': 'Not found'}), 404)

@app.errorhandler(500)
def not_found(error):
    return make_response(jsonify({'error': 'Server Internal error'}), 500)

@app.errorhandler(501)
def not_found(error):
    return make_response(jsonify({'error': 'Not Implemented'}), 501)

@app.errorhandler(417)
def not_found(error):
    return make_response(jsonify({'error': 'Token error'}), 417)

# chk to see if a string is float-able
def myFloat(s):
    try:
        return float(s)
    except ValueError:
        return 0.0

if __name__ == '__main__':
    http_server = WSGIServer(('0.0.0.0', 5000), app)
    http_server.serve_forever()
    
