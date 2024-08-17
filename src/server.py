from flask import Flask, render_template, request, jsonify
import requests
from sys import argv

#curl http://127.0.0.1:33333/api --data "cmd=set&key=abc&value=def"
#curl http://127.0.0.1:33333/api --data "cmd=get&key=abc"

class API(object):
    
    def __init__(self, ip, port):
        print(self)
        self.ip = ip
        self.port = port
        
        self.url = "http://{}:{}".format(self.ip, str(self.port))
        
    def set(self, key, value):
        
        return requests.post(self.url, data={
            "cmd":"SET",
            key:value
        })
        

    def rem(self, key):
        return requests.post(self.url, data={
            "cmd":"REM",
            "key":key
        })

    def get(self, key):
        return requests.post(self.url, data={
            "cmd":"GET",
            "key":key
        })

    def add(self, ip, port, weight):
        return requests.post(self.url, data={
            "cmd":"ADD",
            "ip":ip,
            "port":port,
            "weight":weight
        })

    def delete(self, ip, port):
        return requests.post(self.url, data={
            "cmd":"DEL",
            "ip":ip,
            "port":port
        })

app = Flask(__name__)
api = API(argv[2], argv[3])

@app.route("/")
def root():
    return ""

@app.route("/api", methods=("GET", "POST"))
def insert():
    
    if request.method == 'POST':
     
        if "cmd" in request.form.keys():
            
            command = request.form["cmd"].upper()

            if(command == "GET"):
                r = api.get( request.form["key"] )
                return r.text
            if(command == "SET"):
                key = request.form["key"]
                value = request.form["value"]
                r = api.set(key, value)
                return r.text
            if(command == "REM"):
                r = api.rem(request.form["key"])
                return r.text
            if(command == "ADD"):
                r = api.add(request.form["ip"], request.form["port"], request.form["weight"])
                return r.text
            if(command == "DEL"):
                r = api.delete(request.form["ip"], request.form["port"])
                return r.text
            return "OK\r\n"
    
    
    return "\r\n"


app.run(port=argv[1], debug=True)
    