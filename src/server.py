from flask import Flask, render_template, request, jsonify
import requests

#curl http://127.0.0.1:33333/api --data "cmd=set&key=abc&value=def"
#curl http://127.0.0.1:33333/api --data "cmd=get&key=abc"

class API(object):
    
    def __init__(self, ip, port):
        print(self)
        self.ip = ip
        self.port = port
        
        self.url = "http://{}:{}".format(self.ip, str(self.port))
        
    def add(self, key, value):
        
        return requests.post(self.url, data={
            "cmd":"SET",
            key:value
        })
        

    def delete(self, key):
        return requests.post(self.url, data={
            "cmd":"REM",
            "key":key
        })

    def lookup(self, key):
        return requests.post(self.url, data={
            "cmd":"GET",
            "key":key
        })


app = Flask(__name__)
api = API("127.0.0.1", 5556)

@app.route("/")
def root():
    return ""

@app.route("/api", methods=("GET", "POST"))
def insert():
    
    if request.method == 'POST':
     
        if "cmd" in request.form.keys():
            
            command = request.form["cmd"].upper()

            if(command == "GET"):
                r = api.lookup( request.form["key"] )
                return r.text
            if(command == "SET"):
                key = request.form["key"]
                value = request.form["value"]
                r = api.add(key, value)
                return r.text
            if(command == "REM"):
                r = api.delete(request.form["key"])
                return r.text
          
            return "OK\r\n"
    
    
    return "\r\n"


app.run(port=33333, debug=True)
    