var locationOptions = {
  enableHighAccuracy: true, 
  maximumAge: 10000, 
  timeout: 10000
};
currentPosition = null;

function locationSuccess(pos) {
  console.log('lat= ' + pos.coords.latitude + ' lon= ' + pos.coords.longitude);
  if (currentPosition == null) {
      getCloseStops(pos.coords.latitude, pos.coords.longitude);
  }
  currentPosition = { 'x': pos.coords.latitude, 'y': pos.coords.longitude };
}

function locationError(err) {
  console.log('location error (' + err.code + '): ' + err.message);
}

Pebble.addEventListener('ready',
  function(e) {
    // Request current position
    console.log("Hello world! - Sent from your javascript application.");
    navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
    sendMsg({'0':7});
  }
);

Pebble.addEventListener('appmessage', function (e) {
    console.log(JSON.stringify(e.payload));
    if (e.payload[0] == "#GET STOPS") {
        if (currentPosition !== null) {
            getCloseStops(currentPosition.x,
                          currentPosition.y);    
        } else {
            /* tell pebble that we know shit right now */
        }
    } else {
        getLinesForStop(e.payload[0]);
    }
});

canSend = true;
msgQueue = [];
function sendMsg(data) {
    if (canSend) { 
        canSend = false;
        var forsure = setTimeout(function(){canSend=true;}, 5000);
        Pebble.sendAppMessage( data,
            function(e) {
                console.log('Successfully delivered message with transactionId=' + e.data.transactionId);
                canSend = true;
                clearTimeout(forsure);
                if (msgQueue.length > 0) {
                    sendMsg(msgQueue.shift());
                }
            },
            function(e) {
                console.log('Unable to deliver message with transactionId='
                  + e.data.transactionId
                  + ' Error is: ' + e.error.message);
                canSend = true;
                clearTimeout(forsure);
                if (msgQueue.length > 0) {
                    sendMsg(msgQueue.shift());
                }
            }
        );
        console.log(JSON.stringify(data));
    } else {
        msgQueue.push(data);
    }
}

function getCloseStops(latitude, longitude) {
    var http = new XMLHttpRequest();
    http.open("GET",
            "http://busio.com.pl/Home/GetNearestStops?x=" + latitude + "&y=" + longitude + "&r=1000");
    http.onreadystatechange = function () {
        console.log(http.readyState + ' ' + http.status);
        if (http.readyState == 4 && http.status == 200) {
            console.log(http.responseText);
            var json = JSON.parse(http.responseText);
            json.sort(function(a, b) { return Math.pow(a.x - b.x, 2) + Math.pow(a.y - b.y, 2); });
            var result = {};
            result[0] = 1;
            for (var i = 0; i < json.length; i++) {
                result[i + 1] = json[i].data.name;
            }
            sendMsg(result);
        }
    } 
    http.send(null);
}

function getLinesForStop(stop) {
    var http = new XMLHttpRequest();
    http.open("GET",
            "http://busio.com.pl/Home/GetDeparturesForStop?stop=" + stop + "&delay=0");
    http.onreadystatechange = function () {
        console.log(http.readyState + ' ' + http.status);
        if (http.readyState == 4 && http.status == 200) {
            var json = JSON.parse(http.responseText);
            var index = 1;
            var current_hour = new Date().getHours();
            var lines = [];
            for (var sname in json)
            {
                var time_str = "";
                for (var hour in json[sname]) {
                    if (hour >= current_hour && json[sname][hour].length !== '-') {
                        time_str += hour + ":" + json[sname][hour] + ' ';  
                    }
                }
                if (time_str.length > 0) { 
                    lines.push([sname, time_str]);
                }
            }
            lines.sort();
            var result = {'0':2}
            lines.forEach(function(line) {
                result[index++] = line[0];
                result[index++] = line[1];
                // this means always
                if (Object.keys(result).length > 2) {
                    sendMsg(result);
                    result = {'0':2};
                }
            });
            sendMsg(result);
        }
    } 
    http.send(null);
}