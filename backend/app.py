from flask import Flask, request, jsonify, render_template
import os
from datetime import datetime

app = Flask(__name__)

# Global state and data
system_state = {"power": False}
data_store = {
    "left_distance": None,
    "right_distance": None,
    "depth": None,
    "pitch": None,
    "roll": None
}

@app.route("/")
def index():
    return render_template("index.html", system_state=system_state, data=data_store)

@app.route("/control", methods=["POST"])
def control():
    content = request.json
    if "state" in content:
        system_state["power"] = content["state"]
        return jsonify({"success": True, "power": system_state["power"]})
    return jsonify({"success": False, "message": "Missing 'state' in request."}), 400

@app.route("/send-data", methods=["POST"])
def receive_data():
    content = request.json
    for key in ["left_distance", "right_distance", "depth", "pitch", "roll"]:
        if key in content:
            data_store[key] = content[key]
    return jsonify({"success": True})

@app.route("/get-data", methods=["GET"])
def get_data():
    return jsonify({"power": system_state["power"], **data_store})

if __name__ == "__main__":
    app.run(debug=True, host="0.0.0.0", port=5000)
